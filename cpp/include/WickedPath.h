#pragma once

#include "WickedEvalStatus.h"
#include "WickedPostfix.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WickedPathHeader {
  uint16_t segment_size;
  uint8_t flags;
  uint8_t padding;
} WickedPathHeader_t;

typedef struct WickedPathSegmentHeader {
  uint32_t start_time;
  uint16_t offset;
  uint16_t size;
} WickedPathSegmentHeader_t;

typedef struct WickedPathSegmentDescriptor {
  uint32_t start_time;
  const uint8_t* postfix_data;
  uint16_t postfix_size;
} WickedPathSegmentDescriptor_t;

bool WickedPathValidate(const uint8_t* path_data, size_t path_size);
bool WickedPathSegmentAt(const uint8_t* path_data, uint32_t t, WickedPathSegmentDescriptor_t* descriptor);
WickedEvalStatus_t WickedPathEvaluate(const uint8_t* path_data, uint32_t t, WickedPostfixEval_t* eval);

#ifdef __cplusplus
}

#include <span>
#include <vector>

struct WickedPathSegmentWriter {
  uint32_t start_time;
  WickedPostfixWriter expr;
};

class WickedPathWriter {
public:
  uint16_t data_size() const;
	bool Write(uint8_t* data, size_t size) const;
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

  WickedPathSegmentWriter* add_segments() { return &segments_.emplace_back(); }
  std::span<WickedPathSegmentWriter> segments() { return segments_; }
  std::span<const WickedPathSegmentWriter> segments() const { return segments_; }

private:
  std::vector<WickedPathSegmentWriter> segments_;
};

#endif
