#include <CoreGraphics/CoreGraphics.h>

#include <cstdio>
#include <cstdlib>
#include <unordered_map>

// Test-only interposition: Qt 6.11.1 released the color space returned by
// qt_mac_cgImageFormatForImage before CGImageCreate used it (QTBUG-147602).
// CoreGraphics caching can keep the object alive, hiding the crash in a short
// smoke test. Check Qt's Create/Retain/Release balance at that boundary instead.
namespace {
thread_local bool checking = false;
thread_local int checkedImages = 0;
thread_local std::unordered_map<CFTypeRef, int> references;

CGColorSpaceRef createNamed(CFStringRef name) {
  const auto space = CGColorSpaceCreateWithName(name);
  if (checking && space) ++references[space];
  return space;
}

CGColorSpaceRef createIcc(CFDataRef data) {
  const auto space = CGColorSpaceCreateWithICCData(data);
  if (checking && space) ++references[space];
  return space;
}

CFTypeRef retain(CFTypeRef object) {
  if (checking) {
    const auto found = references.find(object);
    if (found != references.end()) ++found->second;
  }
  return CFRetain(object);
}

void release(CFTypeRef object) {
  if (checking) {
    const auto found = references.find(object);
    if (found != references.end()) --found->second;
  }
  CFRelease(object);
}

CGImageRef createImage(size_t width, size_t height, size_t bpc, size_t bpp,
                       size_t stride, CGColorSpaceRef space, CGBitmapInfo info,
                       CGDataProviderRef provider, const CGFloat* decode,
                       bool interpolate, CGColorRenderingIntent intent) {
  if (checking) {
    const auto found = references.find(space);
    if (found != references.end()) {
      ++checkedImages;
      if (found->second <= 0) {
        std::fputs("FAIL: CGImageCreate received a color space after its last "
                   "owned reference was released (QTBUG-147602)\n", stderr);
        // Do not run framework teardown from inside an intercepted call.
        std::fflush(stdout);
        std::_Exit(EXIT_FAILURE);
      }
      // The image now takes its own reference inside CoreGraphics. Stop
      // tracking this handoff; system-internal ownership is not our contract.
      references.erase(found);
    }
  }
  return CGImageCreate(width, height, bpc, bpp, stride, space, info, provider,
                       decode, interpolate, intent);
}

// Calls from this dylib to the original functions are not interposed by dyld.
#define INTERPOSE(replacement, original)                                      \
  __attribute__((used, section("__DATA,__interpose"))) static const struct {  \
    const void* replacement;                                                 \
    const void* original;                                                    \
  } pair_##original = {reinterpret_cast<const void*>(&replacement),           \
                       reinterpret_cast<const void*>(&original)}

INTERPOSE(createNamed, CGColorSpaceCreateWithName);
INTERPOSE(createIcc, CGColorSpaceCreateWithICCData);
INTERPOSE(retain, CFRetain);
INTERPOSE(release, CFRelease);
INTERPOSE(createImage, CGImageCreate);
}  // namespace

extern "C" void aoideBeginColorSpaceCheck() {
  references.clear();
  checkedImages = 0;
  checking = true;
}

extern "C" int aoideEndColorSpaceCheck() {
  checking = false;
  references.clear();
  return checkedImages;
}
