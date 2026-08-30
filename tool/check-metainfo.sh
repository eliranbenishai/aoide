#!/usr/bin/env bash
# Validate the AppStream metainfo file and keep its newest release in step with
# VERSION, including a feature list that tool/release-notes.sh can print.
#
# This file is the only place a Linux installer learns that the app is called
# Aoide. A .desktop file's Name= does not reach flatpak, GNOME Software or
# Discover, and when the metainfo is missing or unreadable they fall back to the
# app ID -- which is a silent failure everywhere else in the build:
# flatpak-builder skips its AppStream compose step without a word, and
# `flatpak build-bundle` then embeds neither the name nor the icon.
#
# --check-urls additionally fetches every declared <image>. Off by default so a
# moved screenshot host cannot fail a build over an input that is not in the
# repository; on where publishing happens, because a listing that advertises a
# screenshot nobody can load is worse than one with none.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="$ROOT/packaging/linux/com.proximamagnifica.aoide.metainfo.xml"
CHECK_URLS=0
for arg in "$@"; do
  case "$arg" in
    --check-urls) CHECK_URLS=1 ;;
    *) echo "check-metainfo: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done

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

# The GitHub body is derived from this <release>. An entry with no <li>
# features would publish as a blank list — the same class of silent success
# as a blank VERSION padding to a legal Store identity. Fail here, where CI
# and pre-flight already run, not at the publish step.
bash "$ROOT/tool/release-notes.sh" >/dev/null

if [[ "$CHECK_URLS" == 1 ]]; then
  # Strip XML comments first, so a commented-out <image> is not fetched. The
  # live block is uncommented as of 1.2; an example URL parked in a comment
  # would otherwise fail a release for not resolving.
  urls="$(perl -0pe 's/<!--.*?-->//gs' "$FILE" \
    | sed -n 's|.*<image[^>]*>\(https\{0,1\}://[^<]*\)</image>.*|\1|p')"
  if [[ -z "$urls" ]]; then
    echo "check-metainfo: no screenshot URLs declared yet"
  else
    dead=0
    while IFS= read -r url; do
      code="$(curl -sS -o /dev/null -w '%{http_code}' -m 20 -L "$url" || echo 000)"
      if [[ "$code" != 200 ]]; then
        echo "check-metainfo: $url returned $code" >&2
        dead=1
      fi
    done <<<"$urls"
    if [[ "$dead" == 1 ]]; then
      echo "  a listing that advertises an unloadable screenshot is worse than one with none" >&2
      exit 1
    fi
    echo "check-metainfo: every screenshot URL resolves"
  fi
fi

echo "check-metainfo: $version, valid AppStream"
