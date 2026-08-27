#include "chrome_anim.h"

#include <algorithm>

namespace tramp {

bool takesPointerFeedback(ChromeHit::Kind kind) {
  // Every control is listed and there is no default, so adding a hit kind is a
  // -Wswitch error until someone decides. Answering yes for a control no painter
  // draws feedback for is not free: it rebuilds that panel's whole chassis on
  // every mouse move and changes not one pixel.
  switch (kind) {
    // Sliders and list rows are dragged or scrolled, not pressed, and a track
    // list under the pointer would re-rasterise the playlist on every move.
    case ChromeHit::Kind::none:
    case ChromeHit::Kind::volume:
    case ChromeHit::Kind::seek:
    case ChromeHit::Kind::eqPreamp:
    case ChromeHit::Kind::eqBand:
    case ChromeHit::Kind::plDivider:
    case ChromeHit::Kind::plResize:
    case ChromeHit::Kind::plTrackRow:
    case ChromeHit::Kind::plCollectionRow:
    case ChromeHit::Kind::settingsSkinScroll:
    // Bare text and readouts: no face to light.
    case ChromeHit::Kind::timeToggle:
    case ChromeHit::Kind::aboutWeb:
      return false;

    case ChromeHit::Kind::options:
    case ChromeHit::Kind::skins:
    case ChromeHit::Kind::trackInfo:
    case ChromeHit::Kind::mute:
    case ChromeHit::Kind::mono:
    case ChromeHit::Kind::eqToggle:
    case ChromeHit::Kind::plToggle:
    case ChromeHit::Kind::prev:
    case ChromeHit::Kind::play:
    case ChromeHit::Kind::pause:
    case ChromeHit::Kind::stop:
    case ChromeHit::Kind::next:
    case ChromeHit::Kind::eject:
    case ChromeHit::Kind::shuffle:
    case ChromeHit::Kind::repeat:
    case ChromeHit::Kind::eqOn:
    case ChromeHit::Kind::eqAuto:
    case ChromeHit::Kind::eqPresets:
    case ChromeHit::Kind::plCollapse:
    case ChromeHit::Kind::plAddCollection:
    case ChromeHit::Kind::plCreate:
    case ChromeHit::Kind::plRename:
    case ChromeHit::Kind::plRemoveCollection:
    case ChromeHit::Kind::plAdd:
    case ChromeHit::Kind::plRemove:
    case ChromeHit::Kind::plSave:
    case ChromeHit::Kind::plSort:
    case ChromeHit::Kind::plOptions:
    case ChromeHit::Kind::plPrev:
    case ChromeHit::Kind::plPlay:
    case ChromeHit::Kind::plNext:
    case ChromeHit::Kind::plRefresh:
    case ChromeHit::Kind::settingsGeneral:
    case ChromeHit::Kind::settingsAudio:
    case ChromeHit::Kind::settingsResume:
    case ChromeHit::Kind::settingsConfirm:
    case ChromeHit::Kind::settingsScroll:
    case ChromeHit::Kind::settingsMinimize:
    case ChromeHit::Kind::settingsSnapOff:
    case ChromeHit::Kind::settingsSnapNormal:
    case ChromeHit::Kind::settingsSnapStrong:
    case ChromeHit::Kind::settingsReset:
    case ChromeHit::Kind::settingsAudioDevice:
    case ChromeHit::Kind::settingsExclusive:
    case ChromeHit::Kind::settingsSkinRow:
    case ChromeHit::Kind::settingsSkinRemove:
    case ChromeHit::Kind::settingsSkinAdd:
    case ChromeHit::Kind::settingsSkinsFolder:
    case ChromeHit::Kind::settingsSkinsRefresh:
      return true;
  }
  return false;
}

const ChromePhases::Entry* ChromePhases::find(Group group, int key, int index,
                                              BtnChannel channel) const {
  for (const Entry& e : entries_) {
    if (e.group == group && e.key == key && e.index == index && e.channel == channel) return &e;
  }
  return nullptr;
}

void ChromePhases::write(Group group, int key, int index, BtnChannel channel, qreal target,
                         bool snap) {
  target = std::clamp(target, qreal(0), qreal(1));
  for (Entry& e : entries_) {
    if (e.group == group && e.key == key && e.index == index && e.channel == channel) {
      e.target = target;
      if (snap) e.value = target;
      return;
    }
  }
  // Resting at zero and asked for zero is the common case: every unlit button,
  // on every view the session publishes. Storing those would grow the list
  // without ever animating anything.
  if (target <= 0) return;
  entries_.push_back({group, key, index, channel, snap ? target : qreal(0), target});
}

qreal ChromePhases::read(Group group, int key, int index, BtnChannel channel) const {
  const Entry* e = find(group, key, index, channel);
  return e ? easeBtnPhase(e->value) : 0;
}

void ChromePhases::setTarget(ChromeHit::Kind kind, int index, BtnChannel channel, qreal target) {
  write(Group::body, int(kind), index, channel, target, false);
}

void ChromePhases::snapTo(ChromeHit::Kind kind, int index, BtnChannel channel, qreal target) {
  write(Group::body, int(kind), index, channel, target, true);
}

qreal ChromePhases::value(ChromeHit::Kind kind, int index, BtnChannel channel) const {
  return read(Group::body, int(kind), index, channel);
}

BtnFace ChromePhases::face(ChromeHit::Kind kind, int index) const {
  return BtnFace(value(kind, index, BtnChannel::on), value(kind, index, BtnChannel::hover),
                 value(kind, index, BtnChannel::press));
}

void ChromePhases::setTitleTarget(TitleChromeLayout::Hit hit, BtnChannel channel, qreal target) {
  write(Group::title, int(hit), -1, channel, target, false);
}

BtnFace ChromePhases::titleFace(TitleChromeLayout::Hit hit) const {
  return BtnFace(read(Group::title, int(hit), -1, BtnChannel::on),
                 read(Group::title, int(hit), -1, BtnChannel::hover),
                 read(Group::title, int(hit), -1, BtnChannel::press));
}

void ChromePhases::releaseChannel(BtnChannel channel) {
  for (Entry& e : entries_) {
    if (e.channel == channel) e.target = 0;
  }
}

bool ChromePhases::moving() const {
  for (const Entry& e : entries_) {
    if (e.value != e.target) return true;
  }
  return false;
}

bool ChromePhases::advance(qreal dtMs) {
  const qreal step = dtMs <= 0 ? 1 : std::min(qreal(1), dtMs / kBtnTransitionMs);
  bool busy = false;
  for (Entry& e : entries_) {
    if (e.value == e.target) continue;
    const qreal delta = e.target - e.value;
    e.value = std::abs(delta) <= step ? e.target : e.value + (delta > 0 ? step : -step);
    if (e.value != e.target) busy = true;
  }
  // An entry settled at zero is indistinguishable from an absent one, and hover
  // creates one for every control the pointer crosses.
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                [](const Entry& e) { return e.value <= 0 && e.target <= 0; }),
                 entries_.end());
  return busy;
}

}  // namespace tramp
