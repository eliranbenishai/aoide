cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TRAMP_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${TRAMP_VERSION_RAW}" TRAMP_VERSION)
project(tramp VERSION ${TRAMP_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(PkgConfig QUIET)

qt_standard_project_setup()

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
endif()

set(TRAMP_SOURCES
  src/main.cpp
  src/app_icon.cpp
  src/app_icon.h
  src/host_shell_window.cpp
  src/host_shell_window.h
  src/host_window.cpp
  src/host_window.h
  src/session.cpp
  src/session.h
  src/chrome_paint.cpp
  src/chrome_bodies.cpp
  src/chrome_hits.cpp
  src/session_view.cpp
  src/mockup_draw.cpp
  src/tramp_fonts.cpp
  src/m3u.cpp
  src/equalizer.cpp
  src/support_dir.cpp
  src/playlist.cpp
  src/transport.cpp
  src/settings.cpp
  src/persist.cpp
  src/collection.cpp
  src/files.cpp
  src/playback.cpp
  src/docking.cpp
  src/look.cpp
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
    target_link_options(tramp PRIVATE
      "-Wl,--allow-shlib-undefined"
      "-Wl,-rpath,${TRAMP_MPV_ROOT}/linux/x86_64"
      "-Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}/mpv-stubs"
    )
  endif()
endif()
target_link_libraries(tramp PRIVATE tramp_window_spec Qt6::Widgets)

qt_add_executable(window_spec_test tests/window_spec_test.cpp)
target_link_libraries(window_spec_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(window_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_test tests/host_shell_test.cpp)
target_link_libraries(host_shell_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(host_shell_test PROPERTIES AUTOMOC ON)

qt_add_executable(host_shell_window_test
  tests/host_shell_window_test.cpp
  src/host_shell_window.cpp
  src/host_shell_window.h
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
  src/app_icon.cpp
  src/host_window.cpp
  src/host_window.h
  src/chrome_paint.cpp
  src/chrome_bodies.cpp
  src/chrome_hits.cpp
  src/mockup_draw.cpp
  src/tramp_fonts.cpp
  src/session_view.cpp
  src/look.cpp
  src/settings.cpp
  src/equalizer.cpp
)
target_include_directories(host_window_move_test PRIVATE src)
target_compile_definitions(host_window_move_test PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
  TRAMP_SKINS_DIR="${TRAMP_SKINS_DIR}"
)
target_link_libraries(host_window_move_test PRIVATE tramp_window_spec Qt6::Test Qt6::Widgets)
set_target_properties(host_window_move_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_spec_test tests/chrome_spec_test.cpp)
target_link_libraries(chrome_spec_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(chrome_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(domain_test
  tests/domain_test.cpp
  src/m3u.cpp
  src/equalizer.cpp
  src/support_dir.cpp
  src/playlist.cpp
  src/transport.cpp
  src/wav_reader.cpp
  src/stft.cpp
  src/spectrum.cpp
  src/playback.cpp
  src/docking.cpp
  src/collection.cpp
  src/persist.cpp
  src/settings.cpp
)
target_include_directories(domain_test PRIVATE src)
target_link_libraries(domain_test PRIVATE tramp_window_spec Qt6::Widgets)

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

enable_testing()
add_test(NAME window_spec COMMAND window_spec_test)
add_test(NAME host_shell COMMAND host_shell_test)
add_test(NAME host_shell_window COMMAND host_shell_window_test)
add_test(NAME host_window_move COMMAND host_window_move_test)
add_test(NAME chrome_spec COMMAND chrome_spec_test)
add_test(NAME domain COMMAND domain_test)
add_test(NAME look COMMAND look_test)
add_test(NAME font_metrics COMMAND font_metrics_test)

install(TARGETS tramp RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/skins" DESTINATION .)
if(UNIX AND NOT APPLE)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/com.tramp.tramp.desktop"
    DESTINATION share/applications)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/icons/hicolor"
    DESTINATION share/icons)
  file(GLOB TRAMP_LINUX_MPV "${TRAMP_MPV_ROOT}/linux/x86_64/libmpv.so*")
  if(TRAMP_LINUX_MPV)
    install(FILES ${TRAMP_LINUX_MPV} DESTINATION lib)
  endif()
endif()
if(WIN32)
  set(TRAMP_WIN_MPV_DLL "${TRAMP_MPV_ROOT}/windows/x86_64/libmpv-2.dll")
  if(EXISTS "${TRAMP_WIN_MPV_DLL}")
    install(FILES "${TRAMP_WIN_MPV_DLL}" DESTINATION .)
  endif()
endif()
