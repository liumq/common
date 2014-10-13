#ifndef REF_COUNTED_H_
#define REF_COUNTED_H_

#include "macro_def.h"
#include <assert.h>

class RefCounted {
 public:
  void Ref() {
    __sync_add_and_fetch(&count_, 1);
  }

  // return true iif ref_counted is 0.
  bool UnRef() {
    if (__sync_sub_and_fetch(&count_, 1) == 0) {
      delete this;
      return true;
    }
    return false;
  }

  int RefCount() {
    return __sync_fetch_and_add(&count_, 0);
  }

 protected:
  RefCounted()
      : count_(1) {
  }
  virtual ~RefCounted() {
    assert(RefCount() == 0);
  }

 private:
  int count_;

  DISALLOW_COPY_AND_ASSIGN(RefCounted);
};

#endif
