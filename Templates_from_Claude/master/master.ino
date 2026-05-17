// =============================================================================
// master.ino
// Master ESP32 — sends requests, receives and decodes motor status frames.
// Requires: can_protocol.h in the same project folder.
// Libraries: SPI (built-in), mcp_can (install via Library Manager)
// =============================================================================

#include <SPI.h>
#include <mcp_can.h>
#include "can_protocol.h"

MCP_CAN CAN(CAN_CS_PIN);

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Master ESP32 starting...");

    if (CAN.begin(MCP_ANY, CAN_SPEED, CAN_CRYSTAL) == CAN_OK) {
        Serial.println("CAN init OK");
    } else {
        Serial.println("CAN init FAILED — check wiring and crystal frequency");
        while (1);  // halt
    }

    CAN.setMode(MCP_NORMAL);
    Serial.println("Master ready.");
}

// -----------------------------------------------------------------------------
// Main loop — request all motor values every second, handle any responses
// -----------------------------------------------------------------------------
void loop() {
    // Send a request every 1000ms
    static unsigned long lastRequest = 0;
    if (millis() - lastRequest >= 1000) {
        request_motor_data(REQ_ALL);
        lastRequest = millis();
    }

    // Check for incoming messages
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        receive_and_decode();
    }
}

// -----------------------------------------------------------------------------
// Send a request to the motor controller
// requestId: one of REQ_CURRENT, REQ_RPM, REQ_TEMPERATURE, REQ_ALL
// -----------------------------------------------------------------------------
void request_motor_data(byte requestId) {
    byte data[1] = { requestId };
    byte result = CAN.sendMsgBuf(MSG_MOTOR_REQUEST, 1, 1, data);

    if (result == CAN_OK) {
        Serial.printf("Request sent: 0x%02X\n", requestId);
    } else {
        Serial.println("Request send FAILED");
    }
}

// -----------------------------------------------------------------------------
// Receive an incoming CAN frame and route to the correct decoder
// -----------------------------------------------------------------------------
void receive_and_decode() {
    long unsigned int rxId;
    unsigned char rxLen;
    unsigned char rxBuf[8];

    CAN.readMsgBuf(&rxId, &rxLen, rxBuf);

    switch (rxId) {
        case MSG_MOTOR_STATUS:
            decode_motor_status(rxBuf, rxLen);
            break;

        default:
            Serial.printf("Unknown message ID: 0x%03lX\n", rxId);
            break;
    }
}

// -----------------------------------------------------------------------------
// Decode a MSG_MOTOR_STATUS frame and print values to Serial
// -----------------------------------------------------------------------------
void decode_motor_status(unsigned char *data, unsigned char len) {
    if (len < 7) {
        Serial.println("MotorStatus frame too short — ignoring");
        return;
    }

    // Unpack raw values using shared macros
    int16_t  raw_current = UNPACK_INT16 (data, BYTE_CURRENT_HIGH);
    int16_t  raw_rpm     = UNPACK_INT16 (data, BYTE_RPM_HIGH);
    uint16_t raw_temp    = UNPACK_UINT16(data, BYTE_TEMP_HIGH);
    byte     flags       = data[BYTE_FLAGS];

    // Apply scaling
    float current     = raw_current * SCALE_CURRENT;
    float rpm         = (float)raw_rpm;
    float temperature = raw_temp * SCALE_TEMPERATURE;

    // Decode flags
    bool enabled = flags & FLAG_ENABLED;
    bool fault   = flags & FLAG_FAULT;

    // Print decoded values
    Serial.println("--- Motor Status ---");
    Serial.printf("  Current:     %.2f A\n",   current);
    Serial.printf("  RPM:         %.0f RPM\n", rpm);
    Serial.printf("  Temperature: %.1f degC\n", temperature);
    Serial.printf("  Enabled:     %s\n", enabled ? "YES" : "NO");
    Serial.printf("  Fault:       %s\n", fault   ? "YES" : "NO");
    Serial.println("--------------------");
}
