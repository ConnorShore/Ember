# Windows scripts

Every script resolves the repository root from its own location (`%~dp0`), so it can be run from any
working directory or double-clicked from Explorer.

```
Build/     GenerateProjects.bat  Build.bat  Clean.bat
Test/      RunTests.bat
Package/   Stage.bat  Package.bat  BuildInstaller.bat  StageExclude.txt
Tools/     LaunchProfilerViewer.bat
```

## Build

| Script | What it does |
| --- | --- |
| `Build\GenerateProjects.bat` | Cleans, then runs Premake to regenerate the solution and `.vcxproj` files. Run it whenever a `premake5.lua` changes or source files are added or removed — the generated file lists are **not** auto-synced with the filesystem. |
| `Build\Build.bat` | Builds from the command line, without opening Visual Studio. |
| `Build\Clean.bat` | Deletes generated solution/project files, `.vs/`, `bin/`, and vendor build directories. |

```bat
Build.bat                            REM Release, whole solution
Build.bat Debug                      REM Debug, whole solution
Build.bat Release Ember-Test         REM only Ember-Test and what it depends on
Build.bat Debug Ember-Forge rebuild  REM full rebuild instead of incremental
Build.bat Dist clean                 REM delete that configuration's output
```

Arguments are order-independent: `Debug`/`Release`/`Dist`/`Profile` selects the configuration
(default `Release`, matching `RunTests.bat`), `rebuild`/`clean` selects the target, and anything else
is treated as a project name. Exits with MSBuild's exit code.

MSBuild is located with `vswhere`, so no developer command prompt is needed. Set `MSBUILD_PATH` to
override discovery. A named project builds its `.vcxproj` directly rather than a solution target,
because MSBuild rewrites project names in the generated solution metaproj and `/t:Ember-Test:Build`
does not resolve — project references still pull in dependencies.

`Build.bat` deliberately does **not** generate projects first: `GenerateProjects.bat` calls
`Clean.bat`, which deletes `bin/`, so chaining them would turn every build into a full rebuild.

It keeps its window open only when double-clicked, so it composes cleanly inside other scripts. The
detection cannot tell a double-click from `cmd /c Build.bat`, so set `EMBER_NO_PAUSE=1` when driving
it that way (from CI, or a tool that captures output).

## Test

```bat
RunTests.bat                        REM Release, every test
RunTests.bat Debug                  REM Debug (auto-sets EMBER_TEST_PERF_SCALE=8)
RunTests.bat --filter=unit          REM one category: unit|integration|visual|performance|stress
RunTests.bat --run=Physics          REM only tests whose Suite::Name contains "Physics"
RunTests.bat --list                 REM list registered tests without running them
```

Exit code is 0 (all passed) or 1 (any failure). See `Ember-Test/README.md` before writing tests.

## Package

Requires an existing **Dist** build — none of these scripts build the project.

| Script | What it does |
| --- | --- |
| `Package\Stage.bat` | Assembles the shipped layout into `build\stage`. Sole owner of that layout. |
| `Package\Package.bat` | `Stage.bat`, then zips it to `build\dist` (portable distribution). |
| `Package\BuildInstaller.bat` | `Stage.bat`, then runs Inno Setup to produce `build\installer\...-Setup.exe`. Pass `/DSIGN` to sign. |
| `Package\StageExclude.txt` | Payload exclusions, as case-insensitive substrings of the full path. |

`Package.bat` and `BuildInstaller.bat` both consume the same staging tree, so the portable and
installed layouts cannot drift apart. `Stage.bat` extracts the version from
`Ember/src/Ember/Core/Version.h` and writes `build\version.iss` and `build\version.cmd` for the other
two to consume, so the version lives in exactly one place.

## Tools

`Tools\LaunchProfilerViewer.bat` opens every `Profiles\*.json` trace in the Perfetto UI via the
vendored `scripts/vendor/open_trace_in_ui.py`. Requires Python 3. Traces only exist for builds with
`EB_PROFILE` defined (the Profile configuration).

## Releases from CI

`.github/workflows/release-installer.yml` runs `GenerateProjects.bat`, `Build.bat Dist`,
`BuildInstaller.bat` and `Package.bat` on a GitHub-hosted runner, then optionally creates a GitHub
Release. Trigger it manually from the **Actions** tab. It cannot run `RunTests.bat` — hosted runners
have no GPU driver and the suite needs an OpenGL 4.5 context — so run the tests locally first.

Every script here honours `EMBER_NO_PAUSE`, which is what lets CI call them without hanging on a
`pause`.

## Adding a script

Put it in the folder matching its purpose, resolve the repo root with `pushd "%~dp0..\..\.."` (three
levels up from a subfolder), and add a row to the table above. If it needs a `pause`, guard it so the
script stays usable from other scripts — `Build.bat` shows one way.
