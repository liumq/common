#include "event_pipe.h"
#include "event_manager.h"

namespace {

bool createPipe(int* rfd, int* wfd) {
  int ret, fds[2];
#if __linux__
  ret = ::pipe2(fds, O_NONBLOCK | O_CLOEXEC);
#else
  ret = ::pipe(fds);
#endif
  if (ret == -1) {
    PLOG(WARNING)<< "pipe error";
    return false;
  }

#ifndef __linux__
  for (uint32 i = 0; i < 2; ++i) {
    setFdCloExec(fds[i]);
    setFdNonBlock(fds[i]);
  }
#endif

  *rfd = fds[0];
  *wfd = fds[1];
  return true;
}

void handlePipe(int fd, void* arg, uint8 revent, TimeStamp ts) {
  async::EventPipe* p = static_cast<async::EventPipe*>(arg);
  p->handleRead(ts);
}
}

namespace async {

bool EventPipe::init() {
  ev_mgr_->assertThreadSafe();
  if (event_ != nullptr) return false;
  if (!createPipe(&event_fd_[0], &event_fd_[1])) {
    return false;
  }

  event_.reset(new Event);
  event_->fd = event_fd_[0];
  event_->event = EV_READ;
  event_->arg = this;
  event_->cb = handlePipe;
  if (!ev_mgr_->add(event_.get())) {
    event_.reset();
    destory();
    return false;
  }

  return true;
}

void EventPipe::destory() {
  ev_mgr_->assertThreadSafe();
  if (event_ != nullptr) {
    ev_mgr_->del(*event_);
    event_.reset();
  }

  for (uint32 i = 0; i < 2; ++i) {
    closeWrapper(event_fd_[i]);
  }
}

void EventPipe::triggerPipe() {
  if (event_fd_[1] != INVALID_FD) {
    uint8 c;
    ::write(event_fd_[1], &c, sizeof(c));
  }
}

void EventPipe::clearDummyData() {

}

void EventPipe::handleRead(TimeStamp ts) {
  DCHECK_NE(INVALID_FD, event_fd_[0]);
  //todo: if error orrcured, exit...

  uint64 dummy;
  while (true) {  // read all of data.
    int ret = ::read(event_fd_[0], &dummy, sizeof(dummy));
    if (ret == 0) return;
    else if (ret == -1) {
      switch (errno) {
        case EINTR:
          continue;
        case EWOULDBLOCK:
          deletate_->handleEvent(ts);
          return;
      }
      PLOG(WARNING)<< "read pipe error" << event_fd_[0];
      return;
    }
  }
}
}
