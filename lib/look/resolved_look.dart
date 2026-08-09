import 'look_materials.dart';
import 'look_palette.dart';

class ResolvedLook {
  const ResolvedLook({
    required this.id,
    required this.name,
    required this.palette,
    required this.materials,
    required this.chromeFamily,
    required this.lcdFamily,
    this.author,
  });

  final String id;
  final String name;
  final String? author;
  final LookPalette palette;
  final LookMaterials materials;
  final String chromeFamily;
  final String lcdFamily;
}
