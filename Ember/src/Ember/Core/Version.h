#pragma once

// The single source of truth for the engine version; Stage.bat greps these defines to build the
// installer, so keep them one-per-line with the value as the third whitespace-separated token.
#define EMBER_VERSION_MAJOR  0
#define EMBER_VERSION_MINOR  2
#define EMBER_VERSION_PATCH  2

// Numeric form for anything that needs a comparable x.y.z
#define EMBER_VERSION_STRING "0.3.0"

// Display form, which may carry a pre-release suffix.
#define EMBER_VERSION_FULL   "0.3.0-alpha"
