import 'package:flutter/material.dart';

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
      return AlertDialog(
        backgroundColor: TrampColors.panelBottom,
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
                    style: TrampText.chromeLabel.copyWith(
                      fontSize: 22,
                      letterSpacing: 4,
                      color: TrampColors.railAccent,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Version $version',
                    style: TrampText.lcd.copyWith(color: TrampColors.labelDim),
                  ),
                ],
              ),
            ),
          ],
        ),
        content: Text(
          'A desktop music player — dense, playlist-centric, '
          'with distinctive chrome.',
          style: TrampText.lcd.copyWith(color: TrampColors.label, height: 1.35),
        ),
        actions: [
          TextButton(
            key: const Key('about-close'),
            onPressed: () => Navigator.of(context).pop(),
            child: Text(
              'Close',
              style: TrampText.chromeLabel.copyWith(color: TrampColors.phosphor),
            ),
          ),
        ],
      );
    },
  );
}
