#include "event_pooler.h"
#include "event/event_manager.h"

namespace async {

EventPooler::~EventPooler() {
//  ev_mgr_->assertThreadSafe();
  DCHECK(ev_vec_.empty());
}

bool EventPooler::Init() {
  ev_mgr_->assertThreadSafe();

  for (uint8 i = 0; i < worker_; ++i) {
    EventManager* ev_mgr = CreateEventManager();
    if (!ev_mgr->init()) {
      stop();
      delete ev_mgr;
      return false;
    }

    ev_mgr->loopInAnotherThread();
    ev_vec_.push_back(ev_mgr);
  }

  return true;
}

void EventPooler::stop() {
  for (auto ev_mgr : ev_vec_) {
    SyncEvent stop_ev;
    ev_mgr->stop(&stop_ev);
    stop_ev.Wait();
    delete ev_mgr;
  }

  ev_vec_.clear();
}

EventManager* EventPooler::getPoller() {
  ev_mgr_->assertThreadSafe();
  if (ev_vec_.empty()) return ev_mgr_;

  if (index_ == ev_vec_.size()) {
    index_ = 0;
    return ev_mgr_;
  }

  DCHECK_LT(index_, ev_vec_.size());
  return ev_vec_[index_++];
}

}
