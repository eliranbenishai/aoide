# Look Packs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Load look packs (`look.json` + optional TTF/OTF) so users can recolor and retype mockup chrome, with a configurable looks directory, Replace/Cancel install conflicts, and session-wide active look.

**Architecture:** Pure Dart parse/merge/catalog/install under `lib/look/`. `LookController` (session host) resolves the active pack, loads fonts, and exposes a `ResolvedLook`. Window roots wrap chrome in `LookScope`; painters take palette/materials from the scope (or constructor args). Secondary windows receive `LookSnapshotEvent` over the existing session bus. Builtin look is embedded and always the merge root.

**Tech Stack:** Flutter desktop, `path_provider`, `path`, `file_picker` (already present), add `archive` for zip install, Flutter `FontLoader` / `loadFontFromList`.

**Design spec:** [`docs/superpowers/specs/2026-08-09-look-pack-format-design.md`](../specs/2026-08-09-look-pack-format-design.md)

## Global Constraints

- Dart SDK `>=3.5.0 <4.0.0`; do not raise the floor.
- Product term is **look pack** (not classic skin / WSZ). Manifest file: `look.json`.
- Pack `id`: `[a-z0-9]+(-[a-z0-9]+)*`; reserved `builtin`; directory name must equal `id`.
- Partial packs require `extends`; merge depth cap **8**; reject cycles; unknown keys under `colors` / `materials` / `fonts` fail validation; `stops` arrays replace wholly.
- Fonts: `.ttf` / `.otf` only; roles `chrome` and `lcd`.
- Default looks dir: `<getApplicationSupportDirectory()>/looks/`. Settings override via `looksDirectory` (null/empty = default). On directory change: rescan; if active id missing → `builtin`.
- Install conflict: prompt Replace / Cancel with name + author; no silent overwrite.
- Geometry, icons, noise, logo, variable fonts, JSON hot-reload: out of scope.
- Every task: `flutter analyze` clean and relevant `flutter test` green before commit.
- Update `docs/architecture.md` / spec status when the feature lands (final task).

### Pinned implementation choices

| Topic | Pin |
|-------|-----|
| Package root | `lib/look/` |
| Resolved color type | `LookPalette` with human-readable Dart fields (`shellHighlight`, `phosphorDefault`, …) |
| Builtin colors source | Built from current `MockupTokens` values |
| Runtime chrome access | `LookScope` `InheritedWidget` at each window root |
| `MockupTokens` | Remains the compile-time builtin constant table; painters stop reading it directly once `LookScope` exists |
| Font family names | Builtin: `TrampCondensed` / `TrampMono`; overrides: `Look.<id>.chrome` / `Look.<id>.lcd` |
| Zip dependency | `archive: ^4.0.0` (or latest compatible with SDK) |
| Conflict API | `LookInstaller` takes `Future<LookConflictChoice> Function(LookConflict info)` — UI supplies dialog |
| Session fan-out | `LookSnapshotEvent` with serializable palette + materials + font family names (not font bytes) |
| Settings keys | `activeLookId` (String, default `"builtin"`), `looksDirectory` (String?, default null) |

---

## File Structure

**Created:**

| Path | Responsibility |
|------|----------------|
| `lib/look/look_id.dart` | Slug validation + reserved ids |
| `lib/look/look_manifest.dart` | Parsed `look.json` (partial overlays) |
| `lib/look/look_palette.dart` | Full resolved colors |
| `lib/look/look_materials.dart` | Full resolved materials |
| `lib/look/resolved_look.dart` | id/name/author + palette + materials + font family names |
| `lib/look/builtin_look.dart` | Embedded builtin manifest + resolved defaults |
| `lib/look/look_parser.dart` | JSON → `LookManifest` with validation |
| `lib/look/look_merger.dart` | Extends-chain deep merge → `ResolvedLook` (sans font load) |
| `lib/look/look_catalog.dart` | Scan looks directory → installed manifests |
| `lib/look/look_installer.dart` | Install directory/zip; conflict callback |
| `lib/look/look_font_loader.dart` | Load TTF/OTF bytes into Flutter |
| `lib/look/look_controller.dart` | Active look, directory, activate/install/rescan |
| `lib/theme/look_scope.dart` | `InheritedWidget` for `ResolvedLook` |
| `lib/ui/chrome/look_pack_dialog.dart` | Minimal UI: list looks, set directory, install zip |
| `test/look/*.dart` | Unit tests per module |
| `test/look/fixtures/` | Sample `look.json` trees |

**Modified:**

| Path | Change |
|------|--------|
| `pubspec.yaml` | Add `archive` |
| `lib/domain/tramp_settings.dart` | `activeLookId`, `looksDirectory` |
| `test/domain/tramp_settings_test.dart` | Round-trip new fields |
| `lib/theme/tramp_colors.dart` / `tramp_text.dart` | Prefer `LookScope` / accept palette + family names |
| `lib/ui/chrome/mockup/*`, spectrum, EQ painters | Read from `LookScope` / injected palette |
| `lib/ui/session/session_messages.dart` | `LookSnapshotEvent` |
| `lib/ui/session/session_host.dart` | Own `LookController`; broadcast look |
| `lib/ui/session/session_client.dart` | Apply look snapshot + `LookScope` |
| `lib/ui/main_player/mockup_main_player.dart` | Clutter **O** opens look-pack dialog (or sub-entry under options) |
| `docs/architecture.md` | Theme/tokens status → implemented for look packs |
| Spec status line | Designed → Implemented |

---

### Task 1: Parse and validate `look.json`

**Files:**
- Create: `lib/look/look_id.dart`
- Create: `lib/look/look_manifest.dart`
- Create: `lib/look/look_parser.dart`
- Create: `test/look/look_parser_test.dart`
- Create: `test/look/fixtures/neon-cyan/look.json`

**Interfaces:**
- Consumes: none
- Produces:
  - `bool isValidLookId(String id)`
  - `class LookManifest` with `formatVersion`, `id`, `name`, `author?`, `extendsId`, partial `colors`/`materials`/`fonts` maps
  - `LookManifest LookParser.parse(Map<String, dynamic> json)` throws `FormatException` on invalid

- [ ] **Step 1: Write the failing test**

```dart
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/look_parser.dart';

void main() {
  test('parses partial overlay', () {
    final m = LookParser.parse({
      'formatVersion': 1,
      'id': 'neon-cyan',
      'name': 'Neon Cyan',
      'author': 'Example',
      'extends': 'builtin',
      'colors': {
        'phosphor': {'default': '#3de7ff'},
      },
    });
    expect(m.id, 'neon-cyan');
    expect(m.extendsId, 'builtin');
    expect(m.colors['phosphor'], isNotNull);
  });

  test('rejects bad id', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'com.example.neon',
        'name': 'X',
        'extends': 'builtin',
      }),
      throwsFormatException,
    );
  });

  test('rejects unknown color key', () {
    expect(
      () => LookParser.parse({
        'formatVersion': 1,
        'id': 'neon-cyan',
        'name': 'X',
        'extends': 'builtin',
        'colors': {'neon': '#fff'},
      }),
      throwsFormatException,
    );
  });
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `flutter test test/look/look_parser_test.dart`  
Expected: FAIL (library not found)

- [ ] **Step 3: Implement id helper, manifest, parser**

`look_id.dart`:

```dart
final lookIdPattern = RegExp(r'^[a-z0-9]+(-[a-z0-9]+)*$');

bool isValidLookId(String id) =>
    id != 'builtin' && lookIdPattern.hasMatch(id);

bool isReservedLookId(String id) => id == 'builtin';
```

Parser rules (throw `FormatException` with clear message):
- `formatVersion == 1`
- `id` matches slug (allow `builtin` only when parsing the embedded builtin map in Task 2 — for disk packs use `isValidLookId`)
- require `name` (non-empty string), `extends` (non-empty string)
- `author` optional string
- `colors`: only known nested paths from the spec; values `#RRGGBB` or `#RRGGBBAA`
- `materials`: only `bevel.lightOpacity` / `bevel.softOpacity` (num 0–1), `spectrum.stops` / `rail.stops` (non-empty list of color strings)
- `fonts`: only `chrome` / `lcd`; each `{file: string ending .ttf|.otf, weight?: int}`
- Ignore unknown **top-level** keys; reject unknown keys under `colors` / `materials` / `fonts`

Store partial overlays as nested `Map<String, dynamic>` (already validated) on `LookManifest` for the merger.

- [ ] **Step 4: Run tests to verify they pass**

Run: `flutter test test/look/look_parser_test.dart`  
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add lib/look/look_id.dart lib/look/look_manifest.dart lib/look/look_parser.dart test/look/look_parser_test.dart test/look/fixtures
git commit -m "Add look.json parser and id validation for look packs."
```

---

### Task 2: Builtin look + merge to `ResolvedLook`

**Files:**
- Create: `lib/look/look_palette.dart`
- Create: `lib/look/look_materials.dart`
- Create: `lib/look/resolved_look.dart`
- Create: `lib/look/builtin_look.dart`
- Create: `lib/look/look_merger.dart`
- Create: `test/look/look_merger_test.dart`
- Modify: `lib/theme/mockup_tokens.dart` (only if needed to export shared hex helpers — prefer reading existing consts)

**Interfaces:**
- Consumes: `LookManifest`, `LookParser`
- Produces:
  - `class LookPalette` — all 15 colors as `Color` with readable names (`shellHighlight`, `shellBase`, `shellMid`, `shellLow`, `shellDeep`, `inkDefault`, `inkDim`, `inkFaint`, `phosphorDefault`, `phosphorHot`, `phosphorDim`, `phosphorDeep`, `accentDefault`, `accentDim`, `well`)
  - `class LookMaterials` — `bevelLightOpacity`, `bevelSoftOpacity`, `spectrumStops`, `railStops`
  - `class ResolvedLook` — `id`, `name`, `author?`, `palette`, `materials`, `chromeFamily`, `lcdFamily` (families set later by font loader; merger sets builtin names)
  - `LookManifest BuiltinLook.manifest`
  - `ResolvedLook LookMerger.resolve({required String activeId, required Map<String, LookManifest> installed})` where `installed` need not contain builtin

Builtin materials (from mockup):

```dart
bevelLightOpacity: 0.15,
bevelSoftOpacity: 0.06,
spectrumStops: [Color(0xFFCBF9FF), Color(0xFF3DE7FF), Color(0xFF1B9EC4), Color(0xFFFF3D9A)],
railStops: [Color(0xFF1A7A88), Color(0xFF8A2258), Color(0xFF1A7A88)],
```

- [ ] **Step 1: Write failing merger tests**

```dart
test('partial overlay keeps builtin shell colors', () {
  final neon = LookParser.parse({/* phosphor + accent only, extends builtin */});
  final resolved = LookMerger.resolve(
    activeId: 'neon-cyan',
    installed: {'neon-cyan': neon},
  );
  expect(resolved.palette.shellHighlight, MockupTokens.shellHi);
  expect(resolved.palette.phosphorDefault, const Color(0xFF3DE7FF));
});

test('stops replace wholly', () { /* overlay spectrum.stops with 2 colors; length == 2 */ });

test('rejects cycle', () {
  // a extends b, b extends a
  expect(() => LookMerger.resolve(...), throwsFormatException);
});

test('rejects chain deeper than 8', () { /* chain of 9 */ });
```

- [ ] **Step 2: Run test to verify it fails**

Run: `flutter test test/look/look_merger_test.dart`  
Expected: FAIL

- [ ] **Step 3: Implement palette, materials, builtin, merger**

Merge algorithm:
1. Start at `activeId`; walk `extendsId` collecting manifests until `builtin` (use `BuiltinLook.manifest` when id is `builtin`).
2. Detect cycles with a `Set` of visited ids; if length > 8 throw.
3. Fold from root to leaf: deep-merge maps; for `stops` replace list; opacities overwrite.
4. Convert merged maps → `LookPalette` / `LookMaterials` (parse hex to `Color`).
5. Set `chromeFamily` / `lcdFamily` to `TrampCondensed` / `TrampMono` for now (font loader may replace).

Provide `LookPalette.toJson` / `fromJson` for session snapshots (use `#AARRGGBB` or `#RRGGBB` consistently — pin `#RRGGBB` for opaque, `#RRGGBBAA` when alpha ≠ FF).

- [ ] **Step 4: Run tests**

Run: `flutter test test/look/look_merger_test.dart`  
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add lib/look/ test/look/look_merger_test.dart
git commit -m "Resolve look packs via extends merge onto builtin."
```

---

### Task 3: Settings fields + looks catalog

**Files:**
- Modify: `lib/domain/tramp_settings.dart`
- Modify: `test/domain/tramp_settings_test.dart`
- Create: `lib/look/look_catalog.dart`
- Create: `test/look/look_catalog_test.dart`

**Interfaces:**
- Consumes: `LookParser`, `isValidLookId`
- Produces:
  - `TrampSettings.activeLookId` (`String`, default `'builtin'`)
  - `TrampSettings.looksDirectory` (`String?`, default `null`)
  - `class LookCatalog` with `Future<Map<String, LookManifest>> scan(Directory looksDir)`
  - Static helper `Future<Directory> defaultLooksDirectory(Future<Directory> Function() supportDir)` → `supportDir()/looks`

- [ ] **Step 1: Failing settings + catalog tests**

Settings: round-trip `activeLookId` / `looksDirectory`; invalid/missing → defaults.

Catalog (use `Directory.systemTemp.createTempSync`):
- Empty dir → empty map
- Valid `<id>/look.json` with matching id → one entry
- Mismatched folder name vs `id` → skip or throw; **pin: skip invalid pack and continue** (catalog must not crash startup); collect errors on a side list `List<String> warnings` returned with results — use a small result type:

```dart
class LookCatalogResult {
  const LookCatalogResult(this.manifests, this.warnings);
  final Map<String, LookManifest> manifests;
  final List<String> warnings;
}
```

- [ ] **Step 2: Run tests — expect FAIL**

- [ ] **Step 3: Implement settings fields and catalog**

Update `TrampSettings` constructor, `defaults`, `copyWith`, `toJson`, `fromJson`:
- `activeLookId`: string; if missing/invalid use `'builtin'` (allow the value `builtin`)
- `looksDirectory`: string or null; empty string → null

`LookCatalog.scan`:
- List subdirectories; for each, read `look.json`; parse; if `manifest.id != dirname` add warning and skip; else add to map.

- [ ] **Step 4: Run tests — expect PASS**

- [ ] **Step 5: Commit**

```bash
git add lib/domain/tramp_settings.dart test/domain/tramp_settings_test.dart lib/look/look_catalog.dart test/look/look_catalog_test.dart
git commit -m "Persist active look id and scan the looks directory."
```

---

### Task 4: Installer (folder + zip) with Replace/Cancel

**Files:**
- Modify: `pubspec.yaml` (add `archive`)
- Create: `lib/look/look_installer.dart`
- Create: `test/look/look_installer_test.dart`

**Interfaces:**
- Consumes: `LookParser`, `LookCatalog`
- Produces:

```dart
enum LookConflictChoice { replace, cancel }

class LookConflict {
  const LookConflict({
    required this.id,
    required this.installedName,
    required this.installedAuthor,
    required this.incomingName,
    required this.incomingAuthor,
  });
  final String id;
  final String installedName;
  final String? installedAuthor;
  final String incomingName;
  final String? incomingAuthor;
}

class LookInstaller {
  LookInstaller({required this.looksDir, required this.onConflict});
  final Directory looksDir;
  final Future<LookConflictChoice> Function(LookConflict conflict) onConflict;

  /// Install from an extracted pack directory or a directory the user chose.
  Future<bool> installDirectory(Directory source);

  /// Install from a .zip whose root contains look.json or a single top folder with look.json.
  Future<bool> installZip(File zipFile);
}
```

Return `true` if installed, `false` if cancelled.

- [ ] **Step 1: Failing installer tests** (temp dirs; fake `onConflict`)

Cases:
1. Fresh install copies `look.json` (+ `fonts/` if present) to `looksDir/id/`
2. Conflict + `cancel` → existing files unchanged
3. Conflict + `replace` → incoming overwrites
4. Zip with root `look.json` installs under `id/`
5. Zip with single root folder `neon-cyan/look.json` installs
6. Reject `builtin` as install id

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement installer + add dependency**

```yaml
# pubspec.yaml dependencies:
archive: ^4.0.2
```

Run: `flutter pub get`

Implementation notes:
- Parse source `look.json` first to learn `id` / name / author before copying.
- If `Directory(p.join(looksDir.path, id)).existsSync()`, build `LookConflict` from existing manifest (if readable) vs incoming; await `onConflict`; on cancel return false; on replace delete target then copy.
- Copy with `path` package; create recursive dirs.
- Zip: use `ZipDecoder` from `archive`; find `look.json` entry; determine pack root prefix; extract only under that prefix into temp, then `installDirectory`.

- [ ] **Step 4: Run — expect PASS**

- [ ] **Step 5: Commit**

```bash
git add pubspec.yaml pubspec.lock lib/look/look_installer.dart test/look/look_installer_test.dart
git commit -m "Install look packs from folders or zips with replace prompt."
```

---

### Task 5: Font loader + `LookController`

**Files:**
- Create: `lib/look/look_font_loader.dart`
- Create: `lib/look/look_controller.dart`
- Create: `test/look/look_controller_test.dart`
- Test: reuse fixture fonts or skip byte load with a fake loader seam

**Interfaces:**
- Consumes: catalog, merger, installer, settings store, font loader
- Produces:

```dart
typedef FontBytesLoader = Future<void> Function({
  required String family,
  required Uint8List bytes,
  required int weight,
});

class LookFontLoader {
  LookFontLoader({FontBytesLoader? load});
  Future<String> ensureFamily({
    required String packId,
    required String role, // chrome | lcd
    required File file,
    required int weight,
  }); // returns family name Look.<id>.<role>
}

class LookController extends ChangeNotifier {
  ResolvedLook get resolved;
  String get activeLookId;
  Directory get looksDirectory;
  List<LookManifest> get installed; // includes synthetic builtin entry for UI
  String? get lastError;

  Future<void> bootstrap({
    required TrampSettings settings,
    required Future<Directory> Function() supportDir,
  });
  Future<void> setLooksDirectory(String? absolutePath); // null = default
  Future<void> activate(String id);
  Future<bool> installZip(File zip, {required Future<LookConflictChoice> Function(LookConflict) onConflict});
  Future<bool> installDirectory(Directory dir, {required Future<LookConflictChoice> Function(LookConflict) onConflict});
}
```

- [ ] **Step 1: Failing controller tests** (memory settings + temp looks dir + fake font loader)

1. `bootstrap` with missing active id → resolves builtin, persists `activeLookId: builtin` via a `void Function(TrampSettings)` save callback
2. `setLooksDirectory` to empty temp → active missing → falls back to builtin
3. `activate('neon-cyan')` after writing fixture → palette overlay applied; font loader called when fonts present

Pin constructor:

```dart
LookController({
  required SettingsStore settingsStore,
  required Future<Directory> Function() supportDir,
  LookFontLoader? fontLoader,
});
```

On any successful activate/directory change, write settings through `settingsStore`.

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement font loader + controller**

`LookFontLoader` default uses:

```dart
final loader = FontLoader(family);
loader.addFont(ByteData.sublistView(bytes));
await loader.load();
```

Controller flow for `activate` / rescan:
1. Scan catalog at current looks dir
2. `LookMerger.resolve`
3. For each font role overridden in the **leaf** (or merged font file paths — pin: use the nearest file in the chain from leaf to root), load bytes and set family names on a copy of `ResolvedLook`
4. On failure set `lastError`, keep previous `resolved`, do not update `activeLookId`
5. On success update `resolved`, `activeLookId`, notifyListeners, persist settings

`setLooksDirectory`:
1. Persist path (null if default)
2. Rescan + resolve active; if active not in catalog and not builtin → `activate('builtin')`

- [ ] **Step 4: Run — expect PASS**

- [ ] **Step 5: Commit**

```bash
git add lib/look/look_font_loader.dart lib/look/look_controller.dart test/look/look_controller_test.dart
git commit -m "Add LookController with font loading and directory fallback."
```

---

### Task 6: `LookScope` + wire chrome to resolved palette/materials/fonts

**Files:**
- Create: `lib/theme/look_scope.dart`
- Modify: `lib/theme/tramp_text.dart`
- Modify: `lib/theme/tramp_colors.dart`
- Modify: mockup chrome / spectrum / EQ band fill painters that use `MockupTokens`, phosphor, accent, spectrum stops, bevel opacities, or hard-coded font families
- Modify: window roots in `session_host.dart` / `session_client.dart` (or `main_player_window.dart` etc.) to wrap with `LookScope`
- Test: update any token tests; add `test/theme/look_scope_test.dart` (widget test: child sees overlay color)

**Interfaces:**
- Consumes: `ResolvedLook`
- Produces: `LookScope.of(BuildContext context) → ResolvedLook`

- [ ] **Step 1: Failing widget test**

```dart
testWidgets('LookScope supplies palette', (tester) async {
  final look = BuiltinLook.resolved; // or neon overlay
  await tester.pumpWidget(
    LookScope(
      look: look,
      child: Builder(
        builder: (context) {
          final c = LookScope.of(context).palette.phosphorDefault;
          return ColoredBox(color: c, key: const Key('swatch'));
        },
      ),
    ),
  );
  final box = tester.widget<ColoredBox>(find.byKey(const Key('swatch')));
  expect(box.color, look.palette.phosphorDefault);
});
```

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement scope and migrate call sites**

Migration rules:
- Widgets with `BuildContext`: `final look = LookScope.of(context);` then use `look.palette.*` / `look.materials.*` / `look.chromeFamily` / `look.lcdFamily`.
- `CustomPainter` subclasses: add `LookPalette palette` + `LookMaterials materials` fields; parents pass `LookScope.of(context)`.
- `TrampText`: change static styles to builders, e.g. `TrampText.lcd(ResolvedLook look)` or read families from look; keep sizes/weights.
- `TrampColors`: map legacy names to `LookPalette` fields via `TrampColors.of(ResolvedLook look)` **or** deprecate static consts in favor of palette (update call sites in same task).
- Hard-coded spectrum gradient stops in `spectrum_visualizer.dart` / EQ fill → `materials.spectrumStops`.
- Bevel paints that use 0.15 / 0.06 opacities on ink → `materials.bevelLightOpacity` / `bevelSoftOpacity`.
- Leave truly one-off decorative literals that are not in the spec (e.g. close-button pink face) unchanged unless they already use tokens.

Default: if `LookScope` missing in tests, provide a test helper that wraps with `BuiltinLook.resolved` (update `test/support` harness).

- [ ] **Step 4: Run look_scope test + affected chrome/golden tests**

Run: `flutter test test/theme/look_scope_test.dart test/theme/ test/ui/chrome/ test/ui/playlist/mockup_playlist_golden_test.dart`  
Expected: PASS (update goldens only if pixel diff from intentional token wiring — builtin must match previous colors)

- [ ] **Step 5: Commit**

```bash
git add lib/theme/look_scope.dart lib/theme/tramp_text.dart lib/theme/tramp_colors.dart lib/ui/chrome lib/ui/main_player lib/ui/equalizer lib/ui/playlist test/theme test/support
git commit -m "Drive mockup chrome colors and fonts from LookScope."
```

---

### Task 7: Session bus look snapshot + host/client wiring

**Files:**
- Modify: `lib/ui/session/session_messages.dart`
- Modify: `test/ui/session/` (existing codec tests if any; else create `test/ui/session/look_snapshot_event_test.dart`)
- Modify: `lib/ui/session/session_host.dart`
- Modify: `lib/ui/session/session_client.dart`

**Interfaces:**
- Consumes: `ResolvedLook`, `LookController`
- Produces: `LookSnapshotEvent` with JSON payload of palette, materials, font family names, id/name/author

- [ ] **Step 1: Failing codec test** for `LookSnapshotEvent` round-trip

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement event + wiring**

Host:
- Construct `LookController` beside settings store; `await lookController.bootstrap(...)` during startup after settings read.
- Wrap main window tree in `ListenableBuilder` / `AnimatedBuilder` on controller → `LookScope(look: controller.resolved, ...)`.
- On `lookController` changes, `_broadcast(LookSnapshotEvent.fromResolved(controller.resolved))`.
- Include look snapshot in the initial sync when a client sends `ClientReadyCommand` (same place other snapshots are pushed).

Client:
- Hold `ResolvedLook _look = BuiltinLook.resolved`.
- On `LookSnapshotEvent`, replace `_look` and rebuild; wrap EQ/PL trees in `LookScope`.

Do **not** send font file bytes over the bus — secondary engines use the same family names; host must load fonts in the main engine only. **Pin:** also load fonts on secondary isolates/engines when snapshot arrives if families are non-builtin — secondary Flutter engines do not share `FontLoader` registrations.

Secondary font load pin:
- `LookSnapshotEvent` includes optional `fontFiles` map: `{ "chrome": "<absolute path>", "lcd": "<absolute path>" }` for overridden roles (paths under looks dir).
- Client calls `LookFontLoader` on those paths when snapshot applied.

- [ ] **Step 4: Run session + look tests — expect PASS**

- [ ] **Step 5: Commit**

```bash
git add lib/ui/session lib/look test/ui/session
git commit -m "Broadcast resolved look packs to secondary windows."
```

---

### Task 8: Look pack dialog UI (select, directory, install)

**Files:**
- Create: `lib/ui/chrome/look_pack_dialog.dart`
- Modify: `lib/ui/main_player/mockup_main_player.dart` (clutter **O** or about/options entry — **pin: clutter O opens a small menu with “Look packs…”** if O already opens options; otherwise add Look packs button in the existing options surface)
- Create: `test/ui/chrome/look_pack_dialog_test.dart`

**Interfaces:**
- Consumes: `LookController`, `file_picker`
- Produces: dialog actions that call controller methods

- [ ] **Step 1: Failing widget test** with fake controller / pumped dialog

Assert: list shows Builtin + installed names; tapping a row calls `activate`; conflict path can be tested by injecting installer callback that the dialog uses for `showDialog` Replace/Cancel.

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement dialog**

UI (keep minimal, mockup-adjacent dark chrome — no Material marketing layout):
- List of looks: `name` (and author if present); highlight active
- Buttons: **Install look…** (`FilePicker` zip or folder — pin: zip via `FileType.custom` extensions `zip`, plus a “Install folder…” that uses `getDirectoryPath`)
- **Looks folder…** → `FilePicker.platform.getDirectoryPath()` → `setLooksDirectory`
- **Reset folder** → `setLooksDirectory(null)`
- On conflict: `AlertDialog` with installed vs incoming name/author and Replace / Cancel

Wire clutter **O**: if it already opens options/about, add a “Look packs…” entry there; if O is a stub, open this dialog directly.

- [ ] **Step 4: Run dialog test + analyze**

Run: `flutter test test/ui/chrome/look_pack_dialog_test.dart` && `flutter analyze`  
Expected: PASS / clean

- [ ] **Step 5: Commit**

```bash
git add lib/ui/chrome/look_pack_dialog.dart lib/ui/main_player/mockup_main_player.dart test/ui/chrome/look_pack_dialog_test.dart
git commit -m "Add look pack dialog for install, folder, and activation."
```

---

### Task 9: Docs status + architecture map

**Files:**
- Modify: `docs/superpowers/specs/2026-08-09-look-pack-format-design.md` (Status → Implemented)
- Modify: `docs/architecture.md` (Theme/tokens row: look packs implemented; mention `lib/look/` + `LookController`)
- Modify: `CONTEXT.md` only if wording drifted (usually no change)

- [ ] **Step 1: Update status and architecture to match code**

- [ ] **Step 2: Commit**

```bash
git add docs/superpowers/specs/2026-08-09-look-pack-format-design.md docs/architecture.md
git commit -m "Mark look packs implemented in architecture and spec."
```

---

## Spec coverage (self-review)

| Spec requirement | Task |
|------------------|------|
| Human-readable color keys / mapping table | 1–2, 6 |
| Materials bevel + spectrum/rail stops | 1–2, 6 |
| Fonts chrome/lcd TTF/OTF | 1, 5–7 |
| `extends` merge, depth 8, cycles | 2 |
| Unknown nested keys reject | 1 |
| Folder or zip same layout | 4 |
| Default looks dir under app support | 3, 5 |
| Custom looks directory + reload + builtin fallback | 5, 8 |
| Friendly slug ids, no reverse-DNS | 1 |
| Replace/Cancel conflict | 4, 8 |
| Active look persisted; all windows | 3, 5, 7 |
| Builtin embedded | 2 |
| Out of scope items not built | — |

## Placeholder / consistency check

- Types aligned: `LookManifest` → `LookMerger` → `ResolvedLook` → `LookScope` / `LookSnapshotEvent`.
- Settings field names: `activeLookId`, `looksDirectory`.
- Conflict enum: `LookConflictChoice.replace|cancel`.
- No TBD steps remain.
