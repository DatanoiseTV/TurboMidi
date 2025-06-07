/**
 * @file tests.cpp
 * @brief Unit tests for TurboMIDI library
 * 
 * Compile with: g++ -std=c++11 tests.cpp -o tests
 * Run: ./tests
 */

#include <iostream>
#include <cstring>
#include <queue>
#include <algorithm>
#include "TurboMidi.hpp"

// Test framework
class TestFramework {
private:
    int totalTests = 0;
    int passedTests = 0;
    std::string currentTest;
    
public:
    void startTest(const std::string& name) {
        currentTest = name;
        std::cout << "Running: " << name << "... ";
        totalTests++;
    }
    
    void endTest() {
        passedTests++;
        std::cout << "PASSED" << std::endl;
    }
    
    void verify(bool condition, const std::string& message) {
        if (!condition) {
            std::cout << "FAILED" << std::endl;
            std::cout << "  Assertion failed: " << message << std::endl;
            exit(1);
        }
    }
    
    void printSummary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Summary: " << passedTests << "/" << totalTests << " passed" << std::endl;
        if (passedTests == totalTests) {
            std::cout << "All tests passed!" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
};

// Mock platform for testing
class MockPlatform : public TurboMIDI::IPlatform {
public:
    std::queue<uint8_t> rxBuffer;
    std::vector<uint8_t> txBuffer;
    uint32_t currentTime = 0;
    uint32_t currentBaudRate = 31250;
    
    void sendMidiData(const uint8_t* data, size_t length) override {
        txBuffer.insert(txBuffer.end(), data, data + length);
    }
    
    size_t receiveMidiData(uint8_t* buffer, size_t maxLength) override {
        size_t bytesRead = 0;
        while (!rxBuffer.empty() && bytesRead < maxLength) {
            buffer[bytesRead++] = rxBuffer.front();
            rxBuffer.pop();
        }
        return bytesRead;
    }
    
    uint32_t getMillis() override {
        return currentTime;
    }
    
    void setBaudRate(uint32_t baudRate) override {
        currentBaudRate = baudRate;
    }
    
    void delayMs(uint32_t ms) override {
        currentTime += ms;
    }
    
    // Helper methods for testing
    void clearBuffers() {
        txBuffer.clear();
        while (!rxBuffer.empty()) rxBuffer.pop();
    }
    
    void injectMessage(const std::vector<uint8_t>& message) {
        for (uint8_t byte : message) {
            rxBuffer.push(byte);
        }
    }
    
    bool findMessage(const std::vector<uint8_t>& expected) {
        return std::search(txBuffer.begin(), txBuffer.end(), 
                          expected.begin(), expected.end()) != txBuffer.end();
    }
    
    std::vector<uint8_t> getLastMessage() {
        std::vector<uint8_t> message;
        for (size_t i = 0; i < txBuffer.size(); ++i) {
            if (txBuffer[i] == 0xF0) {  // SysEx start
                message.clear();
            }
            message.push_back(txBuffer[i]);
            if (txBuffer[i] == 0xF7) {  // SysEx end
                return message;
            }
        }
        return message;
    }
};

// Test functions
void testCommandBuilders(TestFramework& test) {
    test.startTest("Command Builders - SPEED_REQ");
    auto speedReq = TurboMIDI::CommandBuilder::buildSpeedReq();
    std::vector<uint8_t> expected = {0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x10, 0xF7};
    test.verify(speedReq == expected, "SPEED_REQ message incorrect");
    test.endTest();
    
    test.startTest("Command Builders - SPEED_ANSWER");
    TurboMIDI::SpeedConfig config;
    config.mask1 = 0x55;
    config.mask2 = 0x07;
    config.cert1 = 0x15;
    config.cert2 = 0x02;
    auto speedAnswer = TurboMIDI::CommandBuilder::buildSpeedAnswer(config);
    expected = {0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x11, 0x55, 0x07, 0x15, 0x02, 0xF7};
    test.verify(speedAnswer == expected, "SPEED_ANSWER message incorrect");
    test.endTest();
    
    test.startTest("Command Builders - SPEED_NEG");
    auto speedNeg = TurboMIDI::CommandBuilder::buildSpeedNeg(
        TurboMIDI::SpeedMultiplier::SPEED_4X, 
        TurboMIDI::SpeedMultiplier::SPEED_2X
    );
    expected = {0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x12, 0x04, 0x02, 0xF7};
    test.verify(speedNeg == expected, "SPEED_NEG message incorrect");
    test.endTest();
    
    test.startTest("Command Builders - SPEED_TEST");
    auto speedTest = TurboMIDI::CommandBuilder::buildSpeedTest();
    expected = {0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x14, 
                0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0xF7};
    test.verify(speedTest == expected, "SPEED_TEST message incorrect");
    test.endTest();
}

void testSpeedConfig(TestFramework& test) {
    test.startTest("SpeedConfig - Add and check speeds");
    TurboMIDI::SpeedConfig config;
    
    config.addSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X, true);
    config.addSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X, false);
    config.addSpeed(TurboMIDI::SpeedMultiplier::SPEED_16X, true);
    
    test.verify(config.hasSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X), "Should have 2X speed");
    test.verify(config.hasSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X), "Should have 4X speed");
    test.verify(config.hasSpeed(TurboMIDI::SpeedMultiplier::SPEED_16X), "Should have 16X speed");
    test.verify(!config.hasSpeed(TurboMIDI::SpeedMultiplier::SPEED_8X), "Should not have 8X speed");
    
    test.verify(config.isCertified(TurboMIDI::SpeedMultiplier::SPEED_2X), "2X should be certified");
    test.verify(!config.isCertified(TurboMIDI::SpeedMultiplier::SPEED_4X), "4X should not be certified");
    test.verify(config.isCertified(TurboMIDI::SpeedMultiplier::SPEED_16X), "16X should be certified");
    
    test.endTest();
    
    test.startTest("SpeedConfig - Bit masks");
    test.verify(config.mask1 == 0x05, "mask1 should be 0x05 (bits 0 and 2)");
    test.verify(config.mask2 == 0x02, "mask2 should be 0x02 (bit 1 for 16X)");
    test.verify(config.cert1 == 0x01, "cert1 should be 0x01 (bit 0)");
    test.verify(config.cert2 == 0x02, "cert2 should be 0x02 (bit 1)");
    test.endTest();
}

void testMasterSlaveNegotiation(TestFramework& test) {
    test.startTest("Master-Slave Speed Negotiation");
    
    // Create master and slave platforms
    MockPlatform masterPlatform;
    MockPlatform slavePlatform;
    
    TurboMIDI::TurboMIDI master(&masterPlatform, TurboMIDI::DeviceRole::MASTER);
    TurboMIDI::TurboMIDI slave(&slavePlatform, TurboMIDI::DeviceRole::SLAVE);
    
    // Configure slave speeds
    slave.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X, true);
    slave.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X, true);
    
    // Simulate master sending SPEED_REQ
    masterPlatform.clearBuffers();
    slavePlatform.clearBuffers();
    
    // Master initiates negotiation
    master.negotiateSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X);
    
    // Check that master sent SPEED_REQ
    test.verify(masterPlatform.findMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x10, 0xF7}),
                "Master should send SPEED_REQ");
    
    // Simulate slave receiving SPEED_REQ and responding
    slavePlatform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x10, 0xF7});
    slave.handleIncomingData();
    
    // Check that slave sent SPEED_ANSWER
    auto slaveResponse = slavePlatform.getLastMessage();
    test.verify(slaveResponse.size() == 12, "SPEED_ANSWER should be 12 bytes");
    test.verify(slaveResponse[6] == 0x11, "Should be SPEED_ANSWER command");
    
    test.endTest();
}

void testActiveSensing(TestFramework& test) {
    test.startTest("Active Sensing");
    
    MockPlatform platform;
    TurboMIDI::TurboMIDI turbo(&platform, TurboMIDI::DeviceRole::MASTER);
    
    // No active sensing at 1x speed
    platform.clearBuffers();
    turbo.sendActiveSense();
    test.verify(platform.txBuffer.empty(), "No active sensing at 1x speed");
    
    // Force speed to 2x
    turbo.pushSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X);
    
    // Now active sensing should be sent
    platform.clearBuffers();
    turbo.sendActiveSense();
    test.verify(platform.txBuffer.size() == 1, "Active sensing should be sent");
    test.verify(platform.txBuffer[0] == 0xFE, "Active sensing byte should be 0xFE");
    
    test.endTest();
}

void testTimeouts(TestFramework& test) {
    test.startTest("Timeout Handling");
    
    MockPlatform platform;
    TurboMIDI::TurboMIDI turbo(&platform, TurboMIDI::DeviceRole::SLAVE);
    
    // Set speed changed callback
    bool speedChangedCalled = false;
    TurboMIDI::SpeedMultiplier lastSpeed = TurboMIDI::SpeedMultiplier::SPEED_1X;
    turbo.onSpeedChanged = [&](TurboMIDI::SpeedMultiplier speed) {
        speedChangedCalled = true;
        lastSpeed = speed;
    };
    
    // Force speed to 4x
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x20, 0x04, 0xF7});
    turbo.handleIncomingData();
    
    test.verify(speedChangedCalled, "Speed change callback should be called");
    test.verify(lastSpeed == TurboMIDI::SpeedMultiplier::SPEED_4X, "Speed should be 4x");
    test.verify(platform.currentBaudRate == 125000, "Baud rate should be 125000");
    
    // Simulate 250ms passing without messages
    platform.currentTime += 250;
    turbo.handleIncomingData();
    test.verify(turbo.getCurrentSpeed() == TurboMIDI::SpeedMultiplier::SPEED_4X, 
                "Speed should still be 4x after 250ms");
    
    // Simulate 350ms total without messages (> 300ms timeout)
    platform.currentTime += 100;
    speedChangedCalled = false;
    turbo.handleIncomingData();
    
    test.verify(speedChangedCalled, "Speed change callback should be called on timeout");
    test.verify(lastSpeed == TurboMIDI::SpeedMultiplier::SPEED_1X, "Speed should revert to 1x");
    test.verify(platform.currentBaudRate == 31250, "Baud rate should revert to 31250");
    
    test.endTest();
}

void testSpeedPush(TestFramework& test) {
    test.startTest("Speed Push Command");
    
    MockPlatform platform;
    TurboMIDI::TurboMIDI master(&platform, TurboMIDI::DeviceRole::MASTER);
    
    // Push speed to 8x
    platform.clearBuffers();
    master.pushSpeed(TurboMIDI::SpeedMultiplier::SPEED_8X);
    
    // Check message sent
    auto message = platform.getLastMessage();
    std::vector<uint8_t> expected = {0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x20, 0x07, 0xF7};
    test.verify(message == expected, "SPEED_PUSH message incorrect");
    
    // Check speed changed
    test.verify(master.getCurrentSpeed() == TurboMIDI::SpeedMultiplier::SPEED_8X, 
                "Master speed should be 8x");
    test.verify(platform.currentBaudRate == 250000, "Baud rate should be 250000");
    
    test.endTest();
}

void testInvalidMessages(TestFramework& test) {
    test.startTest("Invalid Message Handling");
    
    MockPlatform platform;
    TurboMIDI::TurboMIDI turbo(&platform, TurboMIDI::DeviceRole::SLAVE);
    
    bool speedChangedCalled = false;
    turbo.onSpeedChanged = [&](TurboMIDI::SpeedMultiplier speed) {
        speedChangedCalled = true;
    };
    
    // Invalid manufacturer ID
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3D, 0x00, 0x00, 0x20, 0x02, 0xF7});
    turbo.handleIncomingData();
    test.verify(!speedChangedCalled, "Should ignore message with wrong manufacturer ID");
    
    // Too short message
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0xF7});
    turbo.handleIncomingData();
    test.verify(!speedChangedCalled, "Should ignore too short message");
    
    // Message without SysEx end
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x20, 0x02});
    turbo.handleIncomingData();
    test.verify(!speedChangedCalled, "Should ignore message without SysEx end");
    
    // Speed push with unsupported speed
    // First configure device to only support 1x and 2x speeds
    turbo.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_2X, true);
    // Now try to push to 20x speed (which is not supported)
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x20, 0x0B, 0xF7}); // 20x
    turbo.handleIncomingData();
    test.verify(!speedChangedCalled, "Should ignore push for unsupported speed");
    
    test.endTest();
}

void testSlaveSpeedTest(TestFramework& test) {
    test.startTest("Slave Speed Test Sequence");
    
    MockPlatform platform;
    TurboMIDI::TurboMIDI slave(&platform, TurboMIDI::DeviceRole::SLAVE);
    
    // Configure slave with uncertified 4x speed
    slave.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_4X, false);
    slave.setSupportedSpeed(TurboMIDI::SpeedMultiplier::SPEED_8X, true);
    
    bool speedChangedCalled = false;
    TurboMIDI::SpeedMultiplier finalSpeed = TurboMIDI::SpeedMultiplier::SPEED_1X;
    slave.onSpeedChanged = [&](TurboMIDI::SpeedMultiplier speed) {
        speedChangedCalled = true;
        finalSpeed = speed;
    };
    
    // Receive SPEED_NEG for 4x (uncertified), test at 8x
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x12, 0x07, 0x04, 0xF7});
    slave.handleIncomingData();
    
    // Should send ACK
    test.verify(platform.findMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x13, 0xF7}),
                "Slave should send ACK");
    
    // Receive SPEED_TEST
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x14, 
                           0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0xF7});
    slave.handleIncomingData();
    
    // Should switch to test speed (8x) and send result
    test.verify(platform.currentBaudRate == 250000, "Should switch to test speed 8x");
    test.verify(platform.findMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x15, 
                                     0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0xF7}),
                "Slave should send SPEED_RESULT");
    
    // Receive SPEED_TEST2
    platform.clearBuffers();
    platform.injectMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x16, 0xF7});
    slave.handleIncomingData();
    
    // Should send SPEED_RESULT2 and switch to target speed
    test.verify(platform.findMessage({0xF0, 0x00, 0x20, 0x3C, 0x00, 0x00, 0x17, 0xF7}),
                "Slave should send SPEED_RESULT2");
    test.verify(speedChangedCalled, "Speed change callback should be called");
    test.verify(finalSpeed == TurboMIDI::SpeedMultiplier::SPEED_4X, "Final speed should be 4x");
    test.verify(platform.currentBaudRate == 125000, "Final baud rate should be 125000");
    
    test.endTest();
}

// Main test runner
int main() {
    TestFramework test;
    
    std::cout << "Running TurboMIDI Tests\n";
    std::cout << "========================================\n";
    
    // Run all tests
    testCommandBuilders(test);
    testSpeedConfig(test);
    testMasterSlaveNegotiation(test);
    testActiveSensing(test);
    testTimeouts(test);
    testSpeedPush(test);
    testInvalidMessages(test);
    testSlaveSpeedTest(test);
    
    // Print summary
    test.printSummary();
    
    return 0;
}