#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include <windows.h>

#include <stdio.h>
#include <string>

#include "flutter_window.h"
#include "utils.h"

static void LogStartup(const char* message) {
  wchar_t dir[MAX_PATH];
  if (::GetTempPathW(MAX_PATH, dir) == 0) {
    return;
  }
  std::wstring path(dir);
  path += L"tramp-startup.log";
  FILE* file = nullptr;
  if (_wfopen_s(&file, path.c_str(), L"a") != 0 || file == nullptr) {
    return;
  }
  fprintf(file, "%s\n", message);
  fclose(file);
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command) {
  // Attach to console when present (e.g., 'flutter run') or create a
  // new console when running with a debugger.
  if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::IsDebuggerPresent()) {
    CreateAndAttachConsole();
  }

  LogStartup("wWinMain");

  // Initialize COM, so that it is available for use in the library and/or
  // plugins.
  ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  flutter::DartProject project(L"data");
  // Flutter 3.47 defaults Impeller on for desktop. Five engines + this GPU
  // path has been a silent-exit on some Windows boxes; Skia is the golden path.
  project.set_impeller_switch(flutter::ImpellerSwitch::Disabled);

  std::vector<std::string> command_line_arguments =
      GetCommandLineArguments();

  project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

  FlutterWindow window(project);
  Win32Window::Point origin(10, 10);
  Win32Window::Size size(1280, 720);
  if (!window.Create(L"tramp", origin, size)) {
    LogStartup("Create failed");
    ::MessageBoxW(nullptr,
                  L"Tramp could not create its window.\n\n"
                  L"See %TEMP%\\tramp-startup.log and Event Viewer "
                  L"(Windows Logs → Application) for tramp.exe.",
                  L"Tramp", MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }
  LogStartup("Create ok");
  window.SetQuitOnClose(true);

  ::MSG msg;
  while (::GetMessage(&msg, nullptr, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  ::CoUninitialize();
  return EXIT_SUCCESS;
}
