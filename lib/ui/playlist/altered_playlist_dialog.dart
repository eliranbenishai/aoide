import 'package:flutter/material.dart';

import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';

/// What the listener chose when asked about an **altered current playlist**.
enum AlteredPlaylistChoice { save, discard, cancel }

/// Asks before an **altered current playlist** is replaced, offering to save it
/// rather than only to throw it away.
///
/// Cancel takes the default focus, so a Return pressed at an idle keyboard can
/// only ever keep the playlist. Dismissing the dialog any other way — the
/// barrier, Escape — is a cancel too, because the safe choice has to be the one
/// that costs nothing.
Future<AlteredPlaylistChoice> showAlteredPlaylistDialog(
  BuildContext context,
) async {
  // Read the look here rather than in the builder: the dialog's route sits
  // above the [LookScope] the window is wrapped in.
  final look = LookScope.of(context);
  final palette = look.palette;
  final choice = await showDialog<AlteredPlaylistChoice>(
    context: context,
    builder: (context) => AlertDialog(
      key: const Key('pl-altered-dialog'),
      backgroundColor: palette.shellMid,
      surfaceTintColor: Colors.transparent,
      title: Text(
        'Save the current playlist?',
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 14,
          color: palette.inkDefault,
        ),
      ),
      content: Text(
        'The current playlist has changes that are not in any file. '
        'Loading another playlist replaces it.',
        style: TrampText.lcd(look).copyWith(
          color: palette.inkDim,
          height: 1.35,
        ),
      ),
      actions: [
        TextButton(
          key: const Key('pl-altered-cancel'),
          autofocus: true,
          onPressed: () =>
              Navigator.of(context).pop(AlteredPlaylistChoice.cancel),
          child: Text(
            'Cancel',
            style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
          ),
        ),
        TextButton(
          key: const Key('pl-altered-discard'),
          onPressed: () =>
              Navigator.of(context).pop(AlteredPlaylistChoice.discard),
          child: Text(
            'Discard and load',
            style: TrampText.chromeLabel(look)
                .copyWith(color: palette.accentDefault),
          ),
        ),
        TextButton(
          key: const Key('pl-altered-save'),
          onPressed: () => Navigator.of(context).pop(AlteredPlaylistChoice.save),
          child: Text(
            'Save and load',
            style: TrampText.chromeLabel(look)
                .copyWith(color: palette.phosphorDefault),
          ),
        ),
      ],
    ),
  );
  return choice ?? AlteredPlaylistChoice.cancel;
}
