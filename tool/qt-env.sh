# Shared Qt pin for build.sh and the tool/* compile scripts.
# Requires ROOT. Sets AOIDE_QT_VERSION, QT, MOC.
# If QT is unset, uses the official kit under .local/qt/<pin>/gcc_64 and
# fetches it when missing. A QT that is the wrong version is a hard error.

aoide_read_qt_pin() {
  AOIDE_QT_VERSION="$(tr -d '\r[:space:]' < "$ROOT/QT_VERSION")"
}

aoide_qt_prefix() {
  echo "$ROOT/.local/qt/$AOIDE_QT_VERSION/gcc_64"
}

aoide_qt_report_version() {
  if [[ -x "$QT/bin/qmake6" ]]; then
    "$QT/bin/qmake6" -query QT_VERSION && return
  fi
  if [[ -x "$QT/bin/qmake" ]]; then
    "$QT/bin/qmake" -query QT_VERSION && return
  fi
  local core
  core="$(echo "$QT"/lib/libQt6Core.so.6.*)"
  if [[ -e "$core" ]]; then
    basename "$core" | sed 's/^libQt6Core\.so\.//'
  else
    echo unknown
  fi
}

aoide_find_moc() {
  if [[ -n "${MOC:-}" && -x "$MOC" ]]; then
    return
  fi
  local candidate
  for candidate in "$QT/libexec/moc" "$QT/share/qt/libexec/moc" "$QT/libexec/moc6"; do
    if [[ -x "$candidate" ]]; then
      MOC="$candidate"
      return
    fi
  done
  echo "moc not found under $QT" >&2
  exit 1
}

aoide_resolve_qt() {
  aoide_read_qt_pin
  if [[ -z "${QT:-}" ]]; then
    QT="$(aoide_qt_prefix)"
  fi
  if [[ ! -e "$QT/lib/libQt6Core.so" && ! -e "$QT/lib/libQt6Core.so.6" ]]; then
    if [[ "${AOIDE_QT_NO_FETCH:-}" == 1 ]]; then
      echo "Qt $AOIDE_QT_VERSION is not at $QT. Run ./tool/fetch_qt.sh" >&2
      exit 1
    fi
    echo "build: fetching Qt $AOIDE_QT_VERSION into .local/qt" >&2
    bash "$ROOT/tool/fetch_qt.sh"
    QT="$(aoide_qt_prefix)"
  fi
  local got
  got="$(aoide_qt_report_version)"
  if [[ "$got" != "$AOIDE_QT_VERSION" ]]; then
    echo "Aoide is pinned to Qt $AOIDE_QT_VERSION (QT_VERSION). Found $got at $QT." >&2
    echo "Run ./tool/fetch_qt.sh, or set QT to that kit." >&2
    exit 1
  fi
  aoide_find_moc
}
