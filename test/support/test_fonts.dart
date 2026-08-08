import 'dart:io';

import 'package:flutter/services.dart';

/// Registers Tramp's bundled TTFs for widget tests.
///
/// `flutter_test` uses a fallback font unless faces are loaded with
/// [FontLoader]. That skews label metrics under the Ahem/fallback face.
/// Do not delete this helper as boilerplate.
Future<void> loadTrampFonts() {
  return _loadFuture ??= _loadOnce();
}

Future<void>? _loadFuture;

Future<void> _loadOnce() async {
  final condensed = FontLoader('TrampCondensed');
  condensed.addFont(_readFont('assets/fonts/TrampCondensed-Bold.ttf'));
  await condensed.load();

  final mono = FontLoader('TrampMono');
  mono.addFont(_readFont('assets/fonts/TrampMono-Medium.ttf'));
  await mono.load();
}

Future<ByteData> _readFont(String relativePath) async {
  final bytes = await File(relativePath).readAsBytes();
  return ByteData.sublistView(bytes);
}
