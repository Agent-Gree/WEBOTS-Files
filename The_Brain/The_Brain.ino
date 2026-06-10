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


// -----------------------------------------------------------------------------
// Send a request to the motor controller
// requestId: one of REQ_CURRENT, REQ_RPM, REQ_TEMPERATURE, REQ_ALL
// -----------------------------------------------------------------------------
void request_motor_data(byte requestID) {
    byte data[1] = {requestID};
    byte result = CAN.sendMsgBuf(MSG_MOTOR_REQUEST, 1, 1, data);

    if (result == CAN_OK) {
        Serial.printf("Request sent: 0x%02X\n", requestID);
    } else {
        Serial.println("Request send FAILED");
    }
}


void request_motor_telemetry(){
    byte data[1] = {0};
    byte result = CAN.sendMsgBuf(MSG_MOTOR_TELEMETRY, 1, 1, data);

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
        case MSG_MOTOR_STATUS:
            decode_motor_status(rxBuf, rxLen);
            break;
        
        case MSG_MOTOR_TELEMETRY:
            decode_motor_telemetry(rxBuf, rxLen);
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
    bool enabled = flags & FLAG_ENABLED;  // indictes if motor is alive 
    bool fault   = flags & FLAG_FAULT;    // indicates if something is wrong

    // Print decoded values
    Serial.println("--- Motor Status ---");
    Serial.printf("  Current:     %.2f A\n",   current);
    Serial.printf("  RPM:         %.0f RPM\n", rpm);
    Serial.printf("  Temperature: %.1f degC\n", temperature);
    Serial.printf("  Enabled:     %s\n", enabled ? "YES" : "NO");
    Serial.printf("  Fault:       %s\n", fault   ? "YES" : "NO");
    Serial.println("--------------------");
}

// -----------------------------------------------------------------------------
// Setting the PID gain constants
// -----------------------------------------------------------------------------
void set_pid_gains(float Kp, float Ki, float Kd) {
    byte data[8] = {0};
    PACK_INT16(data, BYTE_KP_HIGH, (int16_t)(Kp / SCALE_PID_GAIN));
    PACK_INT16(data, BYTE_KI_HIGH, (int16_t)(Ki / SCALE_PID_GAIN));
    PACK_INT16(data, BYTE_KD_HIGH, (int16_t)(Kd / SCALE_PID_GAIN));
    CAN.sendMsgBuf(MSG_PID_SET, 1, 8, data);
}

// -----------------------------------------------------------------------------
// Requesting the value of the PID gain constants
// -----------------------------------------------------------------------------
void request_pid_gains() {
    byte data[1] = {0};
    CAN.sendMsgBuf(MSG_PID_REQUEST, 1, 1, data);
}



// -----------------------------------------------------------
// Send a target RPM
// -----------------------------------------------------------
void set_target_rpm(float rpm) {
    byte data[8] = {0};
    data[BYTE_SETPOINT_MODE] = SETPOINT_RPM;
    PACK_INT32(data, BYTE_SETPOINT_VAL, (int32_t)rpm);
    CAN.sendMsgBuf(MSG_SETPOINT, 1, 8, data);
    Serial.printf("Setpoint sent: %.0f RPM\n", rpm);
}

// -----------------------------------------------------------
// Send a target position
// -----------------------------------------------------------
void set_target_position(int32_t counts) {
    byte data[8] = {0};
    data[BYTE_SETPOINT_MODE] = SETPOINT_POSITION;
    PACK_INT32(data, BYTE_SETPOINT_VAL, counts);
    CAN.sendMsgBuf(MSG_SETPOINT, 1, 8, data);
    Serial.printf("Setpoint sent: %ld counts\n", counts);
}

// -----------------------------------------------------------
// Send home command
// -----------------------------------------------------------
void send_home() {
    byte data[1] = {0};
    CAN.sendMsgBuf(MSG_HOME, 1, 1, data);
    Serial.println("Home command sent");
}

// -----------------------------------------------------------
// Decode telemetry frame
// -----------------------------------------------------------
void decode_motor_telemetry(unsigned char *data, unsigned char len) {
    if (len < 6) {
        Serial.println("Telemetry frame too short");
        return;
    }

    uint16_t raw_voltage = UNPACK_UINT16(data, BYTE_VOLTAGE_HIGH);
    int32_t  position    = UNPACK_INT32 (data, BYTE_POSITION_3);

    float voltage = raw_voltage * SCALE_VOLTAGE;

    Serial.printf("Voltage: %.2fV  Position: %ld counts\n",
                  voltage, position);
}


// -----------------------------------------------------------------------------
// Main loop — request all motor values every second, handle any responses
// -----------------------------------------------------------------------------
void loop() {
    // Send a request evvery 1000ms
    static unsigned long lastRequest = 0; 
    if (millis() - lastRequest >= 1000) {
        request_motor_data(REQ_ALL);
        lastRequest = millis();
    }
    
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

