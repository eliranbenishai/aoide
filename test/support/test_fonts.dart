import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/services.dart';

/// Registers Tramp's bundled TTFs for widget tests.
///
/// `flutter_test` uses a fallback font unless faces are loaded with
/// [FontLoader]. That skews label metrics: "OPEN" in a 54×26 chrome button
/// (40 px usable width after 7 px horizontal padding) measures 46.40 px under
/// the fallback but 26.28 px in Barlow Semi Condensed — the overflow was a
/// test harness artifact, not a layout bug. Do not delete this helper as
/// boilerplate.
Future<void> loadTrampFonts() {
  return _loadFuture ??= _loadOnce();
}

Future<void>? _loadFuture;

Future<void> _loadOnce() async {
  final barlow = FontLoader('BarlowSemiCondensed');
  barlow.addFont(_readFont('assets/fonts/BarlowSemiCondensed-SemiBold.ttf'));
  barlow.addFont(_readFont('assets/fonts/BarlowSemiCondensed-Bold.ttf'));
  await barlow.load();

  final mono = FontLoader('IBMPlexMono');
  mono.addFont(_readFont('assets/fonts/IBMPlexMono-Medium.ttf'));
  mono.addFont(_readFont('assets/fonts/IBMPlexMono-SemiBold.ttf'));
  await mono.load();
}

Future<ByteData> _readFont(String relativePath) async {
  final bytes = await File(relativePath).readAsBytes();
  return ByteData.sublistView(bytes);
}
