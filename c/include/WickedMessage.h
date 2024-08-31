#pragma once

#include <stdint.h>

enum WickedMessageType {
  WickedMessageType_None                     = 0,
  WickedMessageType_PingRequest              = 1,  // Payload: WickedPingRequest
  WickedMessageType_PingResponse             = 2,  // Payload: WickedPingResponse
  WickedMessageType_NotifyBmpStatus          = 3,  // Payload: WickedBmpStatus
  WickedMessageType_NotifyWinchStatus        = 4,  // Payload: WickedWinchStatus
  WickedMessageType_GetWinchTargetsRequest   = 5,  // Payload: none
  WickedMessageType_GetWinchTargetsResponse  = 6,  // Payload: WickedTargetList
  WickedMessageType_GetWinchConfigRequest    = 7,  // Payload: none
  WickedMessageType_GetWinchConfigResponse   = 8,  // Payload: WickedWinchConfig
  WickedMessageType_SetWinchConfig           = 9,  // Payload: WickedWinchConfig
  WickedMessageType_SetWinchPath             = 10, // Payload: WickedWinchPath
  WickedMessageType_GetDmxTargetsRequest     = 11, // Payload: none
  WickedMessageType_GetDmxTargetsResponse    = 12, // Payload: WickedTargetList
  WickedMessageType_GetDmxConfigRequest      = 13, // Payload: none
  WickedMessageType_GetDmxConfigResponse     = 14, // Payload: WickedDmxConfig
  WickedMessageType_SetDmxConfig             = 15, // Payload: WickedDmxConfig
  WickedMessageType_SetDmxPath               = 16, // Payload: WickedDmxPath
};

struct WickedMessageHeader {
  uint8_t target_id;
  uint8_t payload_type;
  uint16_t payload_size;
};

struct WickedPingRequest {
  uint32_t ping_id;
};

struct WickedPingResponse {
  uint32_t ping_id;
  uint32_t device_time;
};

struct WickedTargetList {
  uint8_t targets_size;
  uint8_t targets[0];
};

struct WickedBmpStatus {
  uint32_t device_time;
  float celsius;
  float pascals;
};

enum WickedWinchStatusFlag {
  WickedWinchStatusFlag_PositionKnown = 1 << 0,
  WickedWinchStatusFlag_Limit1        = 1 << 1,
  WickedWinchStatusFlag_Limit2        = 1 << 2,
};

enum WickedWinchMode {
  // Winch is disengaged, free spooling.
  WickedWinchMode_Disengage = 0,
  // Path returns extension velocity of the winch. Positive extends.
  WickedWinchMode_LinearVelocity = 1,
  // Path returns extension position. Positive direction extends.
  WickedWinchMode_LinearPosition = 2,
};

struct WickedWinchStatus {
  uint32_t device_time;
  // The linear position of the winch, if known.
  uint32_t position;
  // Bitwise-or of WinchStatusFlag.
  uint8_t flags;
};

struct WickedWinchConfig {
  // Number of stepper motor steps per revolution.
  int16_t steps_per_rev;
  // Number of encoder ticks per revolution.
  int16_t ticks_per_rev;
  // Linear distance of the winch per revolution (circumference).
  float distance_per_rev;
};

struct WickedWinchPath {
  // The WinchMode of this winch.
  uint8_t mode;
  // Padding for alignment.
  uint8_t padding;
  // The size of path_data.
  uint16_t path_size;
  // The serialized path.
  uint8_t path_data[0];
};

struct WickedDmxConfig {
  // Added to every value in channel_map to get the mapped DMX channel.
  uint8_t channel_offset;
  // The number of DMX channels in channel_map.
  uint8_t channel_size;
  // The DMX channel number at index i is (channel_offset + channel_map[i]).
  uint8_t channel_map[0];
};

struct WickedDmxPath {
  // Padding for alignment.
  uint16_t padding;
  // The size of path_data.
  uint16_t path_size;
  // The serialized path.
  uint8_t path_data[0];
};
