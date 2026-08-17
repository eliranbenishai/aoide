#!/usr/bin/env bash
# Codesign tramp.app (hardened runtime) and notarize + staple a DMG.
# No-ops with a warning when certificate / notary secrets are unset.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="${TRAMP_MAC_APP:-$ROOT/build/macos/tramp.app}"
# shellcheck disable=SC1090
eval "$(bash "$ROOT/tool/version.sh")"
DMG="${1:-$ROOT/build/macos/Tramp-${version}-macos-universal.dmg}"

if [[ ! -d "$APP" ]]; then
  echo "notarize: missing $APP" >&2
  exit 1
fi

if [[ -z "${MACOS_CERTIFICATE_BASE64:-}" || -z "${MACOS_CERTIFICATE_PASSWORD:-}" ]]; then
  echo "notarize: MACOS_CERTIFICATE_BASE64 / PASSWORD unset — skipping (unsigned DMG)" >&2
  exit 0
fi

TMP="${RUNNER_TEMP:-$(mktemp -d)}"
KEYCHAIN="$TMP/tramp.keychain-db"
KEYCHAIN_PASSWORD="$(openssl rand -base64 24)"
CERT_PATH="$TMP/tramp-cert.p12"
echo "$MACOS_CERTIFICATE_BASE64" | base64 --decode >"$CERT_PATH"

security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security set-keychain-settings -lut 21600 "$KEYCHAIN"
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security import "$CERT_PATH" -P "$MACOS_CERTIFICATE_PASSWORD" \
  -T /usr/bin/codesign -T /usr/bin/security \
  -t cert -f pkcs12 -k "$KEYCHAIN"
security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security list-keychain -d user -s "$KEYCHAIN"

IDENTITY="$(security find-identity -v -p codesigning "$KEYCHAIN" | awk -F'\"' '/Developer ID Application/ { print $2; exit }')"
if [[ -z "$IDENTITY" ]]; then
  echo "notarize: no Developer ID Application identity in the certificate" >&2
  exit 1
fi

ENTITLEMENTS="$ROOT/macos/Runner/Release.entitlements"
# Sign nested code first; --deep would stamp app sandbox entitlements on libmpv.
find "$APP/Contents" \( -name '*.framework' -o -name '*.dylib' -o -name '*.so' \) -print0 |
  while IFS= read -r -d '' f; do
    codesign --force --options runtime --timestamp --sign "$IDENTITY" "$f" || true
  done
codesign --force --options runtime --timestamp \
  --entitlements "$ENTITLEMENTS" \
  --sign "$IDENTITY" "$APP"
codesign --verify --deep --strict "$APP"

# DMG must contain the signed .app, not the unsigned copy.
bash "$ROOT/packaging/macos/make_dmg.sh"
DMG="$ROOT/build/macos/Tramp-${version}-macos-universal.dmg"
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
