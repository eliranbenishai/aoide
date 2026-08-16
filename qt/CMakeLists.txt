cmake_minimum_required(VERSION 3.16)
project(tramp-qt-tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

qt_standard_project_setup()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../assets" TRAMP_ASSET_DIR)

add_library(tramp_window_spec STATIC
  src/window_spec.cpp
  src/title_chrome.cpp
)
target_include_directories(tramp_window_spec PUBLIC src)
target_link_libraries(tramp_window_spec PUBLIC Qt6::Widgets)

set(TRAMP_SKIP_TASKBAR_SRC src/skip_taskbar.cpp)
qt_add_executable(tramp-qt-tracer WIN32
  src/main.cpp
  src/host_window.cpp
  src/host_window.h
  src/chrome_paint.cpp
  src/chrome_bodies.cpp
  src/mockup_draw.cpp
  src/tramp_fonts.cpp
  ${TRAMP_SKIP_TASKBAR_SRC}
)
target_compile_definitions(tramp-qt-tracer PRIVATE
  TRAMP_ASSET_DIR="${TRAMP_ASSET_DIR}"
)
target_link_libraries(tramp-qt-tracer PRIVATE tramp_window_spec Qt6::Widgets)

if(UNIX AND NOT APPLE)
  find_package(X11)
  if(X11_FOUND)
    target_compile_definitions(tramp-qt-tracer PRIVATE TRAMP_HAVE_X11)
    target_link_libraries(tramp-qt-tracer PRIVATE X11::X11)
  endif()
endif()

qt_add_executable(window_spec_test tests/window_spec_test.cpp)
target_link_libraries(window_spec_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(window_spec_test PROPERTIES AUTOMOC ON)

qt_add_executable(chrome_spec_test tests/chrome_spec_test.cpp)
target_link_libraries(chrome_spec_test PRIVATE tramp_window_spec Qt6::Test)
set_target_properties(chrome_spec_test PROPERTIES AUTOMOC ON)

enable_testing()
add_test(NAME window_spec COMMAND window_spec_test)
add_test(NAME chrome_spec COMMAND chrome_spec_test)
