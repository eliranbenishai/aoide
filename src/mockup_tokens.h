#pragma once

#include <QColor>

namespace aoide {

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

// Derived LookPaint literals on the builtin palette.
inline const QColor kBtnIdle0{0x3f, 0x46, 0x57};
inline const QColor kBtnIdle48{0x2b, 0x31, 0x3e};
inline const QColor kBtnIdle100{0x1e, 0x22, 0x2c};
inline const QColor kBtnOn0{0xa3, 0xf4, 0xff};
inline const QColor kBtnOnInk{0x04, 0x22, 0x2b};
inline const QColor kBtnOnLip{0xf0, 0xfd, 0xff};
inline const QColor kBtnOnFoot{0x05, 0x46, 0x58};
inline const QColor kBtnLabelIdle{196, 210, 232, 184};
inline const QColor kSliderFillHi{0xcb, 0xf9, 0xff};
inline const QColor kSliderFillLo{0x0f, 0x7f, 0x96};
inline const QColor kSpectrum0{0xcb, 0xf9, 0xff};
inline const QColor kSpectrum2{0x1b, 0x9e, 0xc4};
inline const QColor kPlateFace{0x1e, 0x22, 0x2c};
inline const QColor kHoverLift{0xe8, 0xf0, 0xff};
inline const QColor kIdleLedHi{0x3d, 0x43, 0x50};
inline const QColor kIdleLedLo{0x22, 0x26, 0x2f};
inline const QColor kAccentHot{0xff, 0xd6, 0xea};
inline const QColor kLitLedRim{0x5a, 0x0f, 0x32};
inline const QColor kCurveStroke{0x8d, 0xf2, 0xff};

/// Playlist list CRT wash from the mockup `.list` radial (builtin well lifts).
inline const QColor kListWash0{0x0f, 0x1c, 0x2a};
inline const QColor kListWash1{0x07, 0x10, 0x18};
inline const QColor kListWash2{0x04, 0x07, 0x0c};

}  // namespace aoide
