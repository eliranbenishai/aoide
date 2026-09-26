# Regular chrome fonts

These regular faces let editable group names, their title, and the autosave
hint keep the selected skin's typeface without borrowing its bold chrome weight.
Existing bold font assets remain unchanged. Custom skins without a regular
upright face use the bundled Barlow Semi Condensed Regular fallback.

All sources are pinned to Google Fonts revision
`23e54b51ddffbc7713c583748e3bd86f62b1fa4a`. Each font is distributed under its adjacent linked SIL
Open Font License. The static Saira font is unmodified, preserving its reserved
font name.

| Bundled file | Source | Processing | License |
| --- | --- | --- | --- |
| `BarlowSemiCondensed-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/barlowsemicondensed/BarlowSemiCondensed-Regular.ttf) | Unmodified upstream static font. | [OFL](licenses/BarlowSemiCondensed-OFL.txt) |
| `Rajdhani-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/rajdhani/Rajdhani-Regular.ttf) | Unmodified upstream static font. | [OFL](licenses/Rajdhani-OFL.txt) |
| `ArchivoNarrow-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/archivonarrow/ArchivoNarrow%5Bwght%5D.ttf) | Instantiated at `wght=400`. | [OFL](licenses/ArchivoNarrow-OFL.txt) |
| `CormorantGaramond-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/cormorantgaramond/CormorantGaramond%5Bwght%5D.ttf) | Instantiated at `wght=400`. | [OFL](licenses/CormorantGaramond-OFL.txt) |
| `Oswald-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/oswald/Oswald%5Bwght%5D.ttf) | Instantiated at `wght=400`. | [OFL](licenses/Oswald-OFL.txt) |
| `Cinzel-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/cinzel/Cinzel%5Bwght%5D.ttf) | Instantiated at `wght=400`. | [OFL](licenses/Cinzel-OFL.txt) |
| `SairaCondensed-Regular.ttf` | [Google Fonts](https://github.com/google/fonts/blob/23e54b51ddffbc7713c583748e3bd86f62b1fa4a/ofl/sairacondensed/SairaCondensed-Regular.ttf) | Unmodified upstream static font. | [OFL](licenses/SairaCondensed-OFL.txt) |

Variable sources were instantiated offline with fontTools 4.66.0;
fontTools is not an application or build dependency. These four source licenses
do not declare reserved font names. Their regular outlines and kerning come from
the upstream interpolation data, without manual glyph changes.

To reproduce each variable instance:

```python
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

font = TTFont(source_path)
font = instantiateVariableFont(
    font, {"wght": 400}, inplace=True, optimize=True, updateFontNames=True
)
font.save(destination_path)
```
