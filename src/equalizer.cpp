#include "equalizer.h"

#include <QPair>
#include <cmath>

namespace aoide {
namespace {

QString formatDb(double db) {
  const double rounded = std::round(db);
  if (db == rounded) {
    return QString::number(int(rounded));
  }
  QString text = QString::number(db, 'f', 2);
  if (text.contains(QLatin1Char('.'))) {
    while (text.endsWith(QLatin1Char('0'))) {
      text.chop(1);
    }
    if (text.endsWith(QLatin1Char('.'))) {
      text.chop(1);
    }
  }
  return text;
}

}  // namespace

double EqualizerSettings::clampGain(double value) {
  if (value < -kGainLimit) {
    return -kGainLimit;
  }
  if (value > kGainLimit) {
    return kGainLimit;
  }
  return value;
}

EqualizerSettings EqualizerSettings::withGain(int band, double gain) const {
  if (band < 0 || band >= kBandCount) {
    return *this;
  }
  EqualizerSettings next = *this;
  next.gains[size_t(band)] = clampGain(gain);
  next.presetName.clear();
  return next;
}

EqualizerSettings EqualizerSettings::withPreamp(double value) const {
  EqualizerSettings next = *this;
  next.preamp = clampGain(value);
  return next;
}

EqualizerSettings EqualizerSettings::withPreset(const QString& name,
                                                const QVector<double>& values) const {
  EqualizerSettings next = *this;
  next.presetName = name;
  for (int i = 0; i < kBandCount && i < values.size(); ++i) {
    next.gains[size_t(i)] = clampGain(values[i]);
  }
  return next;
}

const QVector<QPair<QString, QVector<double>>>& EqualizerPresets::builtIn() {
  static const QVector<QPair<QString, QVector<double>>> kPresets = {
      {QStringLiteral("Flat"), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
      {QStringLiteral("Rock"), {5, 4, 2, -1, -2, 1, 3, 5, 5, 5}},
      {QStringLiteral("Pop"), {-1, 2, 4, 5, 3, 0, -1, -1, -1, -1}},
      {QStringLiteral("Jazz"), {4, 3, 1, 2, -1, -1, 0, 2, 3, 4}},
      {QStringLiteral("Classical"), {5, 4, 3, 2, -1, -1, 0, 2, 3, 4}},
      {QStringLiteral("Bass Boost"), {9, 7, 5, 2, 0, 0, 0, 0, 0, 0}},
      {QStringLiteral("Treble Boost"), {0, 0, 0, 0, 0, 2, 5, 7, 8, 8}},
      {QStringLiteral("Vocal"), {-2, -1, 2, 4, 5, 4, 2, 0, -1, -2}},
  };
  return kPresets;
}

QString buildEqualizerAf(const EqualizerSettings& settings) {
  if (!settings.enabled) {
    return {};
  }
  QStringList stages;
  stages << QStringLiteral("volume=%1dB").arg(formatDb(settings.preamp));
  const int count = EqualizerSettings::kBandCount;
  for (int i = 0; i < count; ++i) {
    stages << QStringLiteral("equalizer=f=%1:t=o:w=1:g=%2")
                  .arg(EqualizerSettings::kBandFrequencies[size_t(i)])
                  .arg(formatDb(settings.gains[size_t(i)]));
  }
  return QStringLiteral("lavfi=[%1]").arg(stages.join(QLatin1Char(',')));
}

}  // namespace aoide
