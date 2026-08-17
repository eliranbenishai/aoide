#include "session_view.h"

namespace tramp {

SessionView SessionView::golden() {
  SessionView v;
  v.goldenDemo = true;
  return v;
}

}  // namespace tramp
