/// Whether a tramp OS window should be mapped.
///
/// Cold start keeps every window hidden until the session host has mounted
/// chrome. After that, layout visibility and minimize suppression apply.
bool sessionWindowShouldShow({
  required bool sessionReady,
  required bool layoutVisible,
  bool minimizeSuppressed = false,
}) {
  return sessionReady && layoutVisible && !minimizeSuppressed;
}
