/**
 * @file TurboMidiArduino.hpp
 * @brief Arduino-specific implementation of TurboMIDI using hardware UART
 * @version 1.0
 * 
 * This file provides an Arduino-specific platform implementation for the
 * TurboMIDI library, using hardware UART for MIDI communication.
 */

#ifndef TURBOMIDI_ARDUINO_HPP
#define TURBOMIDI_ARDUINO_HPP

#include "TurboMidi.hpp"
#include <Arduino.h>

namespace TurboMIDI {

/**
 * Arduino platform implementation using hardware UART
 * 
 * This class implements the IPlatform interface for Arduino boards,
 * providing MIDI communication through hardware serial ports.
 */
class ArduinoPlatform : public IPlatform {
public:
    /**
     * Constructor
     * @param serial Reference to hardware serial port (Serial, Serial1, etc.)
     * @param rxPin Optional RX pin for software serial (0 = use hardware serial)
     * @param txPin Optional TX pin for software serial (0 = use hardware serial)
     */
    ArduinoPlatform(HardwareSerial& serial, uint8_t rxPin = 0, uint8_t txPin = 0) 
        : serial_(&serial), rxPin_(rxPin), txPin_(txPin), useSoftwareSerial_(false) {
        // Note: Software serial support could be added in future if needed
        // For now, we'll use hardware serial only for better performance
    }
    
    /**
     * Initialize the platform
     * Call this in Arduino setup() function
     */
    void begin() {
        // Start with standard MIDI baud rate
        serial_->begin(31250);
        
        // Set MIDI baud rate (31250 is not standard, but works on most Arduinos)
        // Some boards might need custom divisor settings
        setBaudRate(31250);
    }
    
    // IPlatform interface implementation
    void sendMidiData(const uint8_t* data, size_t length) override {
        for (size_t i = 0; i < length; ++i) {
            serial_->write(data[i]);
        }
    }
    
    size_t receiveMidiData(uint8_t* buffer, size_t maxLength) override {
        size_t bytesRead = 0;
        while (serial_->available() && bytesRead < maxLength) {
            buffer[bytesRead++] = serial_->read();
        }
        return bytesRead;
    }
    
    uint32_t getMillis() override {
        return millis();
    }
    
    void setBaudRate(uint32_t baudRate) override {
        // End current serial connection
        serial_->end();
        
        // Restart with new baud rate
        serial_->begin(baudRate);
        
        // Some Arduino boards need special handling for non-standard baud rates
        // This is especially true for rates above 115200
        
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
        // Leonardo, Micro, etc. - These can handle most rates directly
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
        // Uno, Nano, etc. - May need custom divisor for some rates
        // For now, we'll rely on the Serial library to handle it
#elif defined(ARDUINO_ARCH_SAMD)
        // SAMD boards (Zero, MKR series) - Generally handle high rates well
#elif defined(ESP32) || defined(ESP8266)
        // ESP boards - Can handle very high baud rates
#elif defined(ARDUINO_ARCH_RP2040)
        // Raspberry Pi Pico - Good high-speed UART support
#endif
        
        // Small delay to ensure UART is ready
        delay(10);
    }
    
    void delayMs(uint32_t ms) override {
        delay(ms);
    }
    
    /**
     * Check if data is available to read
     * @return Number of bytes available
     */
    int available() {
        return serial_->available();
    }
    
    /**
     * Flush the output buffer
     */
    void flush() {
        serial_->flush();
    }
    
private:
    HardwareSerial* serial_;
    uint8_t rxPin_;
    uint8_t txPin_;
    bool useSoftwareSerial_;
};

/**
 * Convenience class combining TurboMIDI with Arduino platform
 * 
 * This class provides a ready-to-use TurboMIDI implementation for Arduino,
 * handling all the platform-specific details internally.
 */
class TurboMIDIArduino {
public:
    /**
     * Constructor
     * @param serial Hardware serial port to use (Serial, Serial1, etc.)
     * @param role Device role (MASTER, SLAVE, or ANY)
     */
    TurboMIDIArduino(HardwareSerial& serial, DeviceRole role = DeviceRole::ANY)
        : platform_(serial), turboMidi_(&platform_, role) {
    }
    
    /**
     * Initialize the TurboMIDI system
     * Call this in Arduino setup() function
     */
    void begin() {
        platform_.begin();
    }
    
    /**
     * Configure supported speeds
     * @param speed Speed multiplier to support
     * @param certified Whether this speed has been certified (tested)
     */
    void setSupportedSpeed(SpeedMultiplier speed, bool certified = false) {
        turboMidi_.setSupportedSpeed(speed, certified);
    }
    
    /**
     * Process incoming MIDI data
     * Call this regularly in Arduino loop() function
     */
    void update() {
        turboMidi_.handleIncomingData();
        
        // Send active sensing if needed
        if (shouldSendActiveSense()) {
            turboMidi_.sendActiveSense();
            lastActiveSenseTime_ = millis();
        }
    }
    
    /**
     * Master: Negotiate speed with slave device
     * @param targetSpeed Desired speed multiplier
     * @param timeoutMs Timeout in milliseconds
     * @return true if negotiation successful
     */
    bool negotiateSpeed(SpeedMultiplier targetSpeed, uint32_t timeoutMs = 30) {
        return turboMidi_.negotiateSpeed(targetSpeed, timeoutMs);
    }
    
    /**
     * Master: Push speed change to slave
     * @param speed New speed multiplier
     */
    void pushSpeed(SpeedMultiplier speed) {
        turboMidi_.pushSpeed(speed);
    }
    
    /**
     * Get current speed multiplier
     * @return Current speed setting
     */
    SpeedMultiplier getCurrentSpeed() const {
        return turboMidi_.getCurrentSpeed();
    }
    
    /**
     * Get current baud rate
     * @return Current UART baud rate
     */
    uint32_t getCurrentBaudRate() const {
        return getBaudRateForSpeed(turboMidi_.getCurrentSpeed());
    }
    
    /**
     * Set callback for speed changes (useful in slave mode)
     * @param callback Function to call when speed changes
     */
    void onSpeedChanged(std::function<void(SpeedMultiplier)> callback) {
        turboMidi_.onSpeedChanged = callback;
    }
    
    /**
     * Set callback for speed requests (useful in slave mode)
     * @param callback Function to call when speed request received
     */
    void onSpeedRequest(std::function<void()> callback) {
        turboMidi_.onSpeedRequest = callback;
    }
    
    /**
     * Send raw MIDI data
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     */
    void sendMidiData(const uint8_t* data, size_t length) {
        platform_.sendMidiData(data, length);
    }
    
    /**
     * Check if MIDI data is available
     * @return Number of bytes available to read
     */
    int available() {
        return platform_.available();
    }
    
    /**
     * Flush output buffer
     */
    void flush() {
        platform_.flush();
    }
    
private:
    ArduinoPlatform platform_;
    TurboMIDI turboMidi_;
    uint32_t lastActiveSenseTime_ = 0;
    
    bool shouldSendActiveSense() {
        // Send active sensing every 250ms when at high speed
        return (turboMidi_.getCurrentSpeed() != SpeedMultiplier::SPEED_1X) &&
               (millis() - lastActiveSenseTime_ > 250);
    }
    
    static uint32_t getBaudRateForSpeed(SpeedMultiplier speed) {
        switch (speed) {
            case SpeedMultiplier::SPEED_1X:    return 31250;
            case SpeedMultiplier::SPEED_2X:    return 62500;
            case SpeedMultiplier::SPEED_3_3X:  return 103125;
            case SpeedMultiplier::SPEED_4X:    return 125000;
            case SpeedMultiplier::SPEED_5X:    return 156250;
            case SpeedMultiplier::SPEED_6_6X:  return 206250;
            case SpeedMultiplier::SPEED_8X:    return 250000;
            case SpeedMultiplier::SPEED_10X:   return 312500;
            case SpeedMultiplier::SPEED_13_3X: return 415625;
            case SpeedMultiplier::SPEED_16X:   return 500000;
            case SpeedMultiplier::SPEED_20X:   return 625000;
            default: return 31250;
        }
    }
};

} // namespace TurboMIDI

#endif // TURBOMIDI_ARDUINO_HPP