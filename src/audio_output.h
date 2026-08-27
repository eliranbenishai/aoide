#pragma once

#include <QString>
#include <QVector>

namespace tramp {

struct AudioOutputDevice {
  QString name;
  QString description;
};

inline QString kDefaultAudioDeviceName() { return QStringLiteral("auto"); }

inline QString normalizeAudioDeviceName(const QString& name) {
  const QString trimmed = name.trimmed();
  return trimmed.isEmpty() ? kDefaultAudioDeviceName() : trimmed;
}

inline QString audioDeviceDisplayLabel(const QString& name,
                                       const QVector<AudioOutputDevice>& devices) {
  const QString id = normalizeAudioDeviceName(name);
  if (id == kDefaultAudioDeviceName()) return QStringLiteral("Auto");
  for (const AudioOutputDevice& device : devices) {
    if (normalizeAudioDeviceName(device.name) != id) continue;
    return device.description.trimmed().isEmpty() ? device.name : device.description;
  }
  return name.trimmed().isEmpty() ? id : name;
}

inline QVector<AudioOutputDevice> withAutoAudioDevice(QVector<AudioOutputDevice> devices) {
  const QString autoName = kDefaultAudioDeviceName();
  int found = -1;
  for (int i = 0; i < devices.size(); ++i) {
    if (normalizeAudioDeviceName(devices[i].name) == autoName) {
      found = i;
      break;
    }
  }
  if (found < 0) {
    devices.prepend({autoName, QStringLiteral("Auto")});
  } else if (found > 0) {
    devices.move(found, 0);
  }
  return devices;
}

}  // namespace tramp
