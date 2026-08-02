import 'package:flutter/widgets.dart';

class SkinImage extends StatelessWidget {
  const SkinImage({
    super.key,
    required this.asset,
    required this.logicalSize,
    this.fit = BoxFit.fill,
  });

  final String asset;
  final Size logicalSize;
  final BoxFit fit;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: Image.asset(
        asset,
        fit: fit,
        filterQuality: FilterQuality.medium,
        gaplessPlayback: true,
      ),
    );
  }
}
