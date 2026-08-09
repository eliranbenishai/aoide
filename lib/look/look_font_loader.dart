import 'dart:io';

import 'package:flutter/services.dart';

typedef FontBytesLoader = Future<void> Function({
  required String family,
  required Uint8List bytes,
  required int weight,
});

class LookFontLoader {
  LookFontLoader({FontBytesLoader? load}) : _load = load ?? _defaultLoad;

  final FontBytesLoader _load;

  Future<String> ensureFamily({
    required String packId,
    required String role,
    required File file,
    required int weight,
  }) async {
    final family = 'Look.$packId.$role';
    final bytes = await file.readAsBytes();
    await _load(
      family: family,
      bytes: Uint8List.fromList(bytes),
      weight: weight,
    );
    return family;
  }

  static Future<void> _defaultLoad({
    required String family,
    required Uint8List bytes,
    required int weight,
  }) async {
    // [weight] is part of the injectable seam; FontLoader registers by family.
    final loader = FontLoader(family);
    loader.addFont(Future<ByteData>.value(ByteData.sublistView(bytes)));
    await loader.load();
  }
}
