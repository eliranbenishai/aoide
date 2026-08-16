#pragma once

#include <QColor>

namespace tramp {

// Palette from player-mockup-2.html `:root`. Channel bytes are the CSS hex.

inline const QColor kShellHi{0x32, 0x37, 0x44};
inline const QColor kShell{0x26, 0x2b, 0x38};
inline const QColor kShellMid{0x1a, 0x1d, 0x26};
inline const QColor kShellLo{0x12, 0x14, 0x1a};
inline const QColor kShellDeep{0x0a, 0x0b, 0x0e};

inline const QColor kInk{0xe8, 0xea, 0xf0};
inline const QColor kInkDim{0x8b, 0x91, 0x9e};
inline const QColor kInkFaint{0x5b, 0x62, 0x70};

inline const QColor kPhos{0x3d, 0xe7, 0xff};
inline const QColor kPhosHot{0xb8, 0xf6, 0xff};
inline const QColor kPhosDim{0x1a, 0x7a, 0x88};
inline const QColor kPhosDeep{0x0d, 0x3d, 0x46};

inline const QColor kAccent{0xff, 0x3d, 0x9a};
inline const QColor kAccentDim{0x8a, 0x22, 0x58};

inline const QColor kWell{0x05, 0x06, 0x08};

// Chrome literals from the mockup stylesheet (`.tbar`, `.wbtn`, `.wordmark`).

inline const QColor kTitleBar0{0x3c, 0x43, 0x56};
inline const QColor kTitleBar26{0x2c, 0x32, 0x41};
inline const QColor kTitleBar62{0x1d, 0x22, 0x2c};
inline const QColor kTitleBar100{0x12, 0x15, 0x1c};

inline const QColor kWordmark{0xea, 0xf2, 0xff};
inline const QColor kWindowName{200, 214, 235, 140};  // rgba(200,214,235,0.55)
inline const QColor kCoolSheen{0xe2, 0xec, 0xff};
inline const QColor kLogoDisc{0xe9, 0xec, 0xf4};

inline const QColor kWbtn0{0x45, 0x4d, 0x60};
inline const QColor kWbtn55{0x2f, 0x35, 0x43};
inline const QColor kWbtn100{0x20, 0x24, 0x2e};
inline const QColor kWbtnClose0{0x9c, 0x2a, 0x60};
inline const QColor kWbtnClose55{0x79, 0x20, 0x4a};
inline const QColor kWbtnClose100{0x4a, 0x11, 0x29};
inline const QColor kGlyphInk{214, 226, 245, 209};  // rgba(214,226,245,0.82)
inline const QColor kCloseGlyph{0xff, 0xd6, 0xe8};

inline const QColor kBevelLight{226, 236, 255, 38};  // 0.15
inline const QColor kBevelSoft{226, 236, 255, 15};   // 0.06

}  // namespace tramp
