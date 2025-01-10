#include <WickedWinchProtocol/Path.h>

#include <algorithm>
#include <cassert>

extern "C" bool WickedPathValidate(const uint8_t* path_data, size_t path_size) {
  if (path_size < sizeof(WickedPathHeader_t)) return false;

  auto* const header = reinterpret_cast<const WickedPathHeader_t*>(path_data);
  auto* const segments = reinterpret_cast<const WickedPathSegmentHeader_t*>(path_data + sizeof(WickedPathHeader_t));

  if (path_size < sizeof(WickedPathHeader_t) + header->segment_size * sizeof(WickedPathSegmentHeader_t)) return false;

  WickedPostfixEval postfix_eval;
  for (uint8_t i = 0; i < header->segment_size; ++i) {
    const WickedPathSegmentHeader_t& segment = segments[i];
    if (path_size < segment.offset + segment.size) return false;
    if (!WickedPostfixRead(path_data + segment.offset, segment.size, &postfix_eval)) return false;
  }
  return true;
}

bool WickedPathSegmentAt(const uint8_t* path_data, uint32_t t, WickedPathSegmentDescriptor_t* descriptor) {
  auto* const header = reinterpret_cast<const WickedPathHeader_t*>(path_data);
  auto* const segments = reinterpret_cast<const WickedPathSegmentHeader_t*>(path_data + sizeof(WickedPathHeader_t));

  const WickedPathSegmentHeader_t* begin = segments;
  const WickedPathSegmentHeader_t* end = begin + header->segment_size;

  struct StartTimeLess {
    uint32_t begin_time;

    bool operator()(uint32_t t, const WickedPathSegmentHeader_t& segment) const {
      return t - begin_time < segment.start_time - begin_time;
    }
  };
  const WickedPathSegmentHeader_t* segment =
      std::upper_bound(begin, end, t, StartTimeLess{.begin_time = begin->start_time});
  if (segment == begin) return false;
  --segment;

  descriptor->start_time = segment->start_time;
  descriptor->postfix_data = path_data + segment->offset;
  descriptor->postfix_size = segment->size;
  return true;
}

extern "C" WickedEvalStatus WickedPathEvaluate(const uint8_t* path_data, uint32_t t, WickedPostfixEval_t* eval) {
  WickedPathSegmentDescriptor_t descriptor;
  if (!WickedPathSegmentAt(path_data, t, &descriptor)) return WickedEvalStatus_UndefinedOperation;
  if (!WickedPostfixRead(descriptor.postfix_data, descriptor.postfix_size, eval)) return WickedEvalStatus_IllegalOperation;
  WickedPostfixEval_reset(eval);
  WickedEvalStatus status = WickedPostfixEval_push(eval, t - descriptor.start_time);
  if (status != WickedEvalStatus_Ok) return status;
  return WickedPostfixEvaluate(eval);
}

uint16_t WickedPathWriter::data_size() const {
  size_t size = sizeof(WickedPathHeader_t) + segments_.size() * sizeof(WickedPathSegmentHeader_t);
  for (const WickedPathSegmentWriter& segment : segments_) {
    size += (segment.expr.data_size() + 3) & ~3;
  }
  return uint16_t(size);
}

bool WickedPathWriter::Write(uint8_t* data, size_t size) const {
  const size_t headers_size = sizeof(WickedPathHeader_t) + segments_.size() * sizeof(WickedPathSegmentHeader_t);
  if (size < headers_size) return false;

  auto* header = reinterpret_cast<WickedPathHeader_t*>(data);
  header->segment_size = uint8_t(segments_.size());
  header->flags = 0;

  auto* segment_header = reinterpret_cast<WickedPathSegmentHeader_t*>(data + sizeof(WickedPathHeader_t));
  uint16_t offset = uint16_t(headers_size);
  for (const WickedPathSegmentWriter& segment : segments_) {
    segment_header->start_time = segment.start_time;
    segment_header->offset = offset;
    segment_header->size = segment.expr.data_size();
    if (!segment.expr.Write(data + offset, size - offset)) return false;
    offset += uint16_t((segment_header->size + 3) & ~3);
    ++segment_header;
  }
  return true;
}
