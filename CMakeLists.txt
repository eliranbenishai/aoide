cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TRAMP_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${TRAMP_VERSION_RAW}" TRAMP_VERSION)
project(tramp VERSION ${TRAMP_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chrome is rasterised on the CPU every frame, so an unoptimised build does not
# just run slower — it drags at a few frames per second. Default to Release so
# a plain `cmake -S . -B build` never produces a build that feels broken.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/QT_VERSION" TRAMP_QT_VERSION LIMIT_COUNT 1)
string(STRIP "${TRAMP_QT_VERSION}" TRAMP_QT_VERSION)
if(UNIX AND NOT APPLE)
  set(_tramp_qt_pin "${CMAKE_CURRENT_SOURCE_DIR}/.local/qt/${TRAMP_QT_VERSION}/gcc_64")
  if(EXISTS "${_tramp_qt_pin}/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${_tramp_qt_pin}")
  endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Qt6 QUIET COMPONENTS DBus)
if(NOT Qt6_VERSION VERSION_EQUAL TRAMP_QT_VERSION)
  message(FATAL_ERROR
    "Tramp is pinned to Qt ${TRAMP_QT_VERSION} (QT_VERSION). Found ${Qt6_VERSION}. "
    "On Linux run ./tool/fetch_qt.sh and reconfigure.")
endif()
message(STATUS "Qt ${Qt6_VERSION} matches QT_VERSION")
find_package(PkgConfig QUIET)

qt_standard_project_setup()

# CI compiles through CMake, not build.sh, so this path has to raise the same
# diagnostics build.sh does — ignored return values and signed/unsigned
# conversions are how the worst bugs here got in. Warnings only: the tree is not
# clean, so -Werror would fail on day one. Linked privately into our own targets;
# Qt arrives as -isystem and stays quiet.
add_library(tramp_warnings INTERFACE)
if(MSVC)
  target_compile_options(tramp_warnings INTERFACE /W4 /wd4100 /w14834)
else()
  target_compile_options(tramp_warnings INTERFACE
    -Wall -Wextra -Wno-unused-parameter
    -Wunused-result -Wsign-compare -Wsign-conversion)
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets" TRAMP_ASSET_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/skins" TRAMP_SKINS_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libmpv" TRAMP_MPV_ROOT)

add_library(tramp_window_spec STATIC
  src/window_spec.cpp
  src/host_shell.cpp
  src/title_chrome.cpp
)
target_include_directories(tramp_window_spec PUBLIC src)
target_link_libraries(tramp_window_spec PUBLIC Qt6::Widgets)

set(TRAMP_HAVE_MPV OFF)
set(TRAMP_MPV_LIBS "")
set(BUNDLED_MPV_SO "${TRAMP_MPV_ROOT}/linux/x86_64/libmpv.so")
set(BUNDLED_MPV_IMPLIB "${TRAMP_MPV_ROOT}/windows/x86_64/libmpv.dll.a")
set(TRAMP_WIN_MPV_DLL "${TRAMP_MPV_ROOT}/windows/x86_64/libmpv-2.dll")

if(PkgConfig_FOUND)
  pkg_check_modules(MPV IMPORTED_TARGET mpv)
endif()

if(TARGET PkgConfig::MPV)
  set(TRAMP_HAVE_MPV ON)
  set(TRAMP_MPV_LIBS PkgConfig::MPV)
elseif(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}")
  set(TRAMP_HAVE_MPV ON)
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
  list(APPEND TRAMP_MPV_LIBS
    "${BUNDLED_MPV_SO}"
    stub_libmujs.so.0.1
    stub_liblua-5.1.so
    stub_libuchardet.so.0
    stub_libvapoursynth-script.so.0
    stub_libXpresent.so.1
  )
elseif(WIN32 AND EXISTS "${BUNDLED_MPV_IMPLIB}" AND EXISTS "${TRAMP_WIN_MPV_DLL}")
  # The pinned shinchiro archive names its import library GNU-style, but the
  # file is a Microsoft short-import library — the format lib.exe emits. link.exe
  # identifies archives by content rather than extension, so the copy to a
  # conventional name is only to keep the toolchain unsurprised.
  #
  # Both files are required: the import library is what links, the DLL is what
  # ships, and a build that can link but not ship is the silent-Windows defect
  # this branch exists to close.
  set(TRAMP_HAVE_MPV ON)
  set(TRAMP_MPV_IMPLIB "${CMAKE_CURRENT_BINARY_DIR}/mpv.lib")
  configure_file("${BUNDLED_MPV_IMPLIB}" "${TRAMP_MPV_IMPLIB}" COPYONLY)
  list(APPEND TRAMP_MPV_LIBS "${TRAMP_MPV_IMPLIB}")
endif()

# A music player that cannot play music must not be packageable by accident.
# TRAMP_HAVE_MPV only turns on via pkg-config or the bundled Linux .so, so a
# Windows configure silently produced a binary with no engine — which then
# shipped next to libmpv-2.dll and looked healthy. Opt in explicitly to build
# one, and never opt in for a release.
option(TRAMP_ALLOW_NO_AUDIO "Build without an audio engine (playback disabled)" OFF)
if(NOT TRAMP_HAVE_MPV AND NOT TRAMP_ALLOW_NO_AUDIO)
  message(FATAL_ERROR
    "No audio engine: libmpv was not found, so this build could not play anything.\n"
    "  Linux/macOS: install libmpv development files (pkg-config must find 'mpv'),\n"
    "               or place the bundle at third_party/libmpv/linux/x86_64/libmpv.so.\n"
    "  Windows:     run tool/fetch_full_libmpv.ps1, which installs libmpv-2.dll and\n"
    "               its import library into third_party/libmpv/windows/x86_64.\n"
    "Configure with -DTRAMP_ALLOW_NO_AUDIO=ON to build a deliberately silent binary.")
endif()

set(TRAMP_SOURCES
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
  src/tramp_fonts.cpp
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
if(TRAMP_HAVE_MPV)
  list(APPEND TRAMP_SOURCES src/mpv_engine.cpp src/mpv_engine.h src/pcm_decoder.cpp)
endif()

if(WIN32)
  set(TRAMP_WIN_ICO "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app_icon.ico")
  configure_file(packaging/windows/tramp.rc.in "${CMAKE_CURRENT_BINARY_DIR}/tramp.rc" @ONLY)
  list(APPEND TRAMP_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/tramp.rc")
endif()
qt_add_executable(tramp WIN32 ${TRAMP_SOURCES})
target_include_directories(tramp PRIVATE src "${TRAMP_MPV_ROOT}/include")
target_compile_definitions(tramp PRIVATE
  TRAMP_VERSION="${PROJECT_VERSION}"
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
  TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}"
)
if(TRAMP_HAVE_MPV)
  target_compile_definitions(tramp PRIVATE TRAMP_HAVE_MPV)
  target_link_libraries(tramp PRIVATE ${TRAMP_MPV_LIBS})
  if(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}" AND NOT TARGET PkgConfig::MPV)
    target_link_options(tramp PRIVATE "-Wl,--allow-shlib-undefined")
    # BUILD_RPATH, not -Wl,-rpath: CMake strips what it set when it installs, so
    # the shipped binary does not carry this machine's source paths and cannot
    # quietly satisfy itself from the build tree.
    set_property(TARGET tramp APPEND PROPERTY BUILD_RPATH
      "${TRAMP_MPV_ROOT}/linux/x86_64"
      "${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs")
  endif()
endif()
if(UNIX AND NOT APPLE)
  # The bundle keeps every shipped library in lib/ beside the binary, and the
  # tarball is extracted and run in place with nothing to set LD_LIBRARY_PATH.
  # packaging/linux/stage_bundle.sh gives the libraries and plugins the matching
  # $ORIGIN runpaths.
  set_target_properties(tramp PROPERTIES INSTALL_RPATH "$ORIGIN/lib;$ORIGIN")
endif()
target_link_libraries(tramp PRIVATE tramp_window_spec Qt6::Widgets)
if(TARGET Qt6::DBus)
  target_link_libraries(tramp PRIVATE Qt6::DBus)
  target_compile_definitions(tramp PRIVATE TRAMP_HAVE_DBUS)
endif()

qt_add_executable(window_spec_test tests/window_spec_test.cpp)
target_link_libraries(window_spec_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(window_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_test tests/host_shell_test.cpp)
target_link_libraries(host_shell_test PRIVATE tramp_window_spec Qt6::Test)
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
target_link_libraries(layout_sync_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(layout_sync_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_window_test
  tests/host_shell_window_test.cpp
  src/host_shell_window.cpp
  src/host_shell_window.h
  src/compositor_keep_above.cpp
  src/app_icon.cpp
  src/tramp_fonts.cpp
)
target_include_directories(host_shell_window_test PRIVATE src)
target_compile_definitions(host_shell_window_test PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
)
target_link_libraries(host_shell_window_test PRIVATE tramp_window_spec Qt6::Test Qt6::Widgets)
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
  src/tramp_fonts.cpp
  src/session_view.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/wait_cursor.cpp
)
target_include_directories(host_window_move_test PRIVATE src)
target_compile_definitions(host_window_move_test PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
  TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}"
)
target_link_libraries(host_window_move_test PRIVATE tramp_window_spec Qt6::Test Qt6::Widgets)
set_target_properties(host_window_move_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_spec_test tests/chrome_spec_test.cpp src/chrome_anim.cpp)
target_link_libraries(chrome_spec_test PRIVATE tramp_window_spec Qt6::Test)
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
target_link_libraries(chrome_command_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(chrome_command_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_tooltip_test
  tests/chrome_tooltip_test.cpp
  src/chrome_tooltip.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/tramp_fonts.cpp
)
target_include_directories(chrome_tooltip_test PRIVATE src)
target_compile_definitions(chrome_tooltip_test PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
  TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}"
)
target_link_libraries(chrome_tooltip_test PRIVATE tramp_window_spec Qt6::Test Qt6::Widgets)
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
)
target_include_directories(domain_test PRIVATE src)
target_link_libraries(domain_test PRIVATE tramp_window_spec Qt6::Widgets)
if(TRAMP_HAVE_MPV)
  target_compile_definitions(domain_test PRIVATE TRAMP_HAVE_MPV)
  target_include_directories(domain_test PRIVATE "${TRAMP_MPV_ROOT}/include")
  target_link_libraries(domain_test PRIVATE ${TRAMP_MPV_LIBS})
  if(UNIX AND NOT APPLE AND EXISTS "${BUNDLED_MPV_SO}" AND NOT TARGET PkgConfig::MPV)
    target_link_options(domain_test PRIVATE
      "-Wl,--allow-shlib-undefined"
      "-Wl,-rpath,${TRAMP_MPV_ROOT}/linux/x86_64"
      "-Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs"
    )
  endif()
endif()

qt_add_executable(look_test
  tests/look_test.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
  src/tramp_fonts.cpp
)
target_include_directories(look_test PRIVATE src)
target_compile_definitions(look_test PRIVATE TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}")
target_link_libraries(look_test PRIVATE Qt6::Gui)

qt_add_executable(font_metrics_test
  tests/font_metrics_test.cpp
  src/tramp_fonts.cpp
)
target_include_directories(font_metrics_test PRIVATE src)
target_compile_definitions(font_metrics_test PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
  TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}"
)
target_link_libraries(font_metrics_test PRIVATE Qt6::Gui)

foreach(tramp_target IN ITEMS
    tramp_window_spec tramp
    window_spec_test host_shell_test host_shell_window_test host_window_move_test
    layout_sync_test
    chrome_spec_test chrome_command_test chrome_tooltip_test chrome_menu_test wait_cursor_test domain_test
    look_test font_metrics_test)
  target_link_libraries(${tramp_target} PRIVATE tramp_warnings)
endforeach()

# Both binaries carry a static import of libmpv-2.dll, so the Windows loader
# resolves it at process start — before main, before any assertion. Without the
# DLL in the build tree ctest dies with STATUS_DLL_NOT_FOUND (0xC0000135) and
# reports nothing useful. The install rule below covers the package, not this.
if(WIN32 AND TRAMP_HAVE_MPV)
  foreach(win_mpv_target IN ITEMS tramp domain_test)
    add_custom_command(TARGET ${win_mpv_target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TRAMP_WIN_MPV_DLL}" "$<TARGET_FILE_DIR:${win_mpv_target}>"
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

install(TARGETS tramp RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/skins" DESTINATION .)
if(UNIX AND NOT APPLE)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/com.proximamagnifica.tramp.desktop"
    DESTINATION share/applications)
  # The name and summary every Linux installer shows. A .desktop file's Name= does
  # not reach flatpak, GNOME Software or Discover; this is the file they read, and
  # without it they fall back to printing the app ID.
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/com.proximamagnifica.tramp.metainfo.xml"
    DESTINATION share/metainfo)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/icons/hicolor"
    DESTINATION share/icons)
  file(GLOB TRAMP_LINUX_MPV "${TRAMP_MPV_ROOT}/linux/x86_64/libmpv.so*")
  if(TRAMP_LINUX_MPV)
    install(FILES ${TRAMP_LINUX_MPV} DESTINATION lib)
  endif()
endif()
if(WIN32 AND EXISTS "${TRAMP_WIN_MPV_DLL}")
  install(FILES "${TRAMP_WIN_MPV_DLL}" DESTINATION .)
endif()
