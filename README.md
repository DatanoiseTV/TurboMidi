# TurboMIDI

A header-only, platform-independent C++ library for implementing the Elektron TurboMIDI protocol. This library enables MIDI communication at speeds up to 20x the standard MIDI rate (31.25 kbit/s).

## Overview

TurboMIDI is a proprietary protocol developed by Elektron that allows compatible devices to negotiate and communicate at higher MIDI speeds. This library provides a complete implementation of the protocol specification, making it easy to add TurboMIDI support to your hardware or software projects.

## Features

- **High-Speed MIDI Communication**: Support for multipliers from 1x to 20x (31.25 kbit/s to 625 kbit/s)
- **Header-Only Library**: Single header file with no external dependencies
- **Platform Independent**: Abstract interface allows easy porting to any platform
- **Complete Protocol Implementation**: All 9 TurboMIDI commands fully implemented
- **Robust Communication**: Built-in timeout handling, active sensing, and error recovery
- **Master and Slave Modes**: Can function as either protocol master or slave device
- **Speed Certification**: Support for both certified and uncertified speed negotiations

## Supported Speeds

| Multiplier | Speed (kbit/s) | Multiplier | Speed (kbit/s) |
|------------|----------------|------------|----------------|
| 1x         | 31.25          | 8x         | 250            |
| 2x         | 62.5           | 10x        | 312.5          |
| 3.3x       | 103.125        | 13.3x      | 415.625        |
| 4x         | 125            | 16x        | 500            |
| 5x         | 156.25         | 20x        | 625            |
| 6.6x       | 206.25         |            |                |

## Requirements

- C++11 or later
- Platform-specific MIDI/Serial implementation

## Installation

Simply copy `TurboMidi.hpp` to your project and include it:

```cpp
#include "TurboMidi.hpp"
```

## Usage

### Basic Example

```cpp
#include "TurboMidi.hpp"

// 1. Implement the platform interface for your system
class MyPlatform : public TurboMIDI::IPlatform {
public:
    void sendMidiData(const uint8_t* data, size_t length) override {
        // Send data via your MIDI/serial interface
    }
    
    size_t receiveMidiData(uint8_t* buffer, size_t maxLength) override {
        // Receive available MIDI data (non-blocking)
        return bytesRead;
    }
    
    uint32_t getMillis() override {
        // Return milliseconds since start
        return millis();
    }
    
    void setBaudRate(uint32_t baudRate) override {
        // Set your UART/serial baud rate
    }
    
    void delayMs(uint32_t ms) override {
        // Platform-specific delay
    }
};

// 2. Create TurboMIDI instance
MyPlatform platform;
TurboMIDI::TurboMIDI turbo(&platform, TurboMIDI::DeviceRole::MASTER);

// 3. Configure supported speeds
turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X, true);   // certified
turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X, true);   // certified
turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_8X, false);  // uncertified

// 4. Negotiate speed with connected device
if (turbo.negotiateSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X)) {
    // Successfully negotiated 4x speed
}

// 5. Main loop
while (running) {
    turbo.handleIncomingData();
    turbo.sendActiveSense();  // Required for speeds > 1x
}
```

### Arduino Example

```cpp
#include "TurboMidi.hpp"

class ArduinoPlatform : public TurboMIDI::IPlatform {
private:
    HardwareSerial* serial;
    
public:
    ArduinoPlatform(HardwareSerial* ser) : serial(ser) {
        serial->begin(31250);  // Standard MIDI baud rate
    }
    
    void sendMidiData(const uint8_t* data, size_t length) override {
        serial->write(data, length);
    }
    
    size_t receiveMidiData(uint8_t* buffer, size_t maxLength) override {
        size_t bytesRead = 0;
        while (serial->available() && bytesRead < maxLength) {
            buffer[bytesRead++] = serial->read();
        }
        return bytesRead;
    }
    
    uint32_t getMillis() override {
        return millis();
    }
    
    void setBaudRate(uint32_t baudRate) override {
        serial->end();
        serial->begin(baudRate);
    }
    
    void delayMs(uint32_t ms) override {
        delay(ms);
    }
};

ArduinoPlatform platform(&Serial1);
TurboMIDI::TurboMIDI turbo(&platform, TurboMIDI::DeviceRole::SLAVE);

void setup() {
    // Configure as slave with 2x and 4x certified speeds
    turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X, true);
    turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X, true);
    
    // Set callbacks
    turbo.onSpeedChanged = [](TurboMIDI::SpeedMultiplier speed) {
        Serial.print("Speed changed to: ");
        Serial.print(static_cast<int>(speed));
        Serial.println("x");
    };
}

void loop() {
    turbo.handleIncomingData();
    turbo.sendActiveSense();
}
```

## Platform Implementation

To use TurboMIDI on your platform, implement the `IPlatform` interface:

| Method | Description |
|--------|-------------|
| `sendMidiData()` | Send raw MIDI bytes |
| `receiveMidiData()` | Non-blocking receive of available MIDI data |
| `getMillis()` | Return milliseconds elapsed (for timeouts) |
| `setBaudRate()` | Change UART/serial baud rate for speed changes |
| `delayMs()` | Platform-specific delay function |

See the examples directory for implementations for:
- Desktop (Windows/macOS/Linux) using RtMidi
- Arduino (AVR/ARM)
- STM32 using HAL
- Generic embedded template

## Protocol Details

### Commands

The library implements all TurboMIDI protocol commands:

| Command | ID | Direction | Description |
|---------|-----|-----------|-------------|
| SPEED_REQ | 0x10 | Any | Request speed capabilities |
| SPEED_ANSWER | 0x11 | Any | Report supported/certified speeds |
| SPEED_NEG | 0x12 | Master | Negotiate speed change |
| SPEED_ACK | 0x13 | Slave | Acknowledge negotiation |
| SPEED_TEST | 0x14 | Master | First test pattern |
| SPEED_RESULT | 0x15 | Slave | First test result |
| SPEED_TEST2 | 0x16 | Master | Second test pattern |
| SPEED_RESULT2 | 0x17 | Slave | Second test result |
| SPEED_PUSH | 0x20 | Master | Force speed change |

### Timeouts

- Master timeout: 30ms minimum
- Slave timeout: 15ms minimum, 25ms maximum
- Active sensing timeout: 300ms (reverts to 1x speed)

### Speed Negotiation Flow

1. Master sends SPEED_REQ
2. Slave responds with SPEED_ANSWER listing capabilities
3. Master sends SPEED_NEG with test and target speeds
4. Slave sends SPEED_ACK if acceptable
5. If uncertified speed: Master/Slave perform speed test
6. On success: Both switch to negotiated speed
7. Both devices send active sensing to maintain connection

## API Reference

### TurboMIDI Class

#### Constructor
```cpp
TurboMIDI(IPlatform* platform, DeviceRole role = DeviceRole::ANY)
```

#### Configuration Methods
```cpp
void setSupportedSpeed(SpeedMultiplier speed, bool certified = false)
```

#### Master Methods
```cpp
bool negotiateSpeed(SpeedMultiplier targetSpeed, uint32_t timeoutMs = 30)
void pushSpeed(SpeedMultiplier speed)
```

#### Common Methods
```cpp
void handleIncomingData()
void sendActiveSense()
SpeedMultiplier getCurrentSpeed() const
```

#### Callbacks
```cpp
std::function<void(SpeedMultiplier)> onSpeedChanged
std::function<void()> onSpeedRequest
```

## License

This library is provided as-is for use with Elektron devices and compatible hardware
and is provided under the MIT License.

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

## Acknowledgments

Protocol documentation provided by Elektron.
