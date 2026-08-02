import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

/// Path of the master logo artwork, declared as an asset in `pubspec.yaml`.
const String trampLogoAsset = 'lib/ui/chrome/logo.svg';

/// The Tramp mark: pin-up in headphones inside a ring badge.
///
/// The artwork is slightly taller than it is wide, so it is fitted inside a
/// square box rather than stretched.
class TrampLogo extends StatelessWidget {
  const TrampLogo({super.key, this.size = 28});

  final double size;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: size,
      height: size,
      child: SvgPicture.asset(
        trampLogoAsset,
        fit: BoxFit.contain,
        semanticsLabel: 'Tramp',
      ),
    );
  }
}
