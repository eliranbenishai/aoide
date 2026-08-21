#!/usr/bin/env bash
# Install the Qt tramp binary plus assets/skins/desktop into build/linux/bundle,
# then deploy Qt and every other non-host library beside it.
#
# A download that needs the user's distro to ship a matching Qt 6 is not a
# download. Windows gets this from windeployqt; Linux has no equivalent, so the
# deployment is here — on the one staging path the tarball, the AppImage and the
# Flatpak all read, so none of them can drift from the others.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${TRAMP_BUILD_DIR:-$ROOT/build}"
DEST="${TRAMP_BUNDLE_DIR:-$ROOT/build/linux/bundle}"
LIB="$DEST/lib"
PLUGINS="$DEST/plugins"

if [[ ! -x "$BUILD/tramp" ]]; then
  echo "stage_bundle: missing $BUILD/tramp — build the Qt host first" >&2
  exit 1
fi
if ! command -v patchelf >/dev/null; then
  echo "stage_bundle: patchelf not found — it is what makes the bundle relocatable" >&2
  exit 1
fi

cmake --install "$BUILD" --prefix "$DEST"
mkdir -p "$LIB" "$PLUGINS"

# Ask the build-tree binary, never the installed one. The installed copy has had
# its RPATH rewritten to $ORIGIN, so it would answer with whatever Qt this
# machine happens to have in ld.so.cache — which is exactly how a bundle ends up
# carrying a different Qt from the one it was compiled against.
qt_core="$(ldd "$BUILD/tramp" | awk '$1 == "libQt6Core.so.6" { print $3 }')"
if [[ -z "$qt_core" || ! -f "$qt_core" ]]; then
  echo "stage_bundle: cannot resolve libQt6Core.so.6 from $BUILD/tramp" >&2
  exit 1
fi
qt_lib_dir="$(cd "$(dirname "$(readlink -f "$qt_core")")" && pwd)"

qt_plugin_root=""
for candidate in \
    "$qt_lib_dir/qt6/plugins" \
    "$qt_lib_dir/../plugins" \
    "$qt_lib_dir/../share/qt/plugins" \
    "$qt_lib_dir/../lib/qt6/plugins"; do
  if [[ -d "$candidate/platforms" ]]; then
    qt_plugin_root="$(cd "$candidate" && pwd)"
    break
  fi
done
if [[ -z "$qt_plugin_root" ]]; then
  echo "stage_bundle: no Qt plugin directory found near $qt_lib_dir" >&2
  exit 1
fi

# Resolve against the Qt we linked and against what is already staged, so a
# second Qt on the build machine cannot leak in. Scoped to ldd and never
# exported: this path is full of libraries that would otherwise be loaded by the
# find and cp calls below, and a rewritten libselinux breaks them silently.
resolve_deps() {
  LD_LIBRARY_PATH="$qt_lib_dir:$LIB" ldd "$1" 2>/dev/null
}

needed_sonames() {
  readelf -d "$1" 2>/dev/null | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p'
}

# Tramp links Core, Gui, Widgets and DBus, and paints every pixel of its own
# chrome. A plugin that also wants Quick, Qml, Pdf, Svg, GTK or KDE Frameworks
# is serving a capability this app does not have, and taking it drags that whole
# stack in behind it — one distro's platform theme turned a 60 MB bundle into
# 400 MB of KDE Frameworks.
wants_foreign_stack() {
  needed_sonames "$1" | grep -qE '^(libQt6(Quick|Qml|Pdf|Svg)\.|libKF6|libgtk-[0-9])'
}

# Groups are taken whole rather than cherry-picked. platforms carries xcb and
# wayland for real desktops and offscreen for the release smoke test, and the
# wayland-* and xcbglintegrations groups are what those platform plugins load in
# turn; guessing wrong here is the failure that only shows up on someone else's
# machine.
skipped=()
for group in platforms platformthemes imageformats \
             xcbglintegrations platforminputcontexts \
             wayland-decoration-client wayland-graphics-integration-client \
             wayland-shell-integration; do
  [[ -d "$qt_plugin_root/$group" ]] || continue
  mkdir -p "$PLUGINS/$group"
  for plugin in "$qt_plugin_root/$group"/*.so; do
    [[ -f "$plugin" ]] || continue
    if wants_foreign_stack "$plugin"; then
      skipped+=("$group/$(basename "$plugin")")
      continue
    fi
    cp -Lf "$plugin" "$PLUGINS/$group/"
  done
  rmdir "$PLUGINS/$group" 2>/dev/null || true
done

for required in platforms/libqoffscreen.so platforms/libqxcb.so; do
  if [[ ! -f "$PLUGINS/$required" ]]; then
    echo "stage_bundle: $qt_plugin_root is missing $required" >&2
    exit 1
  fi
done
# Upstream Qt builds one libqwayland.so; Debian and Ubuntu split the same QPA
# into libqwayland-generic.so and libqwayland-egl.so. Match either shape.
if ! compgen -G "$PLUGINS/platforms/libqwayland*.so" >/dev/null; then
  echo "stage_bundle: warning — no wayland platform plugin in $qt_plugin_root" >&2
fi

# The host keeps the loader, the C/C++ runtimes and the graphics stack: those
# move with the kernel and the GPU, and a bundled copy fights the driver the
# machine actually has. Everything else travels with us.
#
# libcrypt is in the list even though it left glibc years ago: it is still built
# against a particular glibc, and a bundled copy segfaults in its own
# initialiser before main when the host's glibc is not the one it was built for.
host_provided() {
  case "${1##*/}" in
    ld.so|ld-*.so*|ld-linux*|libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*) return 0 ;;
    libresolv.so.*|libutil.so.*|libnsl.so.*|libanl.so.*|libcrypt.so.*) return 0 ;;
    libstdc++.so.*|libgcc_s.so.*) return 0 ;;
    libGL.so.*|libGLX.so.*|libGLdispatch.so.*|libOpenGL.so.*) return 0 ;;
    libEGL.so.*|libGLESv1_CM.so.*|libGLESv2.so.*|libglapi.so.*) return 0 ;;
    libdrm.so.*|libgbm.so.*) return 0 ;;
  esac
  return 1
}

bundle_elfs() {
  printf '%s\n' "$DEST/tramp"
  find "$LIB" "$PLUGINS" -type f -name '*.so*' 2>/dev/null
}

# Walk DT_NEEDED edges and use ldd only to turn a soname into a path. ldd prints
# the whole transitive closure, so copying its output straight would drag in the
# children of libraries we deliberately left to the host — declining libGL and
# then bundling Mesa's 187 MB of LLVM behind it.
#
# A closure, not one pass: a platform plugin pulls a Qt library that pulls an
# ICU library, and stopping early ships a bundle that is one .so short.
added=1
while (( added )); do
  added=0
  while IFS= read -r elf; do
    resolved="$(resolve_deps "$elf" | awk '$2 == "=>" && $3 ~ /^\// { print $1, $3 }')"
    while IFS= read -r soname; do
      [[ -n "$soname" ]] || continue
      host_provided "$soname" && continue
      [[ -e "$LIB/$soname" ]] && continue
      path="$(printf '%s\n' "$resolved" | awk -v want="$soname" '$1 == want { print $2; exit }')"
      [[ -n "$path" && -f "$path" ]] || continue
      cp -Lf "$path" "$LIB/$soname"
      added=1
    done < <(needed_sonames "$elf")
  done < <(bundle_elfs)
done

# Every library still carries the runpath it had on the build machine, and a
# library's own DT_RUNPATH is consulted before anything it was loaded from — so
# libpulse would keep looking in /usr/lib64 for its private sublibrary even
# though we staged a copy. Point them all back inside the bundle. This is the
# difference between an archive that runs where it was built and one that runs
# where it is extracted.
while IFS= read -r elf; do
  patchelf --set-rpath '$ORIGIN' "$elf"
done < <(find "$LIB" -maxdepth 1 -type f -name '*.so*')
while IFS= read -r elf; do
  patchelf --set-rpath '$ORIGIN:$ORIGIN/../../lib' "$elf"
done < <(find "$PLUGINS" -type f -name '*.so')

# The tarball is extracted and run in place with no launcher, so the plugin
# lookup has to live in the files themselves. Qt reads qt.conf relative to the
# directory holding the executable.
cat >"$DEST/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
Libraries = lib
Data = .
EOF

unresolved="$(while IFS= read -r elf; do
  while IFS= read -r soname; do
    [[ -n "$soname" ]] || continue
    host_provided "$soname" && continue
    [[ -e "$LIB/$soname" ]] || printf '%s\n' "$soname"
  done < <(needed_sonames "$elf")
done < <(bundle_elfs) | sort -u)"

echo "Staged $DEST/tramp"
echo "  Qt $qt_lib_dir"
echo "  plugins $qt_plugin_root"
echo "  $(find "$LIB" -maxdepth 1 -name '*.so*' | wc -l) libraries, $(find "$PLUGINS" -name '*.so' | wc -l) plugins, $(du -sh "$DEST" | cut -f1) total"
if ((${#skipped[@]})); then
  echo "  skipped (foreign UI stack): ${skipped[*]}"
fi
if [[ -n "$unresolved" ]]; then
  echo "  unresolved on this machine: $(echo "$unresolved" | tr '\n' ' ')" >&2
fi
