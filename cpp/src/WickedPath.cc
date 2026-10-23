#include "WickedPath.h"
#include "WickedUtil.hh"

extern "C" bool WickedPathValidate(const uint8_t* path_data, size_t path_size) {
  if (path_size < sizeof(WickedPathHeader_t)) return false;

  auto* const header = reinterpret_cast<const WickedPathHeader_t*>(path_data);
  auto* const segments = reinterpret_cast<const WickedPathSegmentHeader_t*>(path_data + sizeof(WickedPathHeader_t));

  if (path_size < sizeof(WickedPathHeader_t) + header->segment_size * sizeof(WickedPathSegmentHeader_t)) return false;

  for (uint8_t i = 0; i < header->segment_size; ++i) {
    const WickedPathSegmentHeader_t& segment = segments[i];
    if (path_size < segment.offset + segment.size) return false;
    const uint8_t* prog_data;
    uint16_t prog_size;
    uint16_t min_capacity;
    uint16_t final_size;
    if (!WickedPostfixRead(path_data + segment.offset, segment.size, &prog_data, &prog_size)) return false;
    if (WickedPostfixValidate(prog_data, prog_size, 1, &min_capacity, &final_size) != WickedEvalStatus_Ok || final_size != 1) return false;
  }
  return true;
}

bool WickedPathSegmentAt(const uint8_t* path_data, uint32_t t, WickedPathSegmentDescriptor_t* descriptor) {
  auto* const header = reinterpret_cast<const WickedPathHeader_t*>(path_data);
  auto* const segments = reinterpret_cast<const WickedPathSegmentHeader_t*>(path_data + sizeof(WickedPathHeader_t));

  uint16_t i = wicked_upper_bound(uint16_t(0), header->segment_size, [segments, t](uint16_t i) {
    return t - segments[0].start_time < segments[i].start_time - segments[0].start_time;
  });
  if (i == 0) return false;
  const WickedPathSegmentHeader_t& segment = segments[i - 1];

  if (descriptor) {
    descriptor->start_time = segment.start_time;
    descriptor->postfix_data = path_data + segment.offset;
    descriptor->postfix_size = segment.size;
  }
  return true;
}

extern "C" WickedEvalStatus_t WickedPathEvaluate(const uint8_t* path_data, uint32_t t, float* stack_data, uint16_t stack_capacity) {
  if (stack_capacity < 1) return WickedEvalStatus_StackOverflow;
  WickedPathSegmentDescriptor_t descriptor;
  if (!WickedPathSegmentAt(path_data, t, &descriptor)) return WickedEvalStatus_UndefinedOperation;
  const uint8_t* prog_data;
  uint16_t prog_size;
  if (!WickedPostfixRead(descriptor.postfix_data, descriptor.postfix_size, &prog_data, &prog_size)) return WickedEvalStatus_IllegalOperation;
  stack_data[0] = static_cast<float>(t - descriptor.start_time);
  WickedPostfixEvaluate(prog_data, prog_size, 1, stack_data);



  return WickedEvalStatus_Ok;
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
