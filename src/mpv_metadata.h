#pragma once

#include "track.h"

#include <mpv/client.h>

namespace aoide {

inline TrackMetadata metadataFromMpvNode(const mpv_node& node) {
  QMap<QString, QString> tags;
  if (node.format == MPV_FORMAT_NODE_MAP && node.u.list) {
    for (int i = 0; i < node.u.list->num; ++i) {
      const mpv_node& value = node.u.list->values[i];
      if (!node.u.list->keys[i] || value.format != MPV_FORMAT_STRING || !value.u.string) continue;
      tags.insert(QString::fromUtf8(node.u.list->keys[i]), QString::fromUtf8(value.u.string));
    }
  }
  return trackMetadataFromTags(tags);
}

}  // namespace aoide
