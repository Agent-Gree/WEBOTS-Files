#include <SPI.h>
#include <mcp_can.h>
#include "can_protocol.h"

#define CS_PIN  5
#define INT_PIN 4

MCP_CAN CAN(CAN_CS_PIN);


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


// -----------------------------------------------------------
// Send a target position
// -----------------------------------------------------------
void send_target_position(int32_t counts) {
    byte data[8] = {0};
    data[0] = SETPOINT_POSITION;
    PACK_INT32(data, BYTE_SETPOINT_VAL, counts);
    CAN.sendMsgBuf(MSG_SETPOSITION, 1, 8, data);
    Serial.printf("Setpoint sent: %ld counts\n", counts);
}

// -----------------------------------------------------------------------------
// Receive an incoming CAN frame and route to the correct decoder
// -----------------------------------------------------------------------------
void receive_and_decode() {
    long unsigned int rxId;
    unsigned char rxLen;
    unsigned char rxBuf[8];

    CAN.readMsgBuf(&rxId, &rxLen, rxBuf);


    // Strip the extended frame flag bit before routing (so it doesn't read the extended flag)
    rxId &= 0x1FFFFFFF;

    switch (rxId) {
        case MSG_MOTOR_TELEMETRY:
            decode_motor_telemetry(rxBuf, rxLen);
            break;
        
        case MSG_MOTOR_POSITION:
            decode_motor_position(rxBuf, rxLen);
            break;

        default:
            Serial.printf("Unknown message ID: 0x%03lX\n", rxId);
            break;
    }
}

// -----------------------------------------------------------------------------
// Decode a MSG_MOTOR_TELEMETRY frame and print values to Serial
// -----------------------------------------------------------------------------
void decode_motor_telemetry(unsigned char *data, unsigned char len) {
    if (len < 7) {
        Serial.println("MotorStatus frame too short — ignoring");
        return;
    }

    // Unpack raw values using shared macros
    int16_t  raw_current = UNPACK_INT16 (data, BYTE_CURRENT_HIGH);
    uint16_t raw_voltage = UNPACK_UINT16(data, BYTE_VOLTAGE_HIGH);
    uint16_t raw_temp    = UNPACK_UINT16(data, BYTE_TEMP_HIGH);
    byte     flags       = data[BYTE_FLAGS];

    // Apply scaling
    float current     = raw_current * SCALE_CURRENT;
    float voltage     = raw_voltage * SCALE_VOLTAGE;
    float temperature = raw_temp * SCALE_TEMPERATURE;

    // Decode flags
    bool enabled = flags & FLAG_ENABLED;  // indictes if motor is alive 
    bool fault   = flags & FLAG_FAULT;    // indicates if something is wrong

    // Print decoded values
    Serial.println("--- Motor Status ---");
    Serial.printf("  Current:     %.2f A\n", current);
    Serial.printf("  Voltage: %.2fV\n", voltage);
    Serial.printf("  Temperature: %.1f degC\n", temperature);
    Serial.printf("  Enabled:     %s\n", enabled ? "YES" : "NO");
    Serial.printf("  Fault:       %s\n", fault   ? "YES" : "NO");
    Serial.println("--------------------");
}

// -----------------------------------------------------------
// Decode motor position frame
// -----------------------------------------------------------
void decode_motor_position(unsigned char *data, unsigned char len) {
    if (len < 6) {
        Serial.println("Telemetry frame too short");
        return;
    }

    int32_t  position    = UNPACK_INT32 (data, BYTE_POSITION_3);
    byte     flags       = data[BYTE_FLAGS];


    // Decode flags
    bool enabled = flags & FLAG_ENABLED;  // indictes if motor is alive 
    bool fault   = flags & FLAG_FAULT;    // indicates if something is wrong


    Serial.printf("Position: %ld counts\n", position);
}


// -----------------------------------------------------------------------------
// Main loop — request all motor values every second, handle any responses
// -----------------------------------------------------------------------------
void loop() {

    /* change to send position whenever it is needed
    // Send a request evvery 1000ms
    static unsigned long lastRequest = 0; 
    if (millis() - lastRequest >= 1000) {
        send_target_position(60);
        lastRequest = millis();
    }
    */

    // Check for incoming messages
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        receive_and_decode();
    }

    // Add temporarily to loop() to catch MCP2515 errors
    byte error = CAN.getError();
    if (error) {
        Serial.printf("CAN error register: 0x%02X\n", error);
    }
}

