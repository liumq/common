#ifndef CRC32_H_
#define CRC32_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

// this file copy from leveldb.

namespace crc32 {
// Return the crc32c of concat(A, data[0,n-1]) where init_crc is the
// crc32c of some string A.  Extend() is often used to maintain the
// crc32c of a stream of data.
uint32_t Extend(uint32_t init_crc, const char* data, size_t n);
inline uint32_t Extend(uint32_t init_crc, std::string& data) {
  return Extend(init_crc, data.c_str(), data.size());
}

// Return the crc32c of data[0,n-1]
inline uint32_t Value(const char* data, size_t n) {
  return Extend(0, data, n);
}
inline uint32_t Value(const std::string& data) {
  return Value(data.c_str(), data.size());
}

static const uint32_t kMaskDelta = 0xa282ead8ul;

// Return a masked representation of crc.
//
// Motivation: it is problematic to compute the CRC of a string that
// contains embedded CRCs.  Therefore we recommend that CRCs stored
// somewhere (e.g., in files) should be masked before being stored.
inline uint32_t Mask(uint32_t crc) {
  // Rotate right by 15 bits and add a constant.
  return ((crc >> 15) | (crc << 17)) + kMaskDelta;
}

// Return the crc whose masked representation is masked_crc.
inline uint32_t Unmask(uint32_t masked_crc) {
  uint32_t rot = masked_crc - kMaskDelta;
  return ((rot >> 17) | (rot << 15));
}

}
#endif /* CRC32_H_ */
