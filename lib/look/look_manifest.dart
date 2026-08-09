class LookManifest {
  const LookManifest({
    required this.formatVersion,
    required this.id,
    required this.name,
    required this.extendsId,
    this.author,
    this.colors = const {},
    this.materials = const {},
    this.fonts = const {},
  });

  final int formatVersion;
  final String id;
  final String name;
  final String? author;
  final String extendsId;
  final Map<String, dynamic> colors;
  final Map<String, dynamic> materials;
  final Map<String, dynamic> fonts;
}
