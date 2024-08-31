#pragma once

#include "EvalStatus.h"
#include "Postfix.h"

#include <span>
#include <vector>

namespace wickedwinch::protocol {

struct PathHeader {
  uint16_t segment_size;
  uint8_t flags;
  uint8_t padding;

  static constexpr uint8_t Overflow = 1 << 0;
};

struct PathSegmentHeader {
  uint32_t start_time;
  uint16_t offset;
  uint16_t size;
};

struct PathSegmentReader {
  uint32_t start_time;
  PostfixReader expr;
};

struct PathSegmentWriter {
  uint32_t start_time;
  PostfixWriter expr;
};

class PathReader {
public:
  bool Read(std::span<const uint8_t> buffer) { return Read(buffer.data(), buffer.size()); }
  bool Read(const uint8_t* data, size_t size);

  static constexpr uint8_t kNoSegment = 255;
  uint8_t SegmentAt(uint32_t) const;
  EvalStatus Eval(uint32_t t, PostfixStack& stack) const;

  uint8_t flags() const { return header()->flags; }

  const PathSegmentHeader* segment_header_data() const {
    return reinterpret_cast<const PathSegmentHeader*>(buffer_ + segment_header_offset());
  }
  uint8_t segment_header_size() const { return header()->segment_size; }
  const PathSegmentHeader& segment_header(uint8_t i) const { return segment_header_data()[i]; };

  std::span<const uint8_t> segment_data(uint8_t i) const {
    if (i == kNoSegment) return {};
    const PathSegmentHeader& segment = segment_header(i);
    return {buffer_ + segment.offset, segment.size};
  }

private:
	const PathHeader* header() const { return reinterpret_cast<const PathHeader*>(buffer_); }

  constexpr size_t segment_header_offset() const { return sizeof(PathHeader); }

	const uint8_t* buffer_ = nullptr;
};

class PathWriter {
public:
  uint16_t data_size() const;
	bool Write(uint8_t* data, size_t size) const;
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

  uint16_t target() const { return target_; }
  void set_target(uint16_t target) { target_ = target; }

  PathSegmentWriter* add_segments() { return &segments_.emplace_back(); }
  std::span<PathSegmentWriter> segments() { return segments_; }
  std::span<const PathSegmentWriter> segments() const { return segments_; }

private:
  uint16_t target_;
  std::vector<PathSegmentWriter> segments_;
};

}