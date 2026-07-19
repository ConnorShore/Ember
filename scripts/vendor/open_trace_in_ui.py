#!/usr/bin/env python3
# Vendored from: https://raw.githubusercontent.com/google/perfetto/main/tools/open_trace_in_ui
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import http.server
import os
import socketserver
import sys
import webbrowser
import urllib


# Premade PerfettoSQL queries, keyed by name, for use with --preset. These are appended to the
# deep-link URL's `query` param so the Perfetto UI runs them automatically as soon as a trace
# loads - see https://perfetto.dev/docs/visualization/deep-linking-to-perfetto-ui.
#
# "system-frame-breakdown" relies on Scene::OnUpdateRuntime/OnUpdateEdit (Ember/src/Ember/Scene/
# Scene.cpp) wrapping every ECS system's OnUpdate call in an EB_PROFILE_SCOPE named
# "<System>::OnUpdate", and on the top-level "Frame" scope in Application::Run that brackets one
# full frame. It reports each system's total/average time and what percentage of the frame
# budget it consumes.
QUERY_PRESETS = {
    'system-frame-breakdown':
        '''
        WITH frame_total AS (
          SELECT SUM(dur) AS total_dur, COUNT(*) AS frame_count
          FROM slice WHERE name = 'Frame'
        ),
        system_totals AS (
          SELECT
            REPLACE(name, '::OnUpdate', '') AS system,
            COUNT(*) AS updates,
            SUM(dur) AS total_dur
          FROM slice
          WHERE name LIKE '%System::OnUpdate'
          GROUP BY name
        )
        SELECT
          'Frame (total)' AS system,
          frame_count AS updates,
          ROUND(total_dur / 1e6, 3) AS total_ms,
          ROUND(total_dur / frame_count / 1e3, 3) AS avg_us_per_call,
          100.0 AS pct_of_frame
        FROM frame_total
        UNION ALL
        SELECT
          system,
          updates,
          ROUND(total_dur / 1e6, 3) AS total_ms,
          ROUND(total_dur / updates / 1e3, 3) AS avg_us_per_call,
          ROUND(total_dur * 100.0 / (SELECT total_dur FROM frame_total), 2) AS pct_of_frame
        FROM system_totals
        ORDER BY total_ms DESC
        ''',
}


class ANSI:
  END = '\033[0m'
  BOLD = '\033[1m'
  RED = '\033[91m'
  BLACK = '\033[30m'
  BLUE = '\033[94m'
  BG_YELLOW = '\033[43m'
  BG_BLUE = '\033[44m'


# HTTP Server used to open the trace in the browser.
class HttpHandler(http.server.SimpleHTTPRequestHandler):

  def end_headers(self):
    self.send_header('Access-Control-Allow-Origin', self.server.allow_origin)
    self.send_header('Cache-Control', 'no-cache')
    super().end_headers()

  def do_GET(self):
    if self.path != '/' + self.server.expected_fname:
      self.send_error(404, 'File not found')
      return

    self.server.fname_get_completed = True
    super().do_GET()

  def do_POST(self):
    self.send_error(404, 'File not found')


def prt(msg, colors=ANSI.END):
  print(colors + msg + ANSI.END)


def open_trace(path, open_browser, origin, query=None):
  # We reuse the HTTP+RPC port because it's the only one allowed by the CSP.
  PORT = 9001
  path = os.path.abspath(path)
  os.chdir(os.path.dirname(path))
  fname = os.path.basename(path)
  socketserver.TCPServer.allow_reuse_address = True
  with socketserver.TCPServer(('127.0.0.1', PORT), HttpHandler) as httpd:
    address = f'{origin}/#!/?url=http://127.0.0.1:{PORT}/{fname}&referrer=open_trace_in_ui'
    if query:
      address += f'&query={urllib.parse.quote(query)}'
    if open_browser:
      webbrowser.open_new_tab(address)
    else:
      print(f'Open URL in browser: {address}')

    httpd.expected_fname = fname
    httpd.fname_get_completed = None

    # In case the user's `--origin` has a path component, strip it out
    def strip_url_path(url):
      url_parsed = urllib.parse.urlparse(url)
      return urllib.parse.urlunparse(
          url_parsed._replace(path='', query='', fragment='', params=''))

    httpd.allow_origin = strip_url_path(origin)
    while httpd.fname_get_completed is None:
      httpd.handle_request()


def main():
  examples = '\n'.join([
      ANSI.BOLD + 'Examples:' + ANSI.END,
      '  tools/open_trace_in_ui trace.pftrace',
  ])
  parser = argparse.ArgumentParser(
      epilog=examples, formatter_class=argparse.RawTextHelpFormatter)

  parser.add_argument('positional_trace', metavar='trace', nargs='?')
  parser.add_argument(
      '-n', '--no-open-browser', action='store_true', default=False)
  origin_group = parser.add_mutually_exclusive_group()
  origin_group.add_argument('--origin', default='https://ui.perfetto.dev')
  origin_group.add_argument(
      '--dev-server',
      action='store_true',
      default=False,
      help='Open the trace in the locally hosted devserver. Shorthand for "--origin http://localhost:10000"'
  )
  parser.add_argument(
      '-i', '--trace', help='input filename (overrides positional argument)')
  query_group = parser.add_mutually_exclusive_group()
  query_group.add_argument(
      '--preset',
      choices=sorted(QUERY_PRESETS.keys()),
      help='run a premade PerfettoSQL query as soon as the trace loads')
  query_group.add_argument(
      '--query', help='run an arbitrary PerfettoSQL query as soon as the trace loads')
  parser.add_argument(
      '--list-presets',
      action='store_true',
      default=False,
      help='print available --preset names and exit')

  args = parser.parse_args()

  if args.list_presets:
    for name in sorted(QUERY_PRESETS.keys()):
      print(name)
    sys.exit(0)

  open_browser = not args.no_open_browser

  trace_file = None
  if args.positional_trace is not None:
    trace_file = args.positional_trace
  if args.trace is not None:
    trace_file = args.trace

  if trace_file is None:
    prt('Please specify trace file name', ANSI.RED)
    sys.exit(1)
  elif not os.path.exists(trace_file):
    prt('%s not found ' % trace_file, ANSI.RED)
    sys.exit(1)

  origin = args.origin
  if args.dev_server:
    origin = 'http://localhost:10000'

  query = args.query
  if args.preset:
    # Collapse the preset's indentation/newlines into single spaces for a cleaner URL.
    query = ' '.join(QUERY_PRESETS[args.preset].split())

  prt('Opening the trace (%s) in the browser' % trace_file)
  open_trace(trace_file, open_browser, origin, query)


if __name__ == '__main__':
  sys.exit(main())
