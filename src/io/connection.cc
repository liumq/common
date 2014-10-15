#include "io_buf.h"
#include "input_buf.h"
#include "protocol.h"
#include "connection.h"
#include "event_manager.h"

DEFINE_int32(input_buf_len, 128, "the size of inputbuf");

namespace {

void HandleEvent(int fd, void* arg, uint8 revent, const TimeStamp& time_stamp) {
  io::Connection* conn = static_cast<io::Connection*>(arg);

  if (revent & EV_WRITE) {
    conn->handleWrite(time_stamp);
  }
  if (revent & EV_READ) {
    conn->handleRead(time_stamp);
  }
}

class io::Connection::OutQueue : public io::OutputObject {
 public:
  OutQueue() {
  }
  virtual ~OutQueue() {
    STLClear(&out_queue_);
  }

  void Push(io::OutputObject* obj) {
    out_queue_.push_back(obj);
  }
  bool empty() const {
    return out_queue_.empty();
  }

  virtual bool Send(int fd, int32* err_no);

 private:
  std::deque<io::OutputObject*> out_queue_;

  DISALLOW_COPY_AND_ASSIGN(OutQueue);
};

bool io::Connection::OutQueue::Send(int fd, int32* err_no) {
  while (!out_queue_.empty()) {
    io::OutputObject* obj = out_queue_.front();
    if (!obj->Send(fd, err_no)) {
      return false;
    }

    delete obj;
    out_queue_.pop_front();
  }

  return true;
}

}

namespace io {

Connection::Connection(int fd, EventManager* ev_mgr)
    : fd_(fd),
      closed_(false),
      ev_mgr_(ev_mgr),
      protocol_(NULL) {
  DCHECK_NE(fd, INVALID_FD);
  DCHECK_NOTNULL(ev_mgr);
}

Connection::~Connection() {
  if (fd_ != INVALID_FD) {
    if (event_.get() != NULL) {
      ev_mgr_->Del(*event_);
    }
    ::close(fd_);
  }
}

bool Connection::Init() {
  if (event_.get() != NULL) return true;

  event_.reset(new Event);
  event_->fd = fd_;
  event_->event = EV_READ;
  event_->arg = this;
  event_->cb = HandleEvent;

  if (!ev_mgr_->Add(event_.get())) {
    event_.reset();
    return false;
  }

  out_queue_.reset(new OutQueue);
  input_buf_.reset(new InputBuf(FLAGS_input_buf_len));

  return true;
}

void Connection::Send(OutputObject* out_obj) {
  if (fd_ == INVALID_FD || closed_) {
    delete out_obj;
    return;
  }

  int32 err_no;
  if (out_queue_->empty()) {
    if (out_obj->Send(fd_, &err_no)) {
      delete out_obj;
      return;
    } else if (err_no != EWOULDBLOCK) {
      ShutDown();
      return;
    }
  }

  out_queue_->Push(out_obj);
  if (!(event_->event & EV_WRITE)) {
    event_->event |= EV_WRITE;
    ev_mgr_->Mod(event_.get());
  }
}

int32 Connection::Recv(uint32 len) {
  if (fd_ == INVALID_FD || closed_) return 0;

  int32 err_no, ret, left = len;
  while (left != 0) {
    ret = input_buf_->ReadFd(fd_, left, &err_no);
    if (ret == 0) {
      ShutDown();
      return 0;
    } else if (ret == -1) {
      if (err_no == EWOULDBLOCK) {
        continue;
      }
      ShutDown();
    }

    DCHECK_GT(ret, 0);
    left -= ret;
  }

  return len - left;
}

void Connection::handleRead(const TimeStamp& time_stamp) {
  if (fd_ != INVALID_FD && !closed_) {
    protocol_->handleRead(this, input_buf_.get(), time_stamp);
  }
}

void Connection::handleWrite(const TimeStamp& time_stamp) {
  if (fd_ == INVALID_FD || closed_) return;

  int32 err_no;
  if (!out_queue_->empty()) {
    if (out_queue_->Send(fd_, &err_no)) {
      event_->event &= ~EV_WRITE;
      ev_mgr_->Mod(event_.get());
      return;
    }

    if (err_no != EWOULDBLOCK) {
      // remove writeable event.
      event_->event &= ~EV_WRITE;
      ev_mgr_->Mod(event_.get());
      ShutDown();
    }
  }
}

void Connection::ShutDown() {
  if (closed_) return;
  ::shutdown(fd_, SHUT_WR);

  if (close_closure_.get() != NULL) {
    close_closure_->Run();
  }
  closed_ = true;
}

}
