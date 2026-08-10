import 'package:flutter/material.dart';

import '../../domain/tramp_settings.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';
import '../session/session_messages.dart';
import 'skins_panel.dart';

/// Settings body: General prefs + Skins catalog + reset.
class MockupSettings extends StatefulWidget {
  const MockupSettings({
    super.key,
    required this.snapshot,
    this.onSessionCommand,
  });

  final SettingsSnapshotEvent snapshot;
  final ValueChanged<SessionCommand>? onSessionCommand;

  @override
  State<MockupSettings> createState() => _MockupSettingsState();
}

class _MockupSettingsState extends State<MockupSettings>
    with SingleTickerProviderStateMixin {
  late final TabController _tabs;

  @override
  void initState() {
    super.initState();
    _tabs = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() {
    _tabs.dispose();
    super.dispose();
  }

  void _emit(SessionCommand command) =>
      widget.onSessionCommand?.call(command);

  Future<void> _confirmReset() async {
    final look = LookScope.of(context);
    final palette = look.palette;
    final ok = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: palette.shellMid,
        title: Text(
          'Reset settings?',
          style: TrampText.chromeLabel(look).copyWith(color: palette.inkDefault),
        ),
        content: Text(
          'This restores all Tramp settings to defaults, including window '
          'positions and the active skin.',
          style: TrampText.lcd(look).copyWith(color: palette.inkDim),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: Text(
              'Cancel',
              style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
            ),
          ),
          TextButton(
            key: const Key('settings-reset-confirm'),
            onPressed: () => Navigator.of(context).pop(true),
            child: Text(
              'Reset',
              style: TrampText.chromeLabel(look)
                  .copyWith(color: palette.accentDefault),
            ),
          ),
        ],
      ),
    );
    if (ok == true) {
      _emit(const ResetSettingsCommand());
    }
  }

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final snap = widget.snapshot;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        TabBar(
          controller: _tabs,
          labelColor: palette.phosphorDefault,
          unselectedLabelColor: palette.inkDim,
          indicatorColor: palette.phosphorDefault,
          labelStyle: TrampText.chromeLabel(look).copyWith(fontSize: 12),
          tabs: const [
            Tab(text: 'General'),
            Tab(text: 'Skins'),
          ],
        ),
        Expanded(
          child: TabBarView(
            controller: _tabs,
            children: [
              _GeneralTab(
                snapshot: snap,
                onChanged: (cmd) => _emit(cmd),
                onReset: _confirmReset,
              ),
              Padding(
                padding: const EdgeInsets.fromLTRB(8, 8, 8, 8),
                child: SkinsPanel(
                  skins: snap.skins,
                  activeSkinId: snap.activeSkinId,
                  lastError: snap.lastSkinError,
                  onActivate: (id) => _emit(ActivateSkinCommand(id)),
                  onInstallZipPath: (path) => _emit(
                    InstallSkinPathCommand(path: path, isDirectory: false),
                  ),
                  onInstallDirectoryPath: (path) => _emit(
                    InstallSkinPathCommand(path: path, isDirectory: true),
                  ),
                  onSetSkinsDirectory: (path) =>
                      _emit(SetSkinsDirectoryCommand(path)),
                  onResetSkinsDirectory: () =>
                      _emit(const SetSkinsDirectoryCommand(null)),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class _GeneralTab extends StatelessWidget {
  const _GeneralTab({
    required this.snapshot,
    required this.onChanged,
    required this.onReset,
  });

  final SettingsSnapshotEvent snapshot;
  final ValueChanged<UpdateGeneralSettingsCommand> onChanged;
  final VoidCallback onReset;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;

    return ListView(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
      children: [
        _ToggleRow(
          label: 'Resume last session',
          value: snapshot.resumeLastSession,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(resumeLastSession: v),
          ),
        ),
        _ToggleRow(
          label: 'Confirm before quit',
          value: snapshot.confirmBeforeQuit,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(confirmBeforeQuit: v),
          ),
        ),
        _ToggleRow(
          label: 'Scroll title',
          value: snapshot.scrollTitle,
          onChanged: (v) =>
              onChanged(UpdateGeneralSettingsCommand(scrollTitle: v)),
        ),
        _ToggleRow(
          label: 'Minimize hides secondaries',
          value: snapshot.minimizeHidesSecondaries,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(minimizeHidesSecondaries: v),
          ),
        ),
        const SizedBox(height: 8),
        Text(
          'Dock snap strength',
          style: TrampText.chromeLabel(look).copyWith(
            fontSize: 12,
            color: palette.inkDefault,
          ),
        ),
        const SizedBox(height: 6),
        SegmentedButton<DockSnapStrength>(
          segments: const [
            ButtonSegment(value: DockSnapStrength.off, label: Text('Off')),
            ButtonSegment(
              value: DockSnapStrength.normal,
              label: Text('Normal'),
            ),
            ButtonSegment(
              value: DockSnapStrength.strong,
              label: Text('Strong'),
            ),
          ],
          selected: {snapshot.dockSnapStrength},
          onSelectionChanged: (set) {
            final value = set.first;
            onChanged(UpdateGeneralSettingsCommand(dockSnapStrength: value));
          },
        ),
        const SizedBox(height: 20),
        Align(
          alignment: Alignment.centerLeft,
          child: TextButton(
            key: const Key('settings-reset'),
            onPressed: onReset,
            child: Text(
              'Reset Settings…',
              style: TrampText.chromeLabel(look)
                  .copyWith(color: palette.accentDefault),
            ),
          ),
        ),
      ],
    );
  }
}

class _ToggleRow extends StatelessWidget {
  const _ToggleRow({
    required this.label,
    required this.value,
    required this.onChanged,
  });

  final String label;
  final bool value;
  final ValueChanged<bool> onChanged;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return SwitchListTile(
      contentPadding: EdgeInsets.zero,
      dense: true,
      title: Text(
        label,
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 12,
          color: palette.inkDefault,
        ),
      ),
      value: value,
      activeThumbColor: palette.phosphorDefault,
      onChanged: onChanged,
    );
  }
}
