/**
 * @file turbomidi.hpp
 * @brief Header-only, platform-independent Elektron TurboMIDI library
 * @version 1.0
 * 
 * This library provides a friendly API for implementing the Elektron TurboMIDI
 * protocol on various platforms (desktop, Arduino, embedded systems, etc.)
 */

#ifndef TURBOMIDI_HPP
#define TURBOMIDI_HPP

#include <cstdint>
#include <vector>
#include <functional>
#include <chrono>
#include <algorithm>
#include <array>

namespace TurboMIDI {

// Constants
constexpr uint8_t SYSEX_START = 0xF0;
constexpr uint8_t SYSEX_END = 0xF7;
constexpr uint8_t ACTIVE_SENSING = 0xFE;

// Elektron manufacturer ID
constexpr std::array<uint8_t, 5> ELEKTRON_ID = {0x00, 0x20, 0x3C, 0x00, 0x00};

// Command IDs
enum class CommandID : uint8_t {
    SPEED_REQ     = 0x10,
    SPEED_ANSWER  = 0x11,
    SPEED_NEG     = 0x12,
    SPEED_ACK     = 0x13,
    SPEED_TEST    = 0x14,
    SPEED_RESULT  = 0x15,
    SPEED_TEST2   = 0x16,
    SPEED_RESULT2 = 0x17,
    SPEED_PUSH    = 0x20
};

// Speed multipliers
enum class SpeedMultiplier : uint8_t {
    SPEED_1X    = 1,
    SPEED_2X    = 2,
    SPEED_3_3X  = 3,
    SPEED_4X    = 4,
    SPEED_5X    = 5,
    SPEED_6_6X  = 6,
    SPEED_8X    = 7,
    SPEED_10X   = 8,
    SPEED_13_3X = 9,
    SPEED_16X   = 10,
    SPEED_20X   = 11
};

// Device role
enum class DeviceRole {
    MASTER,
    SLAVE,
    ANY
};

// Platform abstraction layer
class IPlatform {
public:
    virtual ~IPlatform() = default;
    
    // Send raw MIDI data
    virtual void sendMidiData(const uint8_t* data, size_t length) = 0;
    
    // Receive MIDI data (non-blocking, returns number of bytes read)
    virtual size_t receiveMidiData(uint8_t* buffer, size_t maxLength) = 0;
    
    // Get current time in milliseconds
    virtual uint32_t getMillis() = 0;
    
    // Set UART/MIDI baud rate
    virtual void setBaudRate(uint32_t baudRate) = 0;
    
    // Platform-specific delay
    virtual void delayMs(uint32_t ms) = 0;
};

// Speed configuration
struct SpeedConfig {
    uint8_t mask1 = 0;
    uint8_t mask2 = 0;
    uint8_t cert1 = 0;
    uint8_t cert2 = 0;
    
    void addSpeed(SpeedMultiplier speed, bool certified = false) {
        switch (speed) {
            case SpeedMultiplier::SPEED_2X:    mask1 |= (1 << 0); if (certified) cert1 |= (1 << 0); break;
            case SpeedMultiplier::SPEED_3_3X:  mask1 |= (1 << 1); if (certified) cert1 |= (1 << 1); break;
            case SpeedMultiplier::SPEED_4X:    mask1 |= (1 << 2); if (certified) cert1 |= (1 << 2); break;
            case SpeedMultiplier::SPEED_5X:    mask1 |= (1 << 3); if (certified) cert1 |= (1 << 3); break;
            case SpeedMultiplier::SPEED_6_6X:  mask1 |= (1 << 4); if (certified) cert1 |= (1 << 4); break;
            case SpeedMultiplier::SPEED_8X:    mask1 |= (1 << 5); if (certified) cert1 |= (1 << 5); break;
            case SpeedMultiplier::SPEED_10X:   mask1 |= (1 << 6); if (certified) cert1 |= (1 << 6); break;
            case SpeedMultiplier::SPEED_13_3X: mask2 |= (1 << 0); if (certified) cert2 |= (1 << 0); break;
            case SpeedMultiplier::SPEED_16X:   mask2 |= (1 << 1); if (certified) cert2 |= (1 << 1); break;
            case SpeedMultiplier::SPEED_20X:   mask2 |= (1 << 2); if (certified) cert2 |= (1 << 2); break;
            default: break;
        }
    }
    
    bool hasSpeed(SpeedMultiplier speed) const {
        switch (speed) {
            case SpeedMultiplier::SPEED_2X:    return mask1 & (1 << 0);
            case SpeedMultiplier::SPEED_3_3X:  return mask1 & (1 << 1);
            case SpeedMultiplier::SPEED_4X:    return mask1 & (1 << 2);
            case SpeedMultiplier::SPEED_5X:    return mask1 & (1 << 3);
            case SpeedMultiplier::SPEED_6_6X:  return mask1 & (1 << 4);
            case SpeedMultiplier::SPEED_8X:    return mask1 & (1 << 5);
            case SpeedMultiplier::SPEED_10X:   return mask1 & (1 << 6);
            case SpeedMultiplier::SPEED_13_3X: return mask2 & (1 << 0);
            case SpeedMultiplier::SPEED_16X:   return mask2 & (1 << 1);
            case SpeedMultiplier::SPEED_20X:   return mask2 & (1 << 2);
            default: return false;
        }
    }
    
    bool isCertified(SpeedMultiplier speed) const {
        switch (speed) {
            case SpeedMultiplier::SPEED_2X:    return cert1 & (1 << 0);
            case SpeedMultiplier::SPEED_3_3X:  return cert1 & (1 << 1);
            case SpeedMultiplier::SPEED_4X:    return cert1 & (1 << 2);
            case SpeedMultiplier::SPEED_5X:    return cert1 & (1 << 3);
            case SpeedMultiplier::SPEED_6_6X:  return cert1 & (1 << 4);
            case SpeedMultiplier::SPEED_8X:    return cert1 & (1 << 5);
            case SpeedMultiplier::SPEED_10X:   return cert1 & (1 << 6);
            case SpeedMultiplier::SPEED_13_3X: return cert2 & (1 << 0);
            case SpeedMultiplier::SPEED_16X:   return cert2 & (1 << 1);
            case SpeedMultiplier::SPEED_20X:   return cert2 & (1 << 2);
            default: return false;
        }
    }
};

// Command builders
class CommandBuilder {
public:
    static std::vector<uint8_t> buildSpeedReq() {
        return buildCommand(CommandID::SPEED_REQ);
    }
    
    static std::vector<uint8_t> buildSpeedAnswer(const SpeedConfig& config) {
        return buildCommand(CommandID::SPEED_ANSWER, 
            {config.mask1, config.mask2, config.cert1, config.cert2});
    }
    
    static std::vector<uint8_t> buildSpeedNeg(SpeedMultiplier testSpeed, SpeedMultiplier targetSpeed) {
        return buildCommand(CommandID::SPEED_NEG, 
            {static_cast<uint8_t>(testSpeed), static_cast<uint8_t>(targetSpeed)});
    }
    
    static std::vector<uint8_t> buildSpeedAck() {
        return buildCommand(CommandID::SPEED_ACK);
    }
    
    static std::vector<uint8_t> buildSpeedTest() {
        return buildCommand(CommandID::SPEED_TEST, 
            {0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00});
    }
    
    static std::vector<uint8_t> buildSpeedResult() {
        return buildCommand(CommandID::SPEED_RESULT, 
            {0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00});
    }
    
    static std::vector<uint8_t> buildSpeedTest2() {
        return buildCommand(CommandID::SPEED_TEST2);
    }
    
    static std::vector<uint8_t> buildSpeedResult2() {
        return buildCommand(CommandID::SPEED_RESULT2);
    }
    
    static std::vector<uint8_t> buildSpeedPush(SpeedMultiplier speed) {
        return buildCommand(CommandID::SPEED_PUSH, {static_cast<uint8_t>(speed)});
    }
    
private:
    static std::vector<uint8_t> buildCommand(CommandID cmd, const std::vector<uint8_t>& payload = {}) {
        std::vector<uint8_t> message;
        message.push_back(SYSEX_START);
        message.insert(message.end(), ELEKTRON_ID.begin(), ELEKTRON_ID.end());
        message.push_back(static_cast<uint8_t>(cmd));
        message.insert(message.end(), payload.begin(), payload.end());
        message.push_back(SYSEX_END);
        return message;
    }
};

// Main TurboMIDI class
class TurboMIDI {
public:
    TurboMIDI(IPlatform* platform, DeviceRole role = DeviceRole::ANY) 
        : platform_(platform), role_(role), currentSpeed_(SpeedMultiplier::SPEED_1X),
          lastActiveSenseTime_(0), lastMessageTime_(0), testState_(TestState::IDLE),
          pendingTestSpeed_(SpeedMultiplier::SPEED_1X), 
          pendingTargetSpeed_(SpeedMultiplier::SPEED_1X) {
        // Initialize with only 1x speed supported by default
        localConfig_.addSpeed(SpeedMultiplier::SPEED_1X, true);
    }
    
    // Configure supported speeds
    void setSupportedSpeed(SpeedMultiplier speed, bool certified = false) {
        localConfig_.addSpeed(speed, certified);
    }
    
    // Master functions
    bool negotiateSpeed(SpeedMultiplier targetSpeed, uint32_t timeoutMs = 30) {
        if (role_ == DeviceRole::SLAVE) return false;
        
        // Send speed request
        sendCommand(CommandBuilder::buildSpeedReq());
        
        // Wait for answer
        SpeedConfig remoteConfig;
        if (!waitForSpeedAnswer(remoteConfig, timeoutMs)) {
            return false;
        }
        
        // Check if target speed is supported
        if (!remoteConfig.hasSpeed(targetSpeed)) {
            return false;
        }
        
        // Determine test speed
        SpeedMultiplier testSpeed = targetSpeed;
        if (!remoteConfig.isCertified(targetSpeed) && targetSpeed != SpeedMultiplier::SPEED_1X) {
            // Need to test with higher speed first
            testSpeed = getNextHigherSpeed(targetSpeed);
            if (testSpeed == targetSpeed) return false; // No higher speed available
        }
        
        // Send negotiation
        sendCommand(CommandBuilder::buildSpeedNeg(testSpeed, targetSpeed));
        
        // Wait for ACK
        if (!waitForAck(timeoutMs)) {
            return false;
        }
        
        // Perform speed test if needed
        if (targetSpeed != SpeedMultiplier::SPEED_1X && testSpeed != targetSpeed) {
            if (!performSpeedTest(testSpeed, targetSpeed)) {
                return false;
            }
        }
        
        // Set new speed
        setSpeed(targetSpeed);
        return true;
    }
    
    void pushSpeed(SpeedMultiplier speed) {
        if (role_ == DeviceRole::SLAVE) return;
        sendCommand(CommandBuilder::buildSpeedPush(speed));
        setSpeed(speed);
    }
    
    // Slave functions
    void handleIncomingData() {
        uint8_t buffer[256];
        size_t bytesRead = platform_->receiveMidiData(buffer, sizeof(buffer));
        
        for (size_t i = 0; i < bytesRead; ++i) {
            processIncomingByte(buffer[i]);
        }
        
        // Check for timeouts
        checkTimeouts();
    }
    
    // Common functions
    void sendActiveSense() {
        if (currentSpeed_ != SpeedMultiplier::SPEED_1X) {
            uint8_t activeSense = ACTIVE_SENSING;
            platform_->sendMidiData(&activeSense, 1);
            lastActiveSenseTime_ = platform_->getMillis();
        }
    }
    
    SpeedMultiplier getCurrentSpeed() const { return currentSpeed_; }
    
    // Callbacks for slave mode
    std::function<void(SpeedMultiplier)> onSpeedChanged;
    std::function<void()> onSpeedRequest;
    
private:
    enum class TestState {
        IDLE,
        WAITING_FOR_TEST,
        WAITING_FOR_TEST2
    };
    
    IPlatform* platform_;
    DeviceRole role_;
    SpeedConfig localConfig_;
    SpeedMultiplier currentSpeed_;
    uint32_t lastActiveSenseTime_;
    uint32_t lastMessageTime_;
    std::vector<uint8_t> incomingBuffer_;
    TestState testState_;
    SpeedMultiplier pendingTestSpeed_;
    SpeedMultiplier pendingTargetSpeed_;
    
    void sendCommand(const std::vector<uint8_t>& cmd) {
        platform_->sendMidiData(cmd.data(), cmd.size());
    }
    
    void setSpeed(SpeedMultiplier speed) {
        currentSpeed_ = speed;
        uint32_t baudRate = getBaudRate(speed);
        platform_->setBaudRate(baudRate);
        
        if (onSpeedChanged) {
            onSpeedChanged(speed);
        }
    }
    
    uint32_t getActualMultiplier(SpeedMultiplier speed) {
        switch (speed) {
            case SpeedMultiplier::SPEED_1X:    return 1;
            case SpeedMultiplier::SPEED_2X:    return 2;
            case SpeedMultiplier::SPEED_3_3X:  return 3;  // Actually 3.33, but using 3 for integer math
            case SpeedMultiplier::SPEED_4X:    return 4;
            case SpeedMultiplier::SPEED_5X:    return 5;
            case SpeedMultiplier::SPEED_6_6X:  return 7;  // Actually 6.66, rounding to 7
            case SpeedMultiplier::SPEED_8X:    return 8;
            case SpeedMultiplier::SPEED_10X:   return 10;
            case SpeedMultiplier::SPEED_13_3X: return 13; // Actually 13.33, using 13
            case SpeedMultiplier::SPEED_16X:   return 16;
            case SpeedMultiplier::SPEED_20X:   return 20;
            default: return 1;
        }
    }
    
    uint32_t getBaudRate(SpeedMultiplier speed) {
        // Calculate exact baud rates for each speed
        switch (speed) {
            case SpeedMultiplier::SPEED_1X:    return 31250;
            case SpeedMultiplier::SPEED_2X:    return 62500;
            case SpeedMultiplier::SPEED_3_3X:  return 103125;   // 31250 * 3.3
            case SpeedMultiplier::SPEED_4X:    return 125000;
            case SpeedMultiplier::SPEED_5X:    return 156250;
            case SpeedMultiplier::SPEED_6_6X:  return 206250;   // 31250 * 6.6
            case SpeedMultiplier::SPEED_8X:    return 250000;
            case SpeedMultiplier::SPEED_10X:   return 312500;
            case SpeedMultiplier::SPEED_13_3X: return 415625;   // 31250 * 13.3
            case SpeedMultiplier::SPEED_16X:   return 500000;
            case SpeedMultiplier::SPEED_20X:   return 625000;
            default: return 31250;
        }
    }
    
    bool waitForSpeedAnswer(SpeedConfig& config, uint32_t timeoutMs) {
        uint32_t startTime = platform_->getMillis();
        
        while (platform_->getMillis() - startTime < timeoutMs) {
            handleIncomingData();
            
            // Check if we received a complete SPEED_ANSWER message
            if (parseSpeedAnswer(config)) {
                return true;
            }
            
            platform_->delayMs(1);
        }
        
        return false;
    }
    
    bool waitForAck(uint32_t timeoutMs) {
        uint32_t startTime = platform_->getMillis();
        
        while (platform_->getMillis() - startTime < timeoutMs) {
            handleIncomingData();
            
            // Check if we received ACK
            if (parseAck()) {
                return true;
            }
            
            platform_->delayMs(1);
        }
        
        return false;
    }
    
    bool performSpeedTest(SpeedMultiplier testSpeed, SpeedMultiplier targetSpeed) {
        // Send breathing time (16 null bytes)
        uint8_t nullBytes[16] = {0};
        platform_->sendMidiData(nullBytes, 16);
        platform_->delayMs(10);
        
        // Switch to test speed
        setSpeed(testSpeed);
        
        // Send first test pattern
        sendCommand(CommandBuilder::buildSpeedTest());
        
        // Wait for first result
        if (!waitForSpeedResult(30)) {
            setSpeed(SpeedMultiplier::SPEED_1X);
            return false;
        }
        
        // Send second test pattern
        sendCommand(CommandBuilder::buildSpeedTest2());
        
        // Wait for second result
        if (!waitForSpeedResult2(30)) {
            setSpeed(SpeedMultiplier::SPEED_1X);
            return false;
        }
        
        // Tests passed, switch to target speed
        setSpeed(targetSpeed);
        return true;
    }
    
    bool waitForSpeedResult(uint32_t timeoutMs) {
        uint32_t startTime = platform_->getMillis();
        
        while (platform_->getMillis() - startTime < timeoutMs) {
            handleIncomingData();
            
            // Check if we received SPEED_RESULT
            if (parseSpeedResult()) {
                return true;
            }
            
            platform_->delayMs(1);
        }
        
        return false;
    }
    
    bool waitForSpeedResult2(uint32_t timeoutMs) {
        uint32_t startTime = platform_->getMillis();
        
        while (platform_->getMillis() - startTime < timeoutMs) {
            handleIncomingData();
            
            // Check if we received SPEED_RESULT2
            if (parseSpeedResult2()) {
                return true;
            }
            
            platform_->delayMs(1);
        }
        
        return false;
    }
    
    void processIncomingByte(uint8_t byte) {
        lastMessageTime_ = platform_->getMillis();
        
        if (byte == SYSEX_START) {
            incomingBuffer_.clear();
        }
        
        incomingBuffer_.push_back(byte);
        
        if (byte == SYSEX_END) {
            processCompleteMessage();
        }
        
        if (byte == ACTIVE_SENSING) {
            // Reset timeout
            lastMessageTime_ = platform_->getMillis();
        }
    }
    
    void processCompleteMessage() {
        if (incomingBuffer_.size() < 8) return;
        
        // Verify Elektron header
        if (incomingBuffer_[0] != SYSEX_START ||
            incomingBuffer_[incomingBuffer_.size() - 1] != SYSEX_END) {
            return;
        }
        
        // Check manufacturer ID
        bool validManufacturer = true;
        for (size_t i = 0; i < ELEKTRON_ID.size(); ++i) {
            if (incomingBuffer_[i + 1] != ELEKTRON_ID[i]) {
                validManufacturer = false;
                break;
            }
        }
        if (!validManufacturer) return;
        
        CommandID cmd = static_cast<CommandID>(incomingBuffer_[6]);
        
        // Handle commands based on role
        switch (cmd) {
            case CommandID::SPEED_REQ:
                if (role_ != DeviceRole::MASTER) {
                    sendCommand(CommandBuilder::buildSpeedAnswer(localConfig_));
                    if (onSpeedRequest) onSpeedRequest();
                }
                break;
                
            case CommandID::SPEED_NEG:
                if (role_ != DeviceRole::MASTER && incomingBuffer_.size() >= 10) {
                    SpeedMultiplier testSpeed = static_cast<SpeedMultiplier>(incomingBuffer_[7]);
                    SpeedMultiplier targetSpeed = static_cast<SpeedMultiplier>(incomingBuffer_[8]);
                    
                    if (localConfig_.hasSpeed(targetSpeed)) {
                        sendCommand(CommandBuilder::buildSpeedAck());
                        
                        // Wait for test or immediate speed change
                        if (targetSpeed == SpeedMultiplier::SPEED_1X || 
                            (localConfig_.isCertified(targetSpeed) && testSpeed == targetSpeed)) {
                            // No test needed, change speed immediately
                            setSpeed(targetSpeed);
                        } else {
                            // Prepare for speed test
                            pendingTestSpeed_ = testSpeed;
                            pendingTargetSpeed_ = targetSpeed;
                            testState_ = TestState::WAITING_FOR_TEST;
                        }
                    }
                }
                break;
                
            case CommandID::SPEED_TEST:
                if (role_ != DeviceRole::MASTER && testState_ == TestState::WAITING_FOR_TEST &&
                    incomingBuffer_.size() >= 16) {
                    // Verify test pattern
                    bool testValid = true;
                    if (incomingBuffer_[7] != 0x55 || incomingBuffer_[8] != 0x55 ||
                        incomingBuffer_[9] != 0x55 || incomingBuffer_[10] != 0x55 ||
                        incomingBuffer_[11] != 0x00 || incomingBuffer_[12] != 0x00 ||
                        incomingBuffer_[13] != 0x00 || incomingBuffer_[14] != 0x00) {
                        testValid = false;
                    }
                    
                    if (testValid) {
                        // Switch to test speed and send result
                        setSpeed(pendingTestSpeed_);
                        sendCommand(CommandBuilder::buildSpeedResult());
                        testState_ = TestState::WAITING_FOR_TEST2;
                    } else {
                        // Test failed, revert to 1x
                        setSpeed(SpeedMultiplier::SPEED_1X);
                        testState_ = TestState::IDLE;
                    }
                }
                break;
                
            case CommandID::SPEED_TEST2:
                if (role_ != DeviceRole::MASTER && testState_ == TestState::WAITING_FOR_TEST2) {
                    sendCommand(CommandBuilder::buildSpeedResult2());
                    // Test complete, switch to target speed
                    setSpeed(pendingTargetSpeed_);
                    testState_ = TestState::IDLE;
                }
                break;
                
            case CommandID::SPEED_PUSH:
                if (incomingBuffer_.size() >= 9) {
                    SpeedMultiplier speed = static_cast<SpeedMultiplier>(incomingBuffer_[7]);
                    if (localConfig_.hasSpeed(speed)) {
                        setSpeed(speed);
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    bool parseSpeedAnswer(SpeedConfig& config) {
        // Look for SPEED_ANSWER in buffer
        for (const auto& msg : getParsedMessages()) {
            if (msg.size() >= 12 && msg[6] == static_cast<uint8_t>(CommandID::SPEED_ANSWER)) {
                config.mask1 = msg[7];
                config.mask2 = msg[8];
                config.cert1 = msg[9];
                config.cert2 = msg[10];
                return true;
            }
        }
        return false;
    }
    
    bool parseAck() {
        for (const auto& msg : getParsedMessages()) {
            if (msg.size() >= 8 && msg[6] == static_cast<uint8_t>(CommandID::SPEED_ACK)) {
                return true;
            }
        }
        return false;
    }
    
    bool parseSpeedResult() {
        for (const auto& msg : getParsedMessages()) {
            if (msg.size() >= 16 && msg[6] == static_cast<uint8_t>(CommandID::SPEED_RESULT)) {
                // Verify the result pattern matches what we sent
                if (msg[7] == 0x55 && msg[8] == 0x55 && msg[9] == 0x55 && msg[10] == 0x55 &&
                    msg[11] == 0x00 && msg[12] == 0x00 && msg[13] == 0x00 && msg[14] == 0x00) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool parseSpeedResult2() {
        for (const auto& msg : getParsedMessages()) {
            if (msg.size() >= 8 && msg[6] == static_cast<uint8_t>(CommandID::SPEED_RESULT2)) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<std::vector<uint8_t>> getParsedMessages() {
        std::vector<std::vector<uint8_t>> messages;
        std::vector<uint8_t> currentMsg;
        
        for (uint8_t byte : incomingBuffer_) {
            if (byte == SYSEX_START) {
                currentMsg.clear();
            }
            currentMsg.push_back(byte);
            if (byte == SYSEX_END && !currentMsg.empty()) {
                messages.push_back(currentMsg);
            }
        }
        
        return messages;
    }
    
    void checkTimeouts() {
        uint32_t now = platform_->getMillis();
        
        // Check active sensing timeout (300ms)
        if (currentSpeed_ != SpeedMultiplier::SPEED_1X && 
            now - lastMessageTime_ > 300) {
            setSpeed(SpeedMultiplier::SPEED_1X);
        }
        
        // Send active sensing if needed
        if (currentSpeed_ != SpeedMultiplier::SPEED_1X && 
            now - lastActiveSenseTime_ > 250) {
            sendActiveSense();
        }
    }
    
    SpeedMultiplier getNextHigherSpeed(SpeedMultiplier speed) {
        int current = static_cast<int>(speed);
        if (current < 11) {
            return static_cast<SpeedMultiplier>(current + 1);
        }
        return speed;
    }
};

} // namespace TurboMIDI

#endif // TURBOMIDI_HPP