import 'package:flutter/painting.dart';

Color lookColorFromHex(String hex) {
  final body = hex.substring(1);
  if (body.length == 6) {
    return Color(0xFF000000 | int.parse(body, radix: 16));
  }
  final rgb = int.parse(body.substring(0, 6), radix: 16);
  final alpha = int.parse(body.substring(6, 8), radix: 16);
  return Color((alpha << 24) | rgb);
}

String lookColorToHex(Color color) {
  final value = color.toARGB32();
  final rgb = value & 0xFFFFFF;
  final alpha = (value >> 24) & 0xFF;
  final rgbHex = rgb.toRadixString(16).padLeft(6, '0');
  if (alpha == 0xFF) {
    return '#$rgbHex';
  }
  return '#$rgbHex${alpha.toRadixString(16).padLeft(2, '0')}';
}
