# Seed ephemeral/flutter_linux headers from the engine cache when missing.
# Invoked at build time (Distrobox can leave an empty flutter_linux/ dir).
if(NOT DEFINED FLUTTER_ROOT OR NOT DEFINED EPHEMERAL_DIR)
  message(FATAL_ERROR "seed_flutter_linux.cmake requires FLUTTER_ROOT and EPHEMERAL_DIR")
endif()

set(_hdr "${EPHEMERAL_DIR}/flutter_linux/flutter_linux.h")
set(_src "${FLUTTER_ROOT}/bin/cache/artifacts/engine/linux-x64/flutter_linux")

if(EXISTS "${_hdr}")
  return()
endif()

if(NOT EXISTS "${_src}/flutter_linux.h")
  message(WARNING "Engine flutter_linux headers missing at ${_src}")
  return()
endif()

file(MAKE_DIRECTORY "${EPHEMERAL_DIR}/flutter_linux")
file(COPY "${_src}/" DESTINATION "${EPHEMERAL_DIR}/flutter_linux")
message(STATUS "Seeded flutter_linux headers from engine cache")
