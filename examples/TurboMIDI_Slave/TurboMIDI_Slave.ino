/**
 * TurboMIDI Slave Example
 * 
 * This example shows how to use TurboMIDI as a slave device
 * that responds to speed negotiation requests from a master.
 * 
 * The slave will accept speed changes based on DIP switch
 * settings that determine which speeds are supported.
 * 
 * Hardware connections:
 * - MIDI IN: Connect to RX pin (pin 0 on most Arduinos)
 * - MIDI OUT: Connect to TX pin (pin 1 on most Arduinos)
 * - DIP switches: Pins 4-7 (optional, for speed configuration)
 * - Status LEDs: Pins 8-10 (optional)
 */

#include <TurboMidiArduino.hpp>

using namespace TurboMIDI;

// Create TurboMIDI instance as SLAVE
TurboMIDIArduino turboMidi(Serial, DeviceRole::SLAVE);

// Pin definitions
const int DIP_PINS[] = {4, 5, 6, 7};  // DIP switches for speed config
const int LED_GREEN = 8;   // Connected/OK
const int LED_YELLOW = 9;  // Activity
const int LED_RED = 10;    // High speed active

// MIDI processing
unsigned long lastMidiActivity = 0;

void setup() {
  // Initialize pins
  for (int i = 0; i < 4; i++) {
    pinMode(DIP_PINS[i], INPUT_PULLUP);
  }
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // Show startup sequence
  startupSequence();
  
  // Initialize TurboMIDI
  turboMidi.begin();
  
  // Configure supported speeds based on DIP switches
  configureSupportedSpeeds();
  
  // Set up callbacks
  turboMidi.onSpeedChanged([](SpeedMultiplier newSpeed) {
    // Update LEDs based on speed
    if (newSpeed == SpeedMultiplier::SPEED_1X) {
      digitalWrite(LED_RED, LOW);
    } else {
      digitalWrite(LED_RED, HIGH);
    }
    
    // Flash yellow LED to indicate speed change
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_YELLOW, HIGH);
      delay(100);
      digitalWrite(LED_YELLOW, LOW);
      delay(100);
    }
  });
  
  turboMidi.onSpeedRequest([]() {
    // Flash yellow LED on speed request
    digitalWrite(LED_YELLOW, HIGH);
    delay(50);
    digitalWrite(LED_YELLOW, LOW);
  });
  
  // Green LED on to show ready
  digitalWrite(LED_GREEN, HIGH);
}

void loop() {
  // Process TurboMIDI protocol
  turboMidi.update();
  
  // Process any incoming MIDI data
  if (turboMidi.available() > 0) {
    lastMidiActivity = millis();
    digitalWrite(LED_YELLOW, HIGH);
  }
  
  // Turn off activity LED after a short time
  if (millis() - lastMidiActivity > 50) {
    digitalWrite(LED_YELLOW, LOW);
  }
  
  // Example: Echo received MIDI data with modification
  static uint8_t midiBuffer[3];
  static uint8_t bufferIndex = 0;
  
  while (turboMidi.available() > 0) {
    // This is a simplified MIDI echo - real implementation
    // would need proper MIDI parsing
    uint8_t byte = Serial.read();
    
    // Simple MIDI message detection (very basic)
    if (byte & 0x80) {
      // Status byte
      bufferIndex = 0;
      midiBuffer[bufferIndex++] = byte;
    } else if (bufferIndex > 0 && bufferIndex < 3) {
      // Data byte
      midiBuffer[bufferIndex++] = byte;
      
      // Check if we have a complete 3-byte message
      if (bufferIndex == 3 && (midiBuffer[0] & 0xF0) == 0x90) {
        // Note on - echo with velocity reduced by half
        midiBuffer[2] = midiBuffer[2] / 2;
        turboMidi.sendMidiData(midiBuffer, 3);
        bufferIndex = 0;
      }
    }
  }
}

void configureSupportedSpeeds() {
  // Read DIP switches to determine supported speeds
  // DIP 1: Enable 2x and 4x
  // DIP 2: Enable 8x and 10x  
  // DIP 3: Enable 16x
  // DIP 4: All speeds certified (tested)
  
  bool dip1 = digitalRead(DIP_PINS[0]) == LOW;
  bool dip2 = digitalRead(DIP_PINS[1]) == LOW;
  bool dip3 = digitalRead(DIP_PINS[2]) == LOW;
  bool certified = digitalRead(DIP_PINS[3]) == LOW;
  
  // Always support 1x (standard MIDI)
  turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_1X, true);
  
  if (dip1) {
    turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_2X, certified);
    turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_4X, certified);
  }
  
  if (dip2) {
    turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_8X, certified);
    turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_10X, certified);
  }
  
  if (dip3) {
    turboMidi.setSupportedSpeed(SpeedMultiplier::SPEED_16X, certified);
  }
  
  // Flash LEDs to show configuration
  digitalWrite(LED_GREEN, dip1);
  digitalWrite(LED_YELLOW, dip2);
  digitalWrite(LED_RED, dip3);
  delay(1000);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
}

void startupSequence() {
  // LED test sequence
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  delay(200);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);
  delay(200);
  digitalWrite(LED_RED, LOW);
}