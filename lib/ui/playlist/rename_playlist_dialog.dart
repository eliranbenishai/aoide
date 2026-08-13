import 'package:flutter/material.dart';

import '../../look/resolved_look.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';

/// Asks the listener what a **saved playlist** should be called.
///
/// Answers with the name they typed, or null when they backed out. Empty is a
/// real answer and not a cancel: clearing the field asks for the override to be
/// dropped, and the row falls back to the playlist file's own name rather than
/// going blank.
///
/// Nothing on disk is at stake here — only the name Tramp's index carries, per
/// `docs/adr/0008-playlist-collection-stores-references.md` — so the field is
/// seeded with what the row currently reads and the listener edits from there.
Future<String?> showRenamePlaylistDialog(
  BuildContext context, {
  required String currentName,
}) {
  // Read the look here rather than in the builder: the dialog's route sits
  // above the [LookScope] the window is wrapped in.
  final look = LookScope.of(context);
  return showDialog<String>(
    context: context,
    builder: (context) => _RenamePlaylistDialog(look: look, name: currentName),
  );
}

/// Stateful only to own the [TextEditingController]: the field outlives the
/// `showDialog` future by one exit transition, so the controller has to be
/// disposed by the widget that holds it rather than by the caller.
class _RenamePlaylistDialog extends StatefulWidget {
  const _RenamePlaylistDialog({required this.look, required this.name});

  final ResolvedLook look;
  final String name;

  @override
  State<_RenamePlaylistDialog> createState() => _RenamePlaylistDialogState();
}

class _RenamePlaylistDialogState extends State<_RenamePlaylistDialog> {
  late final TextEditingController _field = TextEditingController(
    text: widget.name,
  )..selection = TextSelection(
      baseOffset: 0,
      extentOffset: widget.name.length,
    );

  @override
  void dispose() {
    _field.dispose();
    super.dispose();
  }

  void _confirm() => Navigator.of(context).pop(_field.text);

  @override
  Widget build(BuildContext context) {
    final look = widget.look;
    final palette = look.palette;
    return AlertDialog(
      key: const Key('pl-rename-dialog'),
      backgroundColor: palette.shellMid,
      surfaceTintColor: Colors.transparent,
      title: Text(
        'Rename playlist',
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 14,
          color: palette.inkDefault,
        ),
      ),
      content: TextField(
        key: const Key('pl-rename-field'),
        controller: _field,
        autofocus: true,
        style: TrampText.lcd(look).copyWith(color: palette.phosphorDefault),
        decoration: InputDecoration(
          hintText: 'Empty falls back to the file name',
          hintStyle: TrampText.lcd(look).copyWith(color: palette.inkFaint),
        ),
        onSubmitted: (_) => _confirm(),
      ),
      actions: [
        TextButton(
          key: const Key('pl-rename-cancel'),
          onPressed: () => Navigator.of(context).pop(),
          child: Text(
            'Cancel',
            style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
          ),
        ),
        TextButton(
          key: const Key('pl-rename-confirm'),
          onPressed: _confirm,
          child: Text(
            'Rename',
            style: TrampText.chromeLabel(look)
                .copyWith(color: palette.phosphorDefault),
          ),
        ),
      ],
    );
  }
}
