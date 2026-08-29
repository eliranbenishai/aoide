cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AOIDE_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${AOIDE_VERSION_RAW}" AOIDE_VERSION)

# APPLE is not defined until project(). These must be set before it so the
# toolchain records the deployment target. 13.0 is Qt 6.10's official floor;
# never lower it — a binary built for 12 will not load that Qt. Raise via the
# cache if a future kit requires it. The DMG is named -universal; the default
# architectures are what make that true. Override when the Qt kit or libmpv is
# single-arch: a universal link against a thin library fails at configure.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
  endif()
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
  endif()
endif()

project(aoide VERSION ${AOIDE_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chrome is rasterised on the CPU every frame, so an unoptimised build does not
# just run slower — it drags at a few frames per second. Default to Release so
# a plain `cmake -S . -B build` never produces a build that feels broken.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/QT_VERSION" AOIDE_QT_VERSION LIMIT_COUNT 1)
string(STRIP "${AOIDE_QT_VERSION}" AOIDE_QT_VERSION)
if(APPLE)
  # Official Qt 6.5+ desktop kits unpack as macos/, same pin layout as Linux
  # gcc_64. fetch_qt.sh does not install this; a Mac host places the kit here
  # or points CMAKE_PREFIX_PATH at it.
  set(_aoide_qt_pin "${CMAKE_CURRENT_SOURCE_DIR}/.local/qt/${AOIDE_QT_VERSION}/macos")
  if(EXISTS "${_aoide_qt_pin}/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${_aoide_qt_pin}")
  endif()
elseif(UNIX)
  set(_aoide_qt_pin "${CMAKE_CURRENT_SOURCE_DIR}/.local/qt/${AOIDE_QT_VERSION}/gcc_64")
  if(EXISTS "${_aoide_qt_pin}/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${_aoide_qt_pin}")
  endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Qt6 QUIET COMPONENTS DBus)
if(NOT Qt6_VERSION VERSION_EQUAL AOIDE_QT_VERSION)
  message(FATAL_ERROR
    "Aoide is pinned to Qt ${AOIDE_QT_VERSION} (QT_VERSION). Found ${Qt6_VERSION}. "
    "On Linux run ./tool/fetch_qt.sh and reconfigure.")
endif()
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "13.0")
  message(FATAL_ERROR
    "CMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} is below "
    "Qt ${AOIDE_QT_VERSION}'s macOS 13 floor. Do not lower it.")
endif()
message(STATUS "Qt ${Qt6_VERSION} matches QT_VERSION")
find_package(PkgConfig QUIET)

qt_standard_project_setup()

# CI compiles through CMake, not build.sh, so this path has to raise the same
# diagnostics build.sh does — ignored return values and signed/unsigned
# conversions are how the worst bugs here got in. Warnings only: the tree is not
# clean, so -Werror would fail on day one. Linked privately into our own targets;
# Qt arrives as -isystem and stays quiet.
add_library(aoide_warnings INTERFACE)
if(MSVC)
  target_compile_options(aoide_warnings INTERFACE /W4 /wd4100 /w14834)
else()
  target_compile_options(aoide_warnings INTERFACE
    -Wall -Wextra -Wno-unused-parameter
    -Wunused-result -Wsign-compare -Wsign-conversion)
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets" AOIDE_ASSET_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/skins" AOIDE_SKINS_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libmpv" AOIDE_MPV_ROOT)

add_library(aoide_window_spec STATIC
  src/window_spec.cpp
  src/host_shell.cpp
  src/title_chrome.cpp
)
target_include_directories(aoide_window_spec PUBLIC src)
target_link_libraries(aoide_window_spec PUBLIC Qt6::Widgets)

set(AOIDE_HAVE_MPV OFF)
set(AOIDE_MPV_LIBS "")
set(BUNDLED_MPV_SO "${AOIDE_MPV_ROOT}/linux/x86_64/libmpv.so")
set(BUNDLED_MPV_IMPLIB "${AOIDE_MPV_ROOT}/windows/x86_64/libmpv.dll.a")
set(AOIDE_WIN_MPV_DLL "${AOIDE_MPV_ROOT}/windows/x86_64/libmpv-2.dll")

if(PkgConfig_FOUND)
  pkg_check_modules(MPV IMPORTED_TARGET mpv)
endif()

if(TARGET PkgConfig::MPV)
  set(AOIDE_HAVE_MPV ON)
  set(AOIDE_MPV_LIBS PkgConfig::MPV)
elseif(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}")
  set(AOIDE_HAVE_MPV ON)
  set(STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs")
  file(MAKE_DIRECTORY "${STUB_DIR}")
  foreach(spec IN ITEMS
      libmujs.so.0.1
      liblua-5.1.so
      libuchardet.so.0
      libvapoursynth-script.so.0
      libXpresent.so.1)
    add_library(stub_${spec} SHARED "src/mpv_stubs/${spec}.c")
    set_target_properties(stub_${spec} PROPERTIES
      PREFIX ""
      SUFFIX ""
      OUTPUT_NAME "${spec}"
      LIBRARY_OUTPUT_DIRECTORY "${STUB_DIR}"
    )
    target_link_options(stub_${spec} PRIVATE "-Wl,-soname,${spec}")
  endforeach()
  list(APPEND AOIDE_MPV_LIBS
    "${BUNDLED_MPV_SO}"
    stub_libmujs.so.0.1
    stub_liblua-5.1.so
    stub_libuchardet.so.0
    stub_libvapoursynth-script.so.0
    stub_libXpresent.so.1
  )
elseif(WIN32 AND EXISTS "${BUNDLED_MPV_IMPLIB}" AND EXISTS "${AOIDE_WIN_MPV_DLL}")
  # The pinned shinchiro archive names its import library GNU-style, but the
  # file is a Microsoft short-import library — the format lib.exe emits. link.exe
  # identifies archives by content rather than extension, so the copy to a
  # conventional name is only to keep the toolchain unsurprised.
  #
  # Both files are required: the import library is what links, the DLL is what
  # ships, and a build that can link but not ship is the silent-Windows defect
  # this branch exists to close.
  set(AOIDE_HAVE_MPV ON)
  set(AOIDE_MPV_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/mpv.lib")
  configure_file("${BUNDLED_MPV_IMPLIB}" "${AOIDE_MPV_IMPLIB}" COPYONLY)
  list(APPEND AOIDE_MPV_LIBS "${AOIDE_MPV_IMPLIB}")
elseif(APPLE)
  # fetch_full_libmpv.sh lifts the archive's versioned wrapper so xcframeworks
  # sit directly under macos/universal/. The second glob is the raw tar layout,
  # in case someone extracted by hand. Reconfigure after a fetch: GLOB does not.
  set(_aoide_mac_mpv_root "${AOIDE_MPV_ROOT}/macos/universal")
  file(GLOB _aoide_mac_mpv_fw
    "${_aoide_mac_mpv_root}/Mpv.xcframework/macos-*/*.framework")
  if(NOT _aoide_mac_mpv_fw)
    file(GLOB _aoide_mac_mpv_fw
      "${_aoide_mac_mpv_root}/*/Mpv.xcframework/macos-*/*.framework")
  endif()
  if(_aoide_mac_mpv_fw)
    list(GET _aoide_mac_mpv_fw 0 AOIDE_MAC_MPV_FRAMEWORK)
    if(EXISTS "${AOIDE_MAC_MPV_FRAMEWORK}/Versions/A/Mpv")
      set(AOIDE_HAVE_MPV ON)
      list(APPEND AOIDE_MPV_LIBS "${AOIDE_MAC_MPV_FRAMEWORK}")
      file(GLOB AOIDE_MAC_MPV_FRAMEWORKS
        "${_aoide_mac_mpv_root}/*.xcframework/macos-*/*.framework"
        "${_aoide_mac_mpv_root}/*/*.xcframework/macos-*/*.framework")
      list(REMOVE_DUPLICATES AOIDE_MAC_MPV_FRAMEWORKS)
      # One rpath for the whole @rpath/*.framework graph (Mpv + FFmpeg + mbed).
      # Contents/Frameworks gets the real copies at POST_BUILD; this directory
      # is what domain_test — not a bundle — uses from the build tree.
      set(AOIDE_MAC_MPV_RPATH_DIR "${CMAKE_CURRENT_BINARY_DIR}/mpv-frameworks")
      file(MAKE_DIRECTORY "${AOIDE_MAC_MPV_RPATH_DIR}")
      foreach(_fw IN LISTS AOIDE_MAC_MPV_FRAMEWORKS)
        get_filename_component(_fw_name "${_fw}" NAME)
        set(_fw_link "${AOIDE_MAC_MPV_RPATH_DIR}/${_fw_name}")
        if(EXISTS "${_fw_link}")
          file(REMOVE "${_fw_link}")
        endif()
        file(CREATE_LINK "${_fw}" "${_fw_link}" SYMBOLIC)
      endforeach()
    endif()
  endif()
endif()

# A music player that cannot play music must not be packageable by accident.
# AOIDE_HAVE_MPV only turns on via pkg-config, the bundled Linux .so, the
# Windows import library plus DLL, or the macOS xcframework, so a configure
# that found none of those used to produce a binary with no engine — which
# then shipped next to libmpv and looked healthy. Opt in explicitly to build
# one, and never opt in for a release.
option(AOIDE_ALLOW_NO_AUDIO "Build without an audio engine (playback disabled)" OFF)
if(NOT AOIDE_HAVE_MPV AND NOT AOIDE_ALLOW_NO_AUDIO)
  message(FATAL_ERROR
    "No audio engine: libmpv was not found, so this build could not play anything.\n"
    "  Linux:   install libmpv development files (pkg-config must find 'mpv'),\n"
    "           or place the bundle at third_party/libmpv/linux/x86_64/libmpv.so.\n"
    "  macOS:   brew install mpv (pkg-config must find 'mpv'), or run\n"
    "           ./tool/fetch_full_libmpv.sh to stage the audio-full xcframeworks\n"
    "           at third_party/libmpv/macos/universal.\n"
    "  Windows: run tool/fetch_full_libmpv.ps1, which installs libmpv-2.dll and\n"
    "           its import library into third_party/libmpv/windows/x86_64.\n"
    "Configure with -DAOIDE_ALLOW_NO_AUDIO=ON to build a deliberately silent binary.")
endif()

set(AOIDE_SOURCES
  src/main.cpp
  src/app_icon.cpp
  src/app_icon.h
  src/host_shell_window.cpp
  src/host_shell_window.h
  src/compositor_keep_above.cpp
  src/compositor_keep_above.h
  src/host_window.cpp
  src/host_window.h
  src/session.cpp
  src/session.h
  src/chrome_command.cpp
  src/chrome_command.h
  src/chrome_anim.cpp
  src/chrome_anim.h
  src/chrome_paint.cpp
  src/chrome_bodies.cpp
  src/chrome_hits.cpp
  src/chrome_tooltip.cpp
  src/chrome_tooltip.h
  src/chrome_menu.cpp
  src/chrome_menu.h
  src/session_view.cpp
  src/mockup_draw.cpp
  src/aoide_fonts.cpp
  src/m3u.cpp
  src/equalizer.cpp
  src/support_dir.cpp
  src/wait_cursor.cpp
  src/playlist.cpp
  src/transport.cpp
  src/settings.cpp
  src/persist.cpp
  src/collection.cpp
  src/duration_probe.cpp
  src/files.cpp
  src/document_portal.cpp
  src/document_portal.h
  src/native_file_dialog.cpp
  src/native_file_dialog.h
  src/native_file_dialog_p.h
  src/playback.cpp
  src/docking.cpp
  src/layout_sync.cpp
  src/layout_sync.h
  src/look.cpp
  src/skin_preview.cpp
  src/wav_reader.cpp
  src/stft.cpp
  src/spectrum.cpp
)
if(AOIDE_HAVE_MPV)
  list(APPEND AOIDE_SOURCES src/mpv_engine.cpp src/mpv_engine.h src/pcm_decoder.cpp)
endif()

if(WIN32)
  set(AOIDE_WIN_ICO "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app_icon.ico")
  configure_file(packaging/windows/aoide.rc.in "${CMAKE_CURRENT_BINARY_DIR}/aoide.rc" @ONLY)
  list(APPEND AOIDE_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/aoide.rc")
endif()
if(APPLE)
  set(AOIDE_MAC_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/aoide.icns")
  set_source_files_properties("${AOIDE_MAC_ICNS}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")
  list(APPEND AOIDE_SOURCES "${AOIDE_MAC_ICNS}")
  # OUTPUT_NAME Aoide makes the bundle Aoide.app — the Mac convention and
  # CFBundleName. packaging/macos/*.sh must use that name, not aoide.app.
  set(AOIDE_MAC_BUNDLE_NAME "Aoide")
  set(AOIDE_MAC_GUI_IDENTIFIER "com.proximamagnifica.aoide")
  set(AOIDE_MAC_ICON_FILE "aoide.icns")
  set(AOIDE_MAC_EXECUTABLE "Aoide")
  set(AOIDE_MAC_COPYRIGHT "Copyright (C) 2026 Proxima Magnifica")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    @ONLY)
endif()
qt_add_executable(aoide WIN32 MACOSX_BUNDLE ${AOIDE_SOURCES})
target_include_directories(aoide PRIVATE src "${AOIDE_MPV_ROOT}/include")
target_compile_definitions(aoide PRIVATE
  AOIDE_VERSION="${PROJECT_VERSION}"
  AOIDE_ASSET_DIR="${AOIDE_ASSET_DIR}"
  AOIDE_SKINS_DIR="${AOIDE_SKINS_DIR}"
)
if(APPLE)
  set_target_properties(aoide PROPERTIES
    OUTPUT_NAME "Aoide"
    MACOSX_BUNDLE_BUNDLE_NAME "Aoide"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.proximamagnifica.aoide"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "aoide.icns"
    MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2026 Proxima Magnifica"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    INSTALL_RPATH "@executable_path/../Frameworks"
  )
  set_property(TARGET aoide APPEND PROPERTY BUILD_RPATH
    "@executable_path/../Frameworks")
  add_custom_command(TARGET aoide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/assets"
      "$<TARGET_BUNDLE_CONTENT_DIR:aoide>/Resources/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/skins"
      "$<TARGET_BUNDLE_CONTENT_DIR:aoide>/Resources/skins"
    VERBATIM)
  if(AOIDE_MAC_MPV_FRAMEWORKS)
    set(_aoide_fw_copy COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_BUNDLE_CONTENT_DIR:aoide>/Frameworks")
    foreach(_fw IN LISTS AOIDE_MAC_MPV_FRAMEWORKS)
      # cp -a, not cmake -E copy_directory: CMake < 3.26 follows framework
      # version symlinks and duplicates the binary, which breaks the bundle.
      list(APPEND _aoide_fw_copy
        COMMAND /bin/cp -a "${_fw}"
          "$<TARGET_BUNDLE_CONTENT_DIR:aoide>/Frameworks/")
    endforeach()
    add_custom_command(TARGET aoide POST_BUILD ${_aoide_fw_copy} VERBATIM)
    set_property(TARGET aoide APPEND PROPERTY BUILD_RPATH
      "${AOIDE_MAC_MPV_RPATH_DIR}")
  endif()
  message(STATUS
    "macOS bundle Aoide.app (min ${CMAKE_OSX_DEPLOYMENT_TARGET}, "
    "arch ${CMAKE_OSX_ARCHITECTURES})")
endif()
if(AOIDE_HAVE_MPV)
  target_compile_definitions(aoide PRIVATE AOIDE_HAVE_MPV)
  target_link_libraries(aoide PRIVATE ${AOIDE_MPV_LIBS})
  if(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}" AND NOT TARGET PkgConfig::MPV)
    target_link_options(aoide PRIVATE "-Wl,--allow-shlib-undefined")
    # BUILD_RPATH, not -Wl,-rpath: CMake strips what it set when it installs, so
    # the shipped binary does not carry this machine's source paths and cannot
    # quietly satisfy itself from the build tree.
    set_property(TARGET aoide APPEND PROPERTY BUILD_RPATH
      "${AOIDE_MPV_ROOT}/linux/x86_64"
      "${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs")
  endif()
endif()
if(UNIX AND NOT APPLE)
  # The bundle keeps every shipped library in lib/ beside the binary, and the
  # tarball is extracted and run in place with nothing to set LD_LIBRARY_PATH.
  # packaging/linux/stage_bundle.sh gives the libraries and plugins the matching
  # $ORIGIN runpaths.
  set_target_properties(aoide PROPERTIES INSTALL_RPATH "$ORIGIN/lib;$ORIGIN")
endif()
target_link_libraries(aoide PRIVATE aoide_window_spec Qt6::Widgets)
if(TARGET Qt6::DBus)
  target_link_libraries(aoide PRIVATE Qt6::DBus)
  target_compile_definitions(aoide PRIVATE AOIDE_HAVE_DBUS)
endif()

qt_add_executable(window_spec_test tests/window_spec_test.cpp)
target_link_libraries(window_spec_test PRIVATE aoide_window_spec Qt6::Test)
set_target_properties(window_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_test tests/host_shell_test.cpp)
target_link_libraries(host_shell_test PRIVATE aoide_window_spec Qt6::Test)
set_target_properties(host_shell_test PROPERTIES AUTOMOC ON)

qt_add_executable(layout_sync_test
  tests/layout_sync_test.cpp
  src/layout_sync.cpp
  src/layout_sync.h
  src/docking.cpp
  src/settings.cpp
  src/equalizer.cpp
)
target_include_directories(layout_sync_test PRIVATE src)
target_link_libraries(layout_sync_test PRIVATE aoide_window_spec Qt6::Test)
set_target_properties(layout_sync_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_window_test
  tests/host_shell_window_test.cpp
  src/host_shell_window.cpp
  src/host_shell_window.h
  src/compositor_keep_above.cpp
  src/app_icon.cpp
  src/aoide_fonts.cpp
)
target_include_directories(host_shell_window_test PRIVATE src)
target_compile_definitions(host_shell_window_test PRIVATE
  AOIDE_ASSET_DIR="${AOIDE_ASSET_DIR}"
)
target_link_libraries(host_shell_window_test PRIVATE aoide_window_spec Qt6::Test Qt6::Widgets)
set_target_properties(host_shell_window_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_window_move_test
  tests/host_window_move_test.cpp
  src/host_shell_window.cpp
  src/host_shell_window.h
  src/compositor_keep_above.cpp
  src/app_icon.cpp
  src/host_window.cpp
  src/host_window.h
  src/chrome_anim.cpp
  src/chrome_paint.cpp
  src/chrome_bodies.cpp
  src/chrome_hits.cpp
  src/chrome_tooltip.cpp
  src/mockup_draw.cpp
  src/aoide_fonts.cpp
  src/session_view.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/wait_cursor.cpp
)
target_include_directories(host_window_move_test PRIVATE src)
target_compile_definitions(host_window_move_test PRIVATE
  AOIDE_ASSET_DIR="${AOIDE_ASSET_DIR}"
  AOIDE_SKINS_DIR="${AOIDE_SKINS_DIR}"
)
target_link_libraries(host_window_move_test PRIVATE aoide_window_spec Qt6::Test Qt6::Widgets)
set_target_properties(host_window_move_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_spec_test tests/chrome_spec_test.cpp src/chrome_anim.cpp)
target_link_libraries(chrome_spec_test PRIVATE aoide_window_spec Qt6::Test)
set_target_properties(chrome_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_command_test
  tests/chrome_command_test.cpp
  src/chrome_command.cpp
  src/playback.cpp
  src/playlist.cpp
  src/transport.cpp
  src/persist.cpp
  src/m3u.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/collection.cpp
  src/docking.cpp
)
target_include_directories(chrome_command_test PRIVATE src)
target_link_libraries(chrome_command_test PRIVATE aoide_window_spec Qt6::Test)
set_target_properties(chrome_command_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_tooltip_test
  tests/chrome_tooltip_test.cpp
  src/chrome_tooltip.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/aoide_fonts.cpp
)
target_include_directories(chrome_tooltip_test PRIVATE src)
target_compile_definitions(chrome_tooltip_test PRIVATE
  AOIDE_ASSET_DIR="${AOIDE_ASSET_DIR}"
  AOIDE_SKINS_DIR="${AOIDE_SKINS_DIR}"
)
target_link_libraries(chrome_tooltip_test PRIVATE aoide_window_spec Qt6::Test Qt6::Widgets)
set_target_properties(chrome_tooltip_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_menu_test tests/chrome_menu_test.cpp)
target_include_directories(chrome_menu_test PRIVATE src)
target_link_libraries(chrome_menu_test PRIVATE Qt6::Test Qt6::Widgets)
set_target_properties(chrome_menu_test PROPERTIES AUTOMOC ON)

qt_add_executable(wait_cursor_test
  tests/wait_cursor_test.cpp
  src/wait_cursor.cpp
)
target_include_directories(wait_cursor_test PRIVATE src)
target_link_libraries(wait_cursor_test PRIVATE Qt6::Test Qt6::Widgets)
set_target_properties(wait_cursor_test PROPERTIES AUTOMOC ON)

qt_add_executable(domain_test
  tests/domain_test.cpp
  src/m3u.cpp
  src/equalizer.cpp
  src/support_dir.cpp
  src/wait_cursor.cpp
  src/playlist.cpp
  src/transport.cpp
  src/wav_reader.cpp
  src/stft.cpp
  src/spectrum.cpp
  src/playback.cpp
  src/docking.cpp
  src/collection.cpp
  src/duration_probe.cpp
  src/persist.cpp
  src/settings.cpp
  src/document_portal.cpp
  src/files.cpp
)
target_include_directories(domain_test PRIVATE src)
target_link_libraries(domain_test PRIVATE aoide_window_spec Qt6::Widgets)
if(TARGET Qt6::DBus)
  target_link_libraries(domain_test PRIVATE Qt6::DBus)
  target_compile_definitions(domain_test PRIVATE AOIDE_HAVE_DBUS)
endif()
if(AOIDE_HAVE_MPV)
  target_compile_definitions(domain_test PRIVATE AOIDE_HAVE_MPV)
  target_include_directories(domain_test PRIVATE "${AOIDE_MPV_ROOT}/include")
  target_link_libraries(domain_test PRIVATE ${AOIDE_MPV_LIBS})
  if(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}" AND NOT TARGET PkgConfig::MPV)
    target_link_options(domain_test PRIVATE
      "-Wl,--allow-shlib-undefined"
      "-Wl,-rpath,${AOIDE_MPV_ROOT}/linux/x86_64"
      "-Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs"
    )
  endif()
  if(APPLE AND AOIDE_MAC_MPV_RPATH_DIR)
    set_property(TARGET domain_test APPEND PROPERTY BUILD_RPATH
      "${AOIDE_MAC_MPV_RPATH_DIR}")
  endif()
endif()

qt_add_executable(look_test
  tests/look_test.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/aoide_fonts.cpp
)
target_include_directories(look_test PRIVATE src)
target_compile_definitions(look_test PRIVATE AOIDE_SKINS_DIR="${AOIDE_SKINS_DIR}")
target_link_libraries(look_test PRIVATE Qt6::Gui)

qt_add_executable(font_metrics_test
  tests/font_metrics_test.cpp
  src/aoide_fonts.cpp
)
target_include_directories(font_metrics_test PRIVATE src)
target_compile_definitions(font_metrics_test PRIVATE
  AOIDE_ASSET_DIR="${AOIDE_ASSET_DIR}"
  AOIDE_SKINS_DIR="${AOIDE_SKINS_DIR}"
)
target_link_libraries(font_metrics_test PRIVATE Qt6::Gui)

foreach(aoide_target IN ITEMS
    aoide_window_spec aoide
    window_spec_test host_shell_test host_shell_window_test host_window_move_test
    layout_sync_test
    chrome_spec_test chrome_command_test chrome_tooltip_test chrome_menu_test wait_cursor_test domain_test
    look_test font_metrics_test)
  target_link_libraries(${aoide_target} PRIVATE aoide_warnings)
endforeach()

# Both binaries carry a static import of libmpv-2.dll, so the Windows loader
# resolves it at process start — before main, before any assertion. Without the
# DLL in the build tree ctest dies with STATUS_DLL_NOT_FOUND (0xC0000135) and
# reports nothing useful. The install rule below covers the package, not this.
if(WIN32 AND AOIDE_HAVE_MPV)
  foreach(win_mpv_target IN ITEMS aoide domain_test)
    add_custom_command(TARGET ${win_mpv_target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${AOIDE_WIN_MPV_DLL}" "$<TARGET_FILE_DIR:${win_mpv_target}>"
      VERBATIM)
  endforeach()
endif()

enable_testing()
add_test(NAME window_spec COMMAND window_spec_test)
add_test(NAME host_shell COMMAND host_shell_test)
add_test(NAME host_shell_window COMMAND host_shell_window_test)
add_test(NAME host_window_move COMMAND host_window_move_test)
add_test(NAME layout_sync COMMAND layout_sync_test)
add_test(NAME chrome_spec COMMAND chrome_spec_test)
add_test(NAME chrome_command COMMAND chrome_command_test)
add_test(NAME chrome_tooltip COMMAND chrome_tooltip_test)
add_test(NAME chrome_menu COMMAND chrome_menu_test)
add_test(NAME wait_cursor COMMAND wait_cursor_test)
add_test(NAME domain COMMAND domain_test)
add_test(NAME look COMMAND look_test)
add_test(NAME font_metrics COMMAND font_metrics_test)

# A test needs a QPA platform iff it constructs something that loads one:
# QTEST_MAIN builds a QApplication, and font_metrics_test builds a
# QGuiApplication by hand. The other six do not — QTEST_APPLESS_MAIN builds no
# application object, QTEST_GUILESS_MAIN builds a QCoreApplication, and
# domain_test / look_test have plain mains — so they need nothing set and are
# left alone. This is the same split build.sh makes, for the same reason.
#
# Offscreen rather than the default because ctest has to be right in both
# directions. On a headless runner the default is xcb with no display to connect
# to, and the test aborts instead of running; on a developer's desktop the
# default is a real compositor that ignores the geometry these tests assert, and
# host_window_move fails for reasons that have nothing to do with the code. Set
# here rather than in the workflow so `ctest` is the same check everywhere.
set_tests_properties(host_shell_window host_window_move chrome_tooltip wait_cursor font_metrics
  PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

if(APPLE)
  # BUNDLE copies the built .app, which already has Resources and Frameworks
  # from MACOSX_PACKAGE_LOCATION / POST_BUILD. Installing assets/ to `.` here
  # would put them next to the .app, which is where bundledDataDir does not look.
  install(TARGETS aoide BUNDLE DESTINATION .)
else()
  install(TARGETS aoide RUNTIME DESTINATION .)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/skins" DESTINATION .)
endif()
if(UNIX AND NOT APPLE)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/com.proximamagnifica.aoide.desktop"
    DESTINATION share/applications)
  # The name and summary every Linux installer shows. A .desktop file's Name= does
  # not reach flatpak, GNOME Software or Discover; this is the file they read, and
  # without it they fall back to printing the app ID.
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/com.proximamagnifica.aoide.metainfo.xml"
    DESTINATION share/metainfo)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/icons/hicolor"
    DESTINATION share/icons)
  file(GLOB AOIDE_LINUX_MPV "${AOIDE_MPV_ROOT}/linux/x86_64/libmpv.so*")
  if(AOIDE_LINUX_MPV)
    install(FILES ${AOIDE_LINUX_MPV} DESTINATION lib)
  endif()
endif()
if(WIN32 AND EXISTS "${AOIDE_WIN_MPV_DLL}")
  install(FILES "${AOIDE_WIN_MPV_DLL}" DESTINATION .)
endif()
