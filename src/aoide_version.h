#pragma once

// Not the product version. CMake, build.sh and tool/build-app.sh define
// AOIDE_VERSION from the VERSION file. This fires only when that define is
// missing — hand-rolled compiles, and the test binaries that never pass it.
// The string is a sentinel, not a release number: do not bump it with VERSION.
#ifndef AOIDE_VERSION
#define AOIDE_VERSION "1.0"
#endif
