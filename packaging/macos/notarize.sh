#!/usr/bin/env bash
# Codesign Aoide.app (hardened runtime) and notarize + staple a DMG.
# No-ops with a warning when certificate / notary secrets are unset.
#
# Sign the app, then wrap it. make_dmg.sh copies without mutating, so the
# sealed signature stays intact inside the image.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"

pick_app() {
  local c
  for c in \
      "${AOIDE_MAC_APP:-}" \
      "$ROOT/build/macos/stage/Aoide.app" \
      "$ROOT/build/macos/Aoide.app" \
      "$ROOT/build/Aoide.app"; do
    [[ -n "$c" ]] || continue
    if [[ -x "$c/Contents/MacOS/Aoide" ]]; then
      printf '%s\n' "$c"
      return 0
    fi
  done
  printf '%s\n' "${AOIDE_MAC_APP:-$ROOT/build/macos/stage/Aoide.app}"
  return 1
}

if ! APP="$(pick_app)"; then
  echo "notarize: $APP is not a complete Aoide.app" >&2
  exit 1
fi
DMG="${1:-${AOIDE_MAC_DMG:-$ROOT/build/macos/Aoide-${version}-macos-universal.dmg}}"

if [[ -z "${MACOS_CERTIFICATE_BASE64:-}" || -z "${MACOS_CERTIFICATE_PASSWORD:-}" ]]; then
  echo "notarize: MACOS_CERTIFICATE_BASE64 / PASSWORD unset — skipping (unsigned DMG)" >&2
  exit 0
fi

TMP="${RUNNER_TEMP:-$(mktemp -d)}"
KEYCHAIN="$TMP/aoide.keychain-db"
KEYCHAIN_PASSWORD="$(openssl rand -base64 24)"
CERT_PATH="$TMP/aoide-cert.p12"
echo "$MACOS_CERTIFICATE_BASE64" | base64 --decode >"$CERT_PATH"

security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security set-keychain-settings -lut 21600 "$KEYCHAIN"
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
# No -t cert: that imports the certificate without its private key, and the
# identity check below then fails with a confusing "no Developer ID" error.
security import "$CERT_PATH" -P "$MACOS_CERTIFICATE_PASSWORD" \
  -T /usr/bin/codesign -T /usr/bin/security \
  -f pkcs12 -k "$KEYCHAIN"
security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN"

# Replacing the search list with only the temp keychain hides the user's
# login keychain for the rest of the session — including after the job, on
# a personal Mac. Append instead.
old_keychains=()
while IFS= read -r line; do
  line="${line#"${line%%[![:space:]]*}"}"
  line="${line#\"}"
  line="${line%\"}"
  [[ -n "$line" ]] && old_keychains+=("$line")
done < <(security list-keychain -d user)
# /bin/bash on macOS is 3.2, where "${empty[@]}" under set -u aborts rather
# than expanding to nothing. Branch instead of expanding an empty list.
if ((${#old_keychains[@]})); then
  security list-keychain -d user -s "$KEYCHAIN" "${old_keychains[@]}"
else
  security list-keychain -d user -s "$KEYCHAIN"
fi

IDENTITY="$(security find-identity -v -p codesigning "$KEYCHAIN" | awk -F'\"' '/Developer ID Application/ { print $2; exit }')"
if [[ -z "$IDENTITY" ]]; then
  echo "notarize: no Developer ID Application identity in the certificate" >&2
  exit 1
fi

ENTITLEMENTS="$ROOT/packaging/macos/aoide.entitlements"
if [[ ! -f "$ENTITLEMENTS" ]]; then
  echo "notarize: missing $ENTITLEMENTS" >&2
  exit 1
fi

# Finder copies and curl leave com.apple.quarantine / resource forks that
# codesign --strict then rejects as an invalid signature.
xattr -cr "$APP" || true

# Sign nested code first, without the app entitlements. --deep would stamp
# those on libmpv / Qt frameworks, which is not what they are for. Dylibs
# before framework bundles so the inner images exist when the bundle is sealed.
while IFS= read -r -d '' f; do
  codesign --force --options runtime --timestamp --sign "$IDENTITY" "$f"
done < <(find "$APP/Contents" \( -name '*.dylib' -o -name '*.so' \) -print0)
while IFS= read -r -d '' f; do
  codesign --force --options runtime --timestamp --sign "$IDENTITY" "$f"
done < <(find "$APP/Contents" -name '*.framework' -print0)

codesign --force --options runtime --timestamp \
  --entitlements "$ENTITLEMENTS" \
  --sign "$IDENTITY" "$APP"
codesign --verify --deep --strict "$APP"

AOIDE_MAC_APP="$APP" AOIDE_MAC_DMG="$DMG" bash "$ROOT/packaging/macos/make_dmg.sh"
codesign --force --timestamp --sign "$IDENTITY" "$DMG"

if [[ -n "${APPLE_API_KEY_BASE64:-}" && -n "${APPLE_API_KEY_ID:-}" && -n "${APPLE_API_ISSUER:-}" ]]; then
  KEYFILE="$TMP/AuthKey_${APPLE_API_KEY_ID}.p8"
  echo "$APPLE_API_KEY_BASE64" | base64 --decode >"$KEYFILE"
  xcrun notarytool submit "$DMG" \
    --key "$KEYFILE" \
    --key-id "$APPLE_API_KEY_ID" \
    --issuer "$APPLE_API_ISSUER" \
    --wait
elif [[ -n "${APPLE_ID:-}" && -n "${APPLE_APP_SPECIFIC_PASSWORD:-}" && -n "${APPLE_TEAM_ID:-}" ]]; then
  xcrun notarytool submit "$DMG" \
    --apple-id "$APPLE_ID" \
    --password "$APPLE_APP_SPECIFIC_PASSWORD" \
    --team-id "$APPLE_TEAM_ID" \
    --wait
else
  echo "notarize: signed but no notary credentials — skipping notarytool" >&2
  exit 0
fi

xcrun stapler staple "$DMG"
echo "notarize: stapled $DMG"
