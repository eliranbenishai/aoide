#!/usr/bin/env bash
# Shared Apple Silicon checks for staging, signing and wrapping macOS bundles.
# Source this file; only stage_app.sh may thin the copied dependencies.

require_arm64_binary() {
  local binary="$1" architectures
  if ! architectures="$(lipo -archs "$binary" 2>/dev/null)"; then
    echo "macOS packaging: cannot read Mach-O architectures: $binary" >&2
    return 1
  fi
  if [[ "$architectures" != arm64 ]]; then
    echo "macOS packaging: expected arm64 only, found '$architectures': $binary" >&2
    return 1
  fi
}

require_arm64_bundle() {
  local app="$1" binary
  require_arm64_binary "$app/Contents/MacOS/Aoide" || return 1
  # Framework symlinks point at these regular files; never replace a symlink.
  while IFS= read -r -d '' binary; do
    [[ "$(file -b "$binary")" == *Mach-O* ]] || continue
    require_arm64_binary "$binary" || return 1
  done < <(find "$app/Contents" -type f -print0)
}

thin_arm64_bundle_dependencies() {
  local app="$1" binary architectures temporary
  # Reject stale Intel/universal app builds rather than silently repairing them.
  require_arm64_binary "$app/Contents/MacOS/Aoide" || return 1
  while IFS= read -r -d '' binary; do
    [[ "$(file -b "$binary")" == *Mach-O* ]] || continue
    architectures="$(lipo -archs "$binary")" || return 1
    [[ "$architectures" != arm64 ]] || continue
    if ! lipo "$binary" -verify_arch arm64; then
      echo "macOS packaging: dependency has no arm64 slice: $binary" >&2
      return 1
    fi
    temporary="$(mktemp "${binary}.arm64.XXXXXX")" || return 1
    if ! lipo "$binary" -thin arm64 -output "$temporary" ||
        ! chmod "$(stat -f '%Lp' "$binary")" "$temporary" ||
        ! mv -f "$temporary" "$binary"; then
      rm -f "$temporary"
      return 1
    fi
  done < <(find "$app/Contents" -type f -print0)
  require_arm64_bundle "$app"
}
