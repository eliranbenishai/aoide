import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../theme/tramp_colors.dart';
import 'title_bar.dart';

class TrampShell extends StatelessWidget {
  const TrampShell({
    super.key,
    required this.transport,
    required this.playlist,
  });

  final Widget transport;
  final Widget playlist;

  @override
  Widget build(BuildContext context) {
    return DragToResizeArea(
      resizeEdgeSize: 6,
      child: ColoredBox(
        color: TrampColors.surface,
        child: DecoratedBox(
          decoration: BoxDecoration(
            border: Border.all(
              color: TrampColors.ink,
              width: TrampColors.borderWidth,
            ),
          ),
          child: Column(
            children: [
              const TitleBar(),
              transport,
              Expanded(child: playlist),
            ],
          ),
        ),
      ),
    );
  }
}
