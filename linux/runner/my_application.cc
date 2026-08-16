#include "my_application.h"

#include <cstdlib>
#include <cstring>

#include <flutter_linux/flutter_linux.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"
#include "desktop_multi_window/desktop_multi_window_plugin.h"

struct _MyApplication {
  GtkApplication parent_instance;
  char** dart_entrypoint_arguments;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

// Prefer the bundled Tramp logo over the generic Flutter/GTK icon in the
// taskbar and alt-tab switcher. Paths are relative to the relocatable bundle.
static void tramp_apply_window_icon(GtkWindow* window) {
  g_autofree gchar* exe = g_file_read_link("/proc/self/exe", nullptr);
  if (exe == nullptr) {
    return;
  }
  g_autofree gchar* dir = g_path_get_dirname(exe);
  const gchar* candidates[] = {
      "data/app_icon.png",
      "data/flutter_assets/assets/branding/app_icon.png",
      nullptr,
  };
  for (int i = 0; candidates[i] != nullptr; ++i) {
    g_autofree gchar* path = g_build_filename(dir, candidates[i], nullptr);
    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
      continue;
    }
    g_autoptr(GError) error = nullptr;
    if (!gtk_window_set_icon_from_file(window, path, &error)) {
      g_warning("Failed to set window icon from %s: %s", path,
                error != nullptr ? error->message : "unknown");
      continue;
    }
    gtk_window_set_default_icon_from_file(path, nullptr);
    return;
  }
}

// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  GtkWindow* window =
      GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));

  tramp_apply_window_icon(window);

  // Use a header bar when running in GNOME as this is the common style used
  // by applications and is the setup most users will be using (e.g. Ubuntu
  // desktop).
  // If running on X and not using GNOME then just use a traditional title bar
  // in case the window manager does more exotic layout, e.g. tiling.
  // If running on Wayland assume the header bar will work (may need changing
  // if future cases occur).
  gboolean use_header_bar = TRUE;
#ifdef GDK_WINDOWING_X11
  GdkScreen* screen = gtk_window_get_screen(window);
  if (GDK_IS_X11_SCREEN(screen)) {
    const gchar* wm_name = gdk_x11_screen_get_window_manager_name(screen);
    if (g_strcmp0(wm_name, "GNOME Shell") != 0) {
      use_header_bar = FALSE;
    }
  }
#endif
  if (use_header_bar) {
    GtkHeaderBar* header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
    gtk_widget_show(GTK_WIDGET(header_bar));
    gtk_header_bar_set_title(header_bar, "tramp");
    gtk_header_bar_set_show_close_button(header_bar, TRUE);
    gtk_window_set_titlebar(window, GTK_WIDGET(header_bar));
  } else {
    gtk_window_set_title(window, "tramp");
  }

  // Default product zoom is 75% of the main canvas (825×348). Avoid the
  // Flutter template 1280×720 default — if Dart setSize is delayed or flakes
  // on Linux, that leaves a huge black FlView with chrome only in the corner.
  gtk_window_set_default_size(window, 619, 261);
  gtk_widget_hide(GTK_WIDGET(window));

  g_autoptr(FlDartProject) project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(
      project, self->dart_entrypoint_arguments);

  FlView* view = fl_view_new(project);
  GdkRGBA background_color;
  // Transparent so MockupShell rounded corners punch through (matches
  // window_manager.setBackgroundColor in SessionHostApp).
  // TRAMP_OPAQUE_WINDOWS=1 forces an opaque fill to isolate compositor drag.
  const char* opaque = getenv("TRAMP_OPAQUE_WINDOWS");
  if (opaque != nullptr && strcmp(opaque, "1") == 0) {
    gdk_rgba_parse(&background_color, "#1a1a1a");
  } else {
    gdk_rgba_parse(&background_color, "#00000000");
  }
  fl_view_set_background_color(view, &background_color);
  gtk_widget_show(GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  // Realize so Flutter can render into a hidden window. Dart shows the
  // toplevel after session chrome is ready (do not map on first-frame).
  gtk_widget_realize(GTK_WIDGET(view));

  fl_register_plugins(FL_PLUGIN_REGISTRY(view));

  desktop_multi_window_plugin_set_window_created_callback(
      [](FlPluginRegistry* registry) { fl_register_plugins(registry); });

  gtk_widget_grab_focus(GTK_WIDGET(view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application,
                                                  gchar*** arguments,
                                                  int* exit_status) {
  MyApplication* self = MY_APPLICATION(application);
  // Strip out the first argument as it is the binary name.
  self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

  g_autoptr(GError) error = nullptr;
  if (!g_application_register(application, nullptr, &error)) {
    g_warning("Failed to register: %s", error->message);
    *exit_status = 1;
    return TRUE;
  }

  g_application_activate(application);
  *exit_status = 0;

  return TRUE;
}

// Implements GApplication::startup.
static void my_application_startup(GApplication* application) {
  // MyApplication* self = MY_APPLICATION(object);

  // Perform any actions required at application startup.

  G_APPLICATION_CLASS(my_application_parent_class)->startup(application);
}

// Implements GApplication::shutdown.
static void my_application_shutdown(GApplication* application) {
  // MyApplication* self = MY_APPLICATION(object);

  // Perform any actions required at application shutdown.

  G_APPLICATION_CLASS(my_application_parent_class)->shutdown(application);
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object) {
  MyApplication* self = MY_APPLICATION(object);
  g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
  G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

static void my_application_class_init(MyApplicationClass* klass) {
  G_APPLICATION_CLASS(klass)->activate = my_application_activate;
  G_APPLICATION_CLASS(klass)->local_command_line =
      my_application_local_command_line;
  G_APPLICATION_CLASS(klass)->startup = my_application_startup;
  G_APPLICATION_CLASS(klass)->shutdown = my_application_shutdown;
  G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
}

static void my_application_init(MyApplication* self) {}

MyApplication* my_application_new() {
  // Set the program name to the application ID, which helps various systems
  // like GTK and desktop environments map this running application to its
  // corresponding .desktop file. This ensures better integration by allowing
  // the application to be recognized beyond its binary name.
  g_set_prgname(APPLICATION_ID);

  return MY_APPLICATION(g_object_new(my_application_get_type(),
                                     "application-id", APPLICATION_ID, "flags",
                                     G_APPLICATION_NON_UNIQUE, nullptr));
}
