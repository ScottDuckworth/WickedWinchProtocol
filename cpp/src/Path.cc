#include <WickedWinchProtocol/Path.h>

#include <algorithm>
#include <cassert>

namespace wickedwinch::protocol {

uint8_t PathReader::SegmentAt(uint32_t t) const {
  if (buffer_ == nullptr) return kNoSegment;

  const PathSegmentHeader* begin = segment_header_data();
  const PathSegmentHeader* end = begin + segment_header_size();

  uint32_t begin_time = 0;
  if (header()->flags & PathHeader::Overflow) {
    begin_time = begin->start_time;
  }
  struct StartTimeLess {
    uint32_t begin_time;

    bool operator()(uint32_t t, const PathSegmentHeader& segment) const {
      return t - begin_time < segment.start_time - begin_time;
    }
  };
  const PathSegmentHeader* segment =
      std::upper_bound(begin, end, t, StartTimeLess{.begin_time = begin_time});
  if (segment == begin) return kNoSegment;
  --segment;
  return uint8_t(segment - begin);
}

WickedEvalStatus PathReader::Eval(uint32_t t, WickedPostfixEval& eval) const {
  uint8_t i = SegmentAt(t);
  if (i == kNoSegment) return WickedEvalStatus_UndefinedOperation;

  const PathSegmentHeader& segment = segment_header(i);
  if (!WickedPostfixRead(buffer_ + segment.offset, segment.size, &eval)) {
    return WickedEvalStatus_IllegalOperation;
  }

  float st = float(t - segment.start_time) * 1e-3f;
  eval.stack_size = 1;
  eval.stack_data[0] = st;
  return WickedPostfixEvaluate(&eval);
}

bool PathReader::Read(const uint8_t* data, size_t size) {
  buffer_ = data;
  if (size < sizeof(PathHeader)) return false;
  if (size < segment_header_offset() + segment_header_size() * sizeof(PathSegmentHeader)) {
    return false;
  }

  WickedPostfixEval eval;
  for (uint8_t i = 0; i < segment_header_size(); ++i) {
    const PathSegmentHeader& segment = segment_header(i);
    if (size < segment.offset + segment.size) return false;
    if (!WickedPostfixRead(buffer_ + segment.offset, segment.size, &eval)) return false;
  }
  return true;
}

uint16_t PathWriter::data_size() const {
  size_t size = sizeof(PathHeader) + segments_.size() * sizeof(PathSegmentHeader);
  for (const PathSegmentWriter& segment : segments_) {
    size += (segment.expr.data_size() + 3) & ~3;
  }
  return uint16_t(size);
}

bool PathWriter::Write(uint8_t* data, size_t size) const {
  const size_t headers_size = sizeof(PathHeader) + segments_.size() * sizeof(PathSegmentHeader);
  if (size < headers_size) return false;

  auto* header = reinterpret_cast<PathHeader*>(data);
  header->segment_size = uint8_t(segments_.size());
  header->flags = 0;

  auto* segment_header = reinterpret_cast<PathSegmentHeader*>(data + sizeof(PathHeader));
  uint16_t offset = uint16_t(headers_size);
  for (const PathSegmentWriter& segment : segments_) {
    if (offset > headers_size && segment.start_time < (segment_header-1)->start_time) {
      header->flags |= PathHeader::Overflow;
    }
    segment_header->start_time = segment.start_time;
    segment_header->offset = offset;
    segment_header->size = segment.expr.data_size();
    if (!segment.expr.Write(data + offset, size - offset)) return false;
    offset += uint16_t((segment_header->size + 3) & ~3);
    ++segment_header;
  }
  return true;
}

}