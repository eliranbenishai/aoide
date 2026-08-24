#!/usr/bin/env bash
# Validate the AppStream metainfo file and keep its newest release in step with
# VERSION.
#
# This file is the only place a Linux installer learns that the app is called
# Tramp. A .desktop file's Name= does not reach flatpak, GNOME Software or
# Discover, and when the metainfo is missing or unreadable they fall back to the
# app ID -- which is a silent failure everywhere else in the build:
# flatpak-builder skips its AppStream compose step without a word, and
# `flatpak build-bundle` then embeds neither the name nor the icon.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="$ROOT/packaging/linux/com.proximamagnifica.tramp.metainfo.xml"

if [[ ! -f "$FILE" ]]; then
  echo "check-metainfo: $FILE is missing" >&2
  exit 1
fi

if ! command -v appstreamcli >/dev/null; then
  echo "check-metainfo: appstreamcli not found (install the 'appstream' package)" >&2
  exit 1
fi

# --no-net so a slow or moved screenshot host cannot fail an otherwise good
# build. appstreamcli treats warnings as failures, and a fetched URL is the one
# input to this check that is not in the repository.
appstreamcli validate --no-net --explain "$FILE"

# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"
top="$(sed -n 's/.*<release version="\([^"]*\)".*/\1/p' "$FILE" | head -n 1)"
if [[ "$top" != "$version" ]]; then
  echo "check-metainfo: newest <release> is '$top' but VERSION says '$version'" >&2
  exit 1
fi

echo "check-metainfo: $version, valid AppStream"
