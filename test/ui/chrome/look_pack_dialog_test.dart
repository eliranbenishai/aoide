import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/look/look_controller.dart';
import 'package:tramp/look/look_font_loader.dart';
import 'package:tramp/look/look_installer.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/ui/chrome/look_pack_dialog.dart';

import '../../support/look_harness.dart';
import '../../support/test_fonts.dart';

class MemorySettingsStore implements SettingsStore {
  MemorySettingsStore([this.stored = TrampSettings.defaults]);

  TrampSettings stored;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async {
    stored = settings;
  }
}

/// Test seam: forces [lastError] for dialog rendering without activate I/O.
class FakeErrorLookController extends LookController {
  FakeErrorLookController({
    required super.settingsStore,
    required super.supportDir,
    super.fontLoader,
    this.forcedError,
  });

  final String? forcedError;

  @override
  String? get lastError => forcedError ?? super.lastError;
}

void main() {
  late Directory supportDir;
  late Directory looksDir;
  late MemorySettingsStore store;
  late LookController controller;

  Future<Directory> supportDirFn() async => supportDir;

  setUpAll(loadTrampFonts);

  setUp(() async {
    supportDir = Directory.systemTemp.createTempSync('tramp-look-dialog');
    looksDir = Directory(p.join(supportDir.path, 'looks'));
    await looksDir.create(recursive: true);
    store = MemorySettingsStore();

    final packDir = Directory(p.join(looksDir.path, 'neon-cyan'));
    await packDir.create(recursive: true);
    await File(p.join(packDir.path, 'look.json')).writeAsString('''
{
  "formatVersion": 1,
  "id": "neon-cyan",
  "name": "Neon Cyan",
  "author": "Example",
  "extends": "builtin",
  "colors": {}
}
''');

    controller = LookController(
      settingsStore: store,
      supportDir: supportDirFn,
      fontLoader: LookFontLoader(
        load: ({
          required String family,
          required Uint8List bytes,
          required int weight,
        }) async {},
      ),
    );
    await controller.bootstrap(
      settings: TrampSettings.defaults,
      supportDir: supportDirFn,
    );
  });

  tearDown(() async {
    controller.dispose();
    if (supportDir.existsSync()) {
      await supportDir.delete(recursive: true);
    }
  });

  Future<void> openDialog(WidgetTester tester) async {
    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: Builder(
          builder: (context) {
            return TextButton(
              key: const Key('open-look-packs'),
              onPressed: () => showLookPackDialog(
                context,
                controller: controller,
              ),
              child: const Text('open'),
            );
          },
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('open-look-packs')));
    await tester.pumpAndSettle();
  }

  testWidgets('lists Builtin and installed looks; tap activates',
      (tester) async {
    await openDialog(tester);

    expect(find.text('Builtin'), findsOneWidget);
    expect(find.text('Neon Cyan'), findsOneWidget);
    expect(find.textContaining('Example'), findsOneWidget);

    await tester.tap(find.text('Neon Cyan'));
    await tester.pumpAndSettle();

    expect(controller.activeLookId, 'neon-cyan');
  });

  testWidgets('shows lastError after activate failure', (tester) async {
    final errorController = FakeErrorLookController(
      settingsStore: MemorySettingsStore(),
      supportDir: () async => supportDir,
      fontLoader: LookFontLoader(
        load: ({
          required String family,
          required Uint8List bytes,
          required int weight,
        }) async {},
      ),
      forcedError: 'font file missing',
    );
    addTearDown(errorController.dispose);

    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: Scaffold(
          body: LookPackDialog(controller: errorController),
        ),
      ),
    );
    await tester.pump();

    expect(find.byKey(const Key('look-pack-error')), findsOneWidget);
    expect(find.text('font file missing'), findsOneWidget);
  });

  testWidgets('conflict dialog offers Replace and Cancel', (tester) async {
    LookConflictChoice? choice;

    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) =>
            wrapWithLook(child ?? const SizedBox.shrink()),
        home: Builder(
          builder: (context) {
            return TextButton(
              key: const Key('open-conflict'),
              onPressed: () async {
                choice = await showLookConflictDialog(
                  context,
                  const LookConflict(
                    id: 'neon-cyan',
                    installedName: 'Neon Cyan',
                    installedAuthor: 'Old',
                    incomingName: 'Neon Cyan 2',
                    incomingAuthor: 'New',
                  ),
                );
              },
              child: const Text('conflict'),
            );
          },
        ),
      ),
    );

    await tester.tap(find.byKey(const Key('open-conflict')));
    await tester.pumpAndSettle();

    expect(find.textContaining('Neon Cyan'), findsWidgets);
    expect(find.textContaining('Old'), findsOneWidget);
    expect(find.textContaining('Neon Cyan 2'), findsOneWidget);
    expect(find.textContaining('New'), findsOneWidget);
    expect(find.byKey(const Key('look-conflict-replace')), findsOneWidget);
    expect(find.byKey(const Key('look-conflict-cancel')), findsOneWidget);

    await tester.tap(find.byKey(const Key('look-conflict-cancel')));
    await tester.pumpAndSettle();
    expect(choice, LookConflictChoice.cancel);
  });
}
