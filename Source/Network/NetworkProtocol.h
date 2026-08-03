#pragma once
#include <juce_core/juce_core.h>

namespace NetworkProtocol {

static constexpr int DEFAULT_PORT = 7777;

enum class PacketType : uint8_t {
  TelemetryUpdate = 0x01,
  ControlCommand = 0x02,
  SampleInspector = 0x03,
  LoadContainer = 0x04,
  MidiForward = 0x05,
  SetLatency = 0x06
};

#pragma pack(push, 1)

struct SetLatencyPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'};
  uint8_t packetType{(uint8_t)PacketType::SetLatency};
  uint32_t bufferSize{128}; // 32, 64, 128, 256
};

struct MidiForwardPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'};
  uint8_t packetType{(uint8_t)PacketType::MidiForward};
  uint8_t channel{1};
  uint8_t statusByte{0}; // e.g. 0x90 (NoteOn), 0x80 (NoteOff), 0xB0 (CC)
  uint8_t data1{0};      // Note Number or CC Controller Number
  uint8_t data2{0};      // Velocity or CC Value
};

struct TelemetryPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'}; // "TKBP"
  uint8_t packetType{(uint8_t)PacketType::TelemetryUpdate};
  float cpuUsage{0.0f};
  uint32_t activeVoices{0};
  float layerGain[4]{1.0f, 1.0f, 1.0f, 1.0f};
  uint8_t layerMuted[4]{0, 0, 0, 0};
};

struct ControlPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'};
  uint8_t packetType{(uint8_t)PacketType::ControlCommand};
  uint8_t layerIndex{0};
  float gain{1.0f};
  uint8_t isMuted{0};
};

struct SampleInspectorPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'};
  uint8_t packetType{(uint8_t)PacketType::SampleInspector};
  char sampleName[64]{0};
  uint8_t rootNote{0};
  uint8_t keyLow{0};
  uint8_t keyHigh{0};
  uint8_t velLow{0};
  uint8_t velHigh{0};
};
#pragma pack(pop)

} // namespace NetworkProtocol
