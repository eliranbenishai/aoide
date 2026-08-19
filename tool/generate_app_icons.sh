#!/usr/bin/env bash
# Rasterize assets/branding/logo.svg into the PNG / ICO / ICNS / hicolor set.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SVG="$ROOT/assets/branding/logo.svg"
MAGICK="${MAGICK:-/usr/bin/magick}"

if [[ ! -f "$SVG" ]]; then
  echo "generate_app_icons: missing $SVG" >&2
  exit 1
fi
if [[ ! -x "$MAGICK" ]]; then
  echo "generate_app_icons: ImageMagick not at $MAGICK" >&2
  exit 1
fi

raster() {
  local size="$1" dest="$2"
  mkdir -p "$(dirname "$dest")"
  "$MAGICK" -background none -density 384 "$SVG" \
    -resize "${size}x${size}" -gravity center -extent "${size}x${size}" \
    -depth 8 "png32:${dest}"
}

BRAND="$ROOT/assets/branding"
raster 512 "$BRAND/logo.png"
raster 256 "$BRAND/app_icon.png"

APP_ID="com.proximamagnifica.tramp"
HICOLOR="$ROOT/packaging/linux/icons/hicolor"
for size in 16 24 32 48 64 128 256 512; do
  raster "$size" "$HICOLOR/${size}x${size}/apps/${APP_ID}.png"
done

"$MAGICK" "$BRAND/app_icon.png" -define icon:auto-resize=256,48,32,16 \
  "$ROOT/packaging/windows/app_icon.ico"

python3 - "$HICOLOR" "$APP_ID" "$ROOT/packaging/macos/tramp.icns" <<'PY'
import struct
import sys
from pathlib import Path

hicolor = Path(sys.argv[1])
app_id = sys.argv[2]
dest = Path(sys.argv[3])
dest.parent.mkdir(parents=True, exist_ok=True)
types = {
    16: b"icp4",
    32: b"icp5",
    64: b"icp6",
    128: b"ic07",
    256: b"ic08",
    512: b"ic09",
}
chunks = []
for size, ostype in types.items():
    png = hicolor / f"{size}x{size}" / "apps" / f"{app_id}.png"
    data = png.read_bytes()
    chunks.append(ostype + struct.pack(">I", 8 + len(data)) + data)
body = b"".join(chunks)
dest.write_bytes(b"icns" + struct.pack(">I", 8 + len(body)) + body)
print(f"wrote {dest}")
PY

echo "generated branding, hicolor, ICO, and ICNS from $SVG"
