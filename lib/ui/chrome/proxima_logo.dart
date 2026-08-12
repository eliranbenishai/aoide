import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

/// Path of the Proxima Magnifica mark, declared as an asset in `pubspec.yaml`.
const String proximaMagnificaLogoAsset = 'assets/proximamagnifica.svg';

/// Company mark: wordmark + device from the Proxima Magnifica SVG.
///
/// The asset is a full lockup, so its own two-line wordmark only reads at
/// roughly 100 logical pixels and up. Below that use [ProximaMagnificaMark]
/// and set the company name in chrome type beside it.
class ProximaMagnificaLogo extends StatelessWidget {
  const ProximaMagnificaLogo({super.key, this.size = 72});

  final double size;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: size,
      height: size,
      child: SvgPicture.asset(
        proximaMagnificaLogoAsset,
        fit: BoxFit.contain,
        semanticsLabel: 'Proxima Magnifica',
      ),
    );
  }
}

/// The comet device from the company lockup, without its wordmark.
///
/// Sized by [height]; width follows the device's own proportions. Crops the
/// shared asset rather than shipping a second file — there is no symbol-only
/// export yet (`docs/design/ASSETS_NEEDED.md`).
class ProximaMagnificaMark extends StatelessWidget {
  const ProximaMagnificaMark({super.key, this.height = 27});

  final double height;

  /// Device bounds inside the asset's 480×480 viewBox.
  static const _deviceRect = Rect.fromLTWH(14, 40, 445, 250);
  static const _viewBox = 480.0;

  @override
  Widget build(BuildContext context) {
    final scale = height / _deviceRect.height;
    return Semantics(
      label: 'Proxima Magnifica',
      child: ClipRect(
        child: SizedBox(
          width: _deviceRect.width * scale,
          height: height,
          child: OverflowBox(
            alignment: Alignment.topLeft,
            maxWidth: double.infinity,
            maxHeight: double.infinity,
            child: Transform.translate(
              offset: -_deviceRect.topLeft * scale,
              child: SizedBox.square(
                dimension: _viewBox * scale,
                child: SvgPicture.asset(proximaMagnificaLogoAsset),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
