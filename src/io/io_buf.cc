#include "io_buf.h"

namespace io {

void OutVectorObject::BuildData(std::vector<iovec>* io_vec) const {
  io_vec->clear();

  if (left_ != 0) {
    uint32 skip_size = offset_;
    const std::vector<iovec>& data = obj_->IoVec();
    for (uint32 i = 0; i < data.size(); ++i) {
      iovec io;
      if (skip_size != 0) {
        if (data[i].iov_len < skip_size) {
          skip_size -= data[i].iov_len;
          continue;
        } else if (data[i].iov_len == skip_size) {
          skip_size = 0;
          continue;
        }

        io.iov_base = (char*) data[i].iov_base + skip_size;
        io.iov_len = data[i].iov_len - skip_size;
        io_vec->push_back(io);
        skip_size = 0;
        continue;
      }

      io_vec->push_back(data[i]);
    }
  }
}

bool OutVectorObject::Send(int fd, int32* err_no) {
  if (left_ == 0) return true;
  std::vector<iovec> io_vec;

  while (left_ != 0) {
    BuildData(&io_vec);
    CHECK(!io_vec.empty());

    int32 writen = ::writev(fd, io_vec.data(), io_vec.size());
    if (writen == -1) {
      if (errno == EINTR) continue;
      if (err_no != NULL) *err_no = errno;
      return false;
    }

    left_ -= writen;
    offset_ += writen;
  }

  DCHECK_EQ(left_, 0);
  return true;
}

}
