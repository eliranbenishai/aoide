#include "multi_window_manager.h"

#include <iomanip>
#include <random>
#include <sstream>

#include "desktop_multi_window_plugin_internal.h"
#include "flutter_window.h"
#include "include/desktop_multi_window/desktop_multi_window_plugin.h"
#include "window_configuration.h"
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

namespace {

std::string GenerateWindowId() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;

  uint64_t part1 = dis(gen);
  uint64_t part2 = dis(gen);

  part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
  part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

  char uuid_str[37];
  snprintf(uuid_str, sizeof(uuid_str), "%08x-%04x-%04x-%04x-%012llx",
           static_cast<uint32_t>(part1 >> 32),
           static_cast<uint16_t>(part1 >> 16), static_cast<uint16_t>(part1),
           static_cast<uint16_t>(part2 >> 48), part2 & 0xFFFFFFFFFFFFULL);

  return std::string(uuid_str);
}

WindowCreatedCallback _g_window_created_callback = nullptr;

// 75% of TrampMetrics canvases, rounded — TrampMetrics.nativeUnmappedSeed.
// About/settings are smaller than the EQ/main 619×261 seed; using that seed
// for them leaves a black FlView rectangle around the chrome.
void tramp_secondary_seed_size(const std::string& arguments, int* width,
                               int* height) {
  *width = 619;
  *height = 261;
  if (arguments.find("\"role\":\"about\"") != std::string::npos) {
    *width = 360;
    *height = 270;
  } else if (arguments.find("\"role\":\"settings\"") != std::string::npos) {
    *width = 390;
    *height = 315;
  } else if (arguments.find("\"role\":\"playlist\"") != std::string::npos) {
    *width = 805;
    *height = 522;
  }
}

}  // namespace

// static
MultiWindowManager* MultiWindowManager::Instance() {
  static auto manager = std::make_shared<MultiWindowManager>();
  return manager.get();
}

MultiWindowManager::MultiWindowManager() : windows_() {}

MultiWindowManager::~MultiWindowManager() = default;

std::string MultiWindowManager::Create(FlValue* args) {
  WindowConfiguration config = WindowConfiguration::FromFlValue(args);
  std::string window_id = GenerateWindowId();

  // Create GTK window
  GtkApplication* app = GTK_APPLICATION(g_application_get_default());
  GtkWindow* window = GTK_WINDOW(gtk_application_window_new(app));
  gtk_application_add_window(app, window);

  // Tramp: secondaries are frameless (window_manager). Skip GTK header bars
  // and the template 1280×720 default — that left a huge black FlView with
  // chrome stuck in the corner when Dart setSize raced/flaked under Distrobox.
  gtk_window_set_title(window, "");
  int seed_w = 619;
  int seed_h = 261;
  tramp_secondary_seed_size(config.arguments, &seed_w, &seed_h);
  gtk_window_set_default_size(window, seed_w, seed_h);

  gtk_window_set_title(window, "");
  if (config.hidden_at_launch) {
    gtk_widget_realize(GTK_WIDGET(window));
  } else {
    gtk_widget_show(GTK_WIDGET(window));
  }

  // Create FlutterWindow instance
  auto w = std::make_unique<FlutterWindow>(window_id, config.arguments,
                                           GTK_WIDGET(window));
  windows_[window_id] = std::move(w);

  // Setup Flutter project
  g_autoptr(FlDartProject) project = fl_dart_project_new();
  const char* entrypoint_args[] = {"multi_window", window_id.c_str(),
                                   config.arguments.c_str(), nullptr};
  fl_dart_project_set_dart_entrypoint_arguments(
      project, const_cast<char**>(entrypoint_args));

  // Create Flutter view
  auto fl_view = fl_view_new(project);
  GdkRGBA background_color;
  // Transparent so MockupShell rounded corners punch through (matches
  // window_manager.setBackgroundColor / the main-window FlView).
  gdk_rgba_parse(&background_color, "#00000000");
  fl_view_set_background_color(fl_view, &background_color);
  gtk_widget_show(GTK_WIDGET(fl_view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(fl_view));

  // Issues from flutter/engine: https://github.com/flutter/engine/pull/40033
  // Prevent delete-event from flutter engine shell, which will quit the whole
  // appplication when the window is closed. this can be done by
  // [window_manager] plugin, but we need it here if user is not using that
  // plugin.
  guint handler_id = g_signal_handler_find(window, G_SIGNAL_MATCH_DATA, 0, 0,
                                           NULL, NULL, fl_view);
  if (handler_id > 0) {
    g_signal_handler_disconnect(window, handler_id);
  }

  // Call window created callback
  if (_g_window_created_callback) {
    _g_window_created_callback(FL_PLUGIN_REGISTRY(fl_view));
  }

  ObserveWindowClose(window_id, window);
  // Register plugin
  g_autoptr(FlPluginRegistrar) desktop_multi_window_registrar =
      fl_plugin_registry_get_registrar_for_plugin(FL_PLUGIN_REGISTRY(fl_view),
                                                  "DesktopMultiWindowPlugin");

  desktop_multi_window_plugin_register_with_registrar_internal(
      desktop_multi_window_registrar, windows_[window_id].get());

  gtk_widget_grab_focus(GTK_WIDGET(fl_view));

  // Notify all windows about the change
  NotifyWindowsChanged();

  return window_id;
}

void MultiWindowManager::AttachMainWindow(GtkWidget* window_widget,
                                          FlPluginRegistrar* registrar) {
  // check window widget is in windows_
  for (const auto& pair : windows_) {
    if (pair.second->GetWindow() == GTK_WINDOW(window_widget)) {
      return;
    }
  }

  const std::string main_window_id = GenerateWindowId();
  auto window =
      std::make_unique<FlutterWindow>(main_window_id, "", window_widget);
  windows_[main_window_id] = std::move(window);

  ObserveWindowClose(main_window_id, GTK_WINDOW(window_widget));
  desktop_multi_window_plugin_register_with_registrar_internal(
      registrar, windows_[main_window_id].get());

  // Notify all windows about the change
  NotifyWindowsChanged();
}

void MultiWindowManager::ObserveWindowClose(const std::string& window_id,
                                            GtkWindow* window) {
  g_signal_connect(
      GTK_WIDGET(window), "destroy",
      G_CALLBACK(+[](GtkWidget* widget, gpointer arg) {
        auto* window_id_ptr = static_cast<std::string*>(arg);

        GtkWidget* child = gtk_bin_get_child(GTK_BIN(widget));
        if (child && FL_IS_VIEW(child)) {
          gtk_container_remove(GTK_CONTAINER(widget), child);
        }

        MultiWindowManager::Instance()->RemoveWindow(*window_id_ptr);
        delete window_id_ptr;
      }),
      new std::string(window_id));
}

FlutterWindow* MultiWindowManager::GetWindow(const std::string& window_id) {
  auto it = windows_.find(window_id);
  if (it != windows_.end()) {
    return it->second.get();
  }
  return nullptr;
}

FlValue* MultiWindowManager::GetAllWindows() {
  g_autoptr(FlValue) windows = fl_value_new_list();
  for (const auto& pair : windows_) {
    g_autoptr(FlValue) window_info = fl_value_new_map();
    fl_value_set_string_take(
        window_info, "windowId",
        fl_value_new_string(pair.second->GetWindowId().c_str()));
    fl_value_set_string_take(
        window_info, "windowArgument",
        fl_value_new_string(pair.second->GetWindowArgument().c_str()));
    fl_value_append_take(windows, fl_value_ref(window_info));
  }
  return fl_value_ref(windows);
}

std::vector<std::string> MultiWindowManager::GetAllWindowIds() {
  std::vector<std::string> window_ids;
  for (const auto& pair : windows_) {
    window_ids.push_back(pair.first);
  }
  return window_ids;
}

void MultiWindowManager::NotifyWindowsChanged() {
  auto window_ids = GetAllWindowIds();

  g_autoptr(FlValue) window_ids_list = fl_value_new_list();
  for (const auto& id : window_ids) {
    fl_value_append_take(window_ids_list, fl_value_new_string(id.c_str()));
  }

  g_autoptr(FlValue) data = fl_value_new_map();
  fl_value_set_string_take(data, "windowIds", fl_value_ref(window_ids_list));

  for (const auto& pair : windows_) {
    pair.second->NotifyWindowEvent("onWindowsChanged", data);
  }
}

void MultiWindowManager::RemoveWindow(const std::string& window_id) {
  g_warning("RemoveWindow: %s", window_id.c_str());
  windows_.erase(window_id);
  NotifyWindowsChanged();
}

void desktop_multi_window_plugin_set_window_created_callback(
    WindowCreatedCallback callback) {
  _g_window_created_callback = callback;
}
