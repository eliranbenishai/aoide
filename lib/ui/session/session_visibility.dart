/// Whether a tramp OS window should be mapped.
///
/// Cold start keeps windows hidden until the session host has mounted chrome
/// (main first; secondaries after their engines exist). After that, layout
/// visibility and minimize suppression apply.
bool sessionWindowShouldShow({
  required bool sessionReady,
  required bool layoutVisible,
  bool minimizeSuppressed = false,
}) {
  return sessionReady && layoutVisible && !minimizeSuppressed;
}
