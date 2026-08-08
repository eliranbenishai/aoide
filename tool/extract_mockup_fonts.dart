// One-shot: decode the two mockup @font-face TTFs into assets/fonts/.
//
// Run from the package root:
//   dart run tool/extract_mockup_fonts.dart
import 'dart:convert';
import 'dart:io';

void main(List<String> args) {
  final root = Directory.current;
  final htmlPath = args.isNotEmpty ? args.first : 'player-mockup-2.html';
  final htmlFile = File(htmlPath);
  if (!htmlFile.existsSync()) {
    stderr.writeln('Missing $htmlPath (run from package root).');
    exitCode = 1;
    return;
  }

  final html = htmlFile.readAsStringSync();
  final blobPattern = RegExp(r'data:font/ttf;base64,([A-Za-z0-9+/=]+)');
  final blobs = blobPattern
      .allMatches(html)
      .map((m) => m.group(1)!)
      .toList(growable: false);

  if (blobs.length < 2) {
    stderr.writeln(
      'Expected ≥2 data:font/ttf;base64 blobs in $htmlPath, found ${blobs.length}.',
    );
    exitCode = 1;
    return;
  }

  // Mockup order: Tramp Condensed (Bold), then Tramp Mono (Medium).
  final outDir = Directory('assets/fonts')..createSync(recursive: true);
  final targets = <String, String>{
    'TrampCondensed-Bold.ttf': blobs[0],
    'TrampMono-Medium.ttf': blobs[1],
  };

  for (final entry in targets.entries) {
    final bytes = base64.decode(entry.value);
    final out = File('${outDir.path}/${entry.key}');
    out.writeAsBytesSync(bytes);
    stdout.writeln('Wrote ${out.path} (${bytes.length} bytes)');
  }

  stdout.writeln('Done (cwd=${root.path}).');
}
