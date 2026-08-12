# Seed ephemeral/flutter_linux headers from the engine cache when missing.
# Invoked AFTER flutter_assemble (Distrobox often leaves an empty flutter_linux/).
if(FLUTTER_ROOT STREQUAL "" OR EPHEMERAL_DIR STREQUAL "")
  message(FATAL_ERROR "seed_flutter_linux.cmake requires FLUTTER_ROOT and EPHEMERAL_DIR")
endif()

set(_hdr "${EPHEMERAL_DIR}/flutter_linux/flutter_linux.h")
set(_src "${FLUTTER_ROOT}/bin/cache/artifacts/engine/linux-x64/flutter_linux")

if(EXISTS "${_hdr}")
  return()
endif()

if(NOT EXISTS "${_src}/flutter_linux.h")
  message(FATAL_ERROR "Engine flutter_linux headers missing at ${_src}")
endif()

file(MAKE_DIRECTORY "${EPHEMERAL_DIR}/flutter_linux")
file(COPY "${_src}/" DESTINATION "${EPHEMERAL_DIR}/flutter_linux")
if(NOT EXISTS "${_hdr}")
  message(FATAL_ERROR "Failed to seed flutter_linux.h into ${EPHEMERAL_DIR}/flutter_linux")
endif()
message(STATUS "Seeded flutter_linux headers from engine cache")