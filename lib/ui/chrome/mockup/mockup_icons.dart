import 'package:flutter/widgets.dart';
import 'package:flutter_svg/flutter_svg.dart';

import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';

/// Icon glyphs from `player-mockup-2.html` SVG paths (viewBox 16 / 24).
abstract final class MockupIcons {
  /// Look-aware glyph ink (transport / chrome icons).
  static Color inkOf(BuildContext context, [int alpha = 0xD9]) =>
      LookPaint.glyphInk(LookScope.of(context).palette, alpha);

  /// Look-aware close-button glyph tint.
  static Color closeInkOf(BuildContext context) =>
      LookPaint.closeGlyphInk(LookScope.of(context).palette);

  static Widget minimize({
    Color color = const Color(0xD1D6E2F5),
    double size = 12,
  }) =>
      _svg(_w16('M3 10h10v2H3z'), color, size);

  static Widget zoomOut({
    Color color = const Color(0xD1D6E2F5),
    double size = 12,
  }) =>
      _svg(_w16('M3 7h10v2H3z'), color, size);

  static Widget zoomIn({
    Color color = const Color(0xD1D6E2F5),
    double size = 12,
  }) =>
      _svg(_w16('M7 3h2v4h4v2H9v4H7V9H3V7h4z'), color, size);

  static Widget close({
    Color color = const Color(0xFFFFD6E8),
    double size = 12,
  }) =>
      _svg(
        _w16(
          'M4.4 3l3.6 3.6L11.6 3 13 4.4 9.4 8l3.6 3.6L11.6 13 8 9.4 4.4 13 3 11.6 6.6 8 3 4.4z',
        ),
        color,
        size,
      );

  static Widget previous({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M6 5h2.4v14H6zM20 5v14l-9.6-7z'), color, size);

  static Widget play({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M7 4.5l13 7.5-13 7.5z'), color, size);

  static Widget pause({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M7 5h3.6v14H7zM13.4 5H17v14h-3.6z'), color, size);

  static Widget stop({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M6 6h12v12H6z'), color, size);

  static Widget next({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M15.6 5H18v14h-2.4zM4 5l9.6 7L4 19z'), color, size);

  static Widget eject({
    Color color = const Color(0xD9D6E2F5),
    double size = 22,
  }) =>
      _svg(_w24('M12 4.5l7.5 8.5h-15zM4.5 15.5h15V19h-15z'), color, size);

  static Widget mute({
    Color color = const Color(0xD9D6E2F5),
    double size = 21,
  }) =>
      _svg(
        '''
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="#FFFFFF" d="M4 9.4h3.3L12 5v14L7.3 14.6H4z"/>
  <g fill="none" stroke="#FFFFFF" stroke-width="1.7" stroke-linecap="round">
    <path d="M15.4 9.3a4.2 4.2 0 0 1 0 5.4"/>
    <path d="M18 6.7a7.8 7.8 0 0 1 0 10.6"/>
  </g>
</svg>
''',
        color,
        size,
      );

  static Widget add({
    Color color = const Color(0xD9D6E2F5),
    double size = 21,
  }) =>
      _svg(
        _w24('M10.9 4h2.2v6.9H20v2.2h-6.9V20h-2.2v-6.9H4v-2.2h6.9z'),
        color,
        size,
      );

  static Widget remove({
    Color color = const Color(0xD9D6E2F5),
    double size = 21,
  }) =>
      _svg(_w24('M4 10.9h16v2.2H4z'), color, size);

  static Widget sort({
    Color color = const Color(0xD9D6E2F5),
    double size = 21,
  }) =>
      _svg(
        '''
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#FFFFFF">
  <rect x="2.6" y="5.4" width="11" height="2.2" rx="1.1"/>
  <rect x="2.6" y="10.9" width="8" height="2.2" rx="1.1"/>
  <rect x="2.6" y="16.4" width="5" height="2.2" rx="1.1"/>
  <rect x="17.9" y="5.4" width="2.2" height="9.4" rx=".6"/>
  <path d="M19 19.3 14.9 14.2h8.2z"/>
</svg>
''',
        color,
        size,
      );

  static Widget options({
    Color color = const Color(0xD9D6E2F5),
    double size = 21,
  }) =>
      _svg(
        '''
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#FFFFFF">
  <g>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(45 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(90 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(135 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(180 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(225 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(270 12 12)"/>
    <rect x="10.9" y="3.2" width="2.2" height="4" rx=".5" transform="rotate(315 12 12)"/>
  </g>
  <path fill-rule="evenodd" d="M12 5.7a6.3 6.3 0 1 1 0 12.6 6.3 6.3 0 1 1 0-12.6Zm0 3.7a2.6 2.6 0 1 1 0 5.2 2.6 2.6 0 1 1 0-5.2Z"/>
</svg>
''',
        color,
        size,
      );

  static Widget _svg(String raw, Color color, double size) => SizedBox(
        width: size,
        height: size,
        child: SvgPicture.string(
          raw,
          fit: BoxFit.contain,
          colorFilter: ColorFilter.mode(color, BlendMode.srcIn),
        ),
      );

  static String _w16(String path) =>
      '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16">'
      '<path fill="#FFFFFF" d="$path"/></svg>';

  static String _w24(String path) =>
      '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">'
      '<path fill="#FFFFFF" d="$path"/></svg>';
}
