/**
 * TurboMIDI Basic Example
 * 
 * This example demonstrates basic TurboMIDI functionality.
 * It creates a device that can operate as either master or slave,
 * and shows how to handle speed negotiation.
 * 
 * Hardware connections:
 * - MIDI IN: Connect to RX pin (pin 0 on most Arduinos)
 * - MIDI OUT: Connect to TX pin (pin 1 on most Arduinos)
 * - Remember to use proper MIDI circuitry (optocouplers, resistors, etc.)
 */

#include <TurboMidiArduino.hpp>

using namespace TurboMIDI;

// Create TurboMIDI instance using Serial port
// Change to Serial1, Serial2, etc. for boards with multiple UARTs
TurboMIDIArduino turboMidi(Serial, DeviceRole::ANY);

// LED pin for status indication
const int LED_PIN = 13;

void setup() {
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize TurboMIDI
  turboMidi.begin();
  
  // Configure supported speeds
  // Add all speeds your hardware can handle
  turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_2X, false);   // Not certified
  turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_4X, false);   // Not certified
  turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_8X, false);   // Not certified
  
  // Set up callbacks for slave mode
  turboMidi.onSpeedChanged([](SpeedMultiplier newSpeed) {
    // Flash LED when speed changes
    for (int i = 0; i < static_cast<int>(newSpeed); i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  });
  
  turboMidi.onSpeedRequest([]() {
    // Quick flash when speed request received
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  });
}

void loop() {
  // Process incoming MIDI data and handle TurboMIDI protocol
  turboMidi.update();
  
  // Example: Send a MIDI note every second
  static unsigned long lastNoteTime = 0;
  if (millis() - lastNoteTime > 1000) {
    lastNoteTime = millis();
    
    // Send MIDI Note On (middle C)
    uint8_t noteOn[] = {0x90, 60, 127};
    turboMidi.sendMidiData(noteOn, 3);
    
    delay(100);
    
    // Send MIDI Note Off
    uint8_t noteOff[] = {0x80, 60, 0};
    turboMidi.sendMidiData(noteOff, 3);
    
    // Show current speed on LED (short flashes)
    int flashes = static_cast<int>(turboMidi.getCurrentSpeed());
    for (int i = 0; i < flashes; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      delay(50);
    }
  }
}