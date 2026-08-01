import 'package:desktop_drop/desktop_drop.dart';

import 'package:flutter/material.dart';

import 'package:window_manager/window_manager.dart';

import '../theme/tramp_colors.dart';

import 'title_bar.dart';

class TrampShell extends StatelessWidget {
  const TrampShell({
    super.key,
    required this.transport,
    required this.playlist,
    this.onDropPaths,
  });

  final Widget transport;

  final Widget playlist;

  final void Function(List<String> paths)? onDropPaths;

  @override
  Widget build(BuildContext context) {
    final shell = DragToResizeArea(
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

    if (onDropPaths == null) {
      return shell;
    }

    return DropTarget(
      onDragDone: (details) {
        final paths = details.files
            .map((file) => file.path)
            .where((path) => path.isNotEmpty)
            .toList();

        if (paths.isNotEmpty) {
          onDropPaths!(paths);
        }
      },
      child: shell,
    );
  }
}
