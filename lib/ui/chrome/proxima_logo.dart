import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

/// Path of the Proxima Magnifica mark, declared as an asset in `pubspec.yaml`.
const String proximaMagnificaLogoAsset = 'assets/proximamagnifica.svg';

/// Company mark: wordmark + device from the Proxima Magnifica SVG.
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
