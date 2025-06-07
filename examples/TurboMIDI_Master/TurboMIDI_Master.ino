/**
 * TurboMIDI Master Example
 * 
 * This example shows how to use TurboMIDI as a master device
 * that initiates speed negotiation with a slave device.
 * 
 * It attempts to negotiate progressively higher speeds with
 * the connected device and displays the results.
 * 
 * Hardware connections:
 * - MIDI IN: Connect to RX pin (pin 0 on most Arduinos)
 * - MIDI OUT: Connect to TX pin (pin 1 on most Arduinos)
 * - Button: Connect to pin 2 (with pull-up)
 * - LED: Built-in LED on pin 13
 */

#include <TurboMidiArduino.hpp>

using namespace TurboMIDI;

// Create TurboMIDI instance as MASTER
TurboMIDIArduino turboMidi(Serial, DeviceRole::MASTER);

// Pin definitions
const int BUTTON_PIN = 2;
const int LED_PIN = 13;

// Speed levels to try
const SpeedMultiplier speeds[] = {
  SpeedMultiplier::SPEED_2X,
  SpeedMultiplier::SPEED_4X,
  SpeedMultiplier::SPEED_8X,
  SpeedMultiplier::SPEED_10X,
  SpeedMultiplier::SPEED_16X
};
const int numSpeeds = sizeof(speeds) / sizeof(speeds[0]);
int currentSpeedIndex = -1; // -1 means 1x

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 500;

void setup() {
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize serial for debugging (if using different port for MIDI)
  // Serial.begin(115200);
  // Serial.println("TurboMIDI Master Example");
  
  // Initialize TurboMIDI
  turboMidi.begin();
  
  // Configure our supported speeds (as master, we should support all)
  for (int i = 0; i < numSpeeds; i++) {
    turboMidi.setSupportedSpeed(speeds[i], true); // Certified
  }
  
  // Initial LED indication (1 flash = 1x speed)
  flashLED(1);
}

void loop() {
  // Process TurboMIDI protocol
  turboMidi.update();
  
  // Check button press
  if (digitalRead(BUTTON_PIN) == LOW && 
      millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    // Try next speed
    attemptNextSpeed();
  }
  
  // Send test MIDI data periodically
  static unsigned long lastMidiTime = 0;
  if (millis() - lastMidiTime > 500) {
    lastMidiTime = millis();
    
    // Send a control change message
    uint8_t cc[] = {0xB0, 7, random(128)}; // Volume CC
    turboMidi.sendMidiData(cc, 3);
  }
}

void attemptNextSpeed() {
  // Quick flash to indicate button press
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(200);
  
  // Increment speed index
  currentSpeedIndex++;
  if (currentSpeedIndex >= numSpeeds) {
    // Wrap back to 1x
    currentSpeedIndex = -1;
    
    // Push 1x speed (no negotiation needed)
    turboMidi.pushSpeed(SpeedMultiplier::SPEED_1X);
    flashLED(1);
    return;
  }
  
  // Attempt to negotiate new speed
  SpeedMultiplier targetSpeed = speeds[currentSpeedIndex];
  
  // Flash rapidly during negotiation
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }
  
  bool success = turboMidi.negotiateSpeed(targetSpeed, 1000); // 1 second timeout
  
  if (success) {
    // Success! Flash LED to show new speed
    flashLED(static_cast<int>(targetSpeed));
  } else {
    // Failed, revert to previous speed
    currentSpeedIndex--;
    
    // Long flash to indicate failure
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
  }
}

void flashLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}