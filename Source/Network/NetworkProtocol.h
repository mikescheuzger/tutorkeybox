#pragma once
#include <juce_core/juce_core.h>

namespace NetworkProtocol {

static constexpr int DEFAULT_PORT = 7777;
static constexpr int MIDI_PORT = 7778; // UDP port for real-time network MIDI forwarding

enum class PacketType : uint8_t {
  TelemetryUpdate = 0x01,
  ControlCommand  = 0x02,
  SampleInspector = 0x03,
  LoadContainer   = 0x04,
  MidiForward     = 0x05
};

#pragma pack(push, 1)
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

// CORE CONCEPT: Network packet for forwarding live MIDI events from Raspberry Pi to Mac GUI.
struct MidiForwardPacket {
  uint8_t magic[4]{'T', 'K', 'B', 'P'};
  uint8_t packetType{(uint8_t)PacketType::MidiForward};
  uint8_t status{0}; // MIDI status byte (0x90 = NoteOn, 0x80 = NoteOff, 0xB0 = CC)
  uint8_t data1{0};  // Note or CC number
  uint8_t data2{0};  // Velocity or CC value
};
#pragma pack(pop)

} // namespace NetworkProtocol
