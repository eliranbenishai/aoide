import 'package:flutter/material.dart';

import '../../theme/look_scope.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';
import 'logo.dart';

/// Brand-forward About sheet: full colour [TrampLogo], name, version, blurb.
Future<void> showTrampAboutDialog(
  BuildContext context, {
  required String version,
}) {
  return showDialog<void>(
    context: context,
    builder: (context) {
      final look = LookScope.of(context);
      final colors = TrampColors.of(look);
      return AlertDialog(
        backgroundColor: colors.panelBottom,
        surfaceTintColor: Colors.transparent,
        titlePadding: const EdgeInsets.fromLTRB(24, 20, 24, 0),
        contentPadding: const EdgeInsets.fromLTRB(24, 16, 24, 8),
        actionsPadding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
        title: Row(
          children: [
            const TrampLogo(size: 56),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'TRAMP',
                    style: TrampText.chromeLabel(look).copyWith(
                      fontSize: 22,
                      letterSpacing: 4,
                      color: colors.railAccent,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Version $version',
                    style: TrampText.lcd(look).copyWith(color: colors.labelDim),
                  ),
                ],
              ),
            ),
          ],
        ),
        content: Text(
          'A desktop music player — dense, playlist-centric, '
          'with distinctive chrome.',
          style: TrampText.lcd(look).copyWith(color: colors.label, height: 1.35),
        ),
        actions: [
          TextButton(
            key: const Key('about-close'),
            onPressed: () => Navigator.of(context).pop(),
            child: Text(
              'Close',
              style: TrampText.chromeLabel(look).copyWith(color: colors.phosphor),
            ),
          ),
        ],
      );
    },
  );
}
