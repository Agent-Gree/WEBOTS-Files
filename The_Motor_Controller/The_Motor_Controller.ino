#include <SPI.h>
#include <mcp_can.h>
#include "can_protocol.h"

MCP_CAN CAN(CAN_CS_PIN);

// -----------------------------------------------------------------------------
// Motor state — replace these with your real sensor reads
// -----------------------------------------------------------------------------
bool motor_enabled = true;
bool motor_fault   = false;



void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Motor Controller ESP32 starting...");

    if (CAN.begin(MCP_ANY, CAN_SPEED, CAN_CRYSTAL) == CAN_OK) {
        Serial.println("CAN init OK");
    } else {
        Serial.println("CAN init FAILED — check wiring and crystal frequency");
        while (1);  // halt
    }

    CAN.setMode(MCP_NORMAL);
    Serial.println("Motor controller ready.");
}

// -----------------------------------------------------------------------------
// Main loop — listen for requests and respond
// -----------------------------------------------------------------------------
void loop() {
  if (CAN_MSGAVAIL == Can.checkReceive()) {
    long unsigned int rxId;
    unsigned char rxLen;
    unsigned char rxBuf[8];

    Can.readMsgBuf(&rxId, &rxLen, rxBuf);

    switch (rxId) {
      case MSG_MOTOR_REQUEST:
        handle_request(rxBuf[0]);
        break;
      default:
        // Ignore unknown messages
        break;
    }
  }
}

// -----------------------------------------------------------------------------
// Route a request to the correct response function
// -----------------------------------------------------------------------------
void handle_request(byte requestId) {
  Serial.printf("Request recieved: 0x%02X\n", requestId);

  switch (requestId) {
    case REQ_CURRENT:
      send_current_only();
      break;

    case REQ_RPM:
      send_rpm_only();
      break;

    case REQ_TEMPERATURE:
      send_temperature_only();
      break;

    case REQ_ALL:
      send_motor_status();
      break;

    default:
      Serial.printf("Unknown request ID: 0x%02X\n", requestId);
      break;
  }
}

// -----------------------------------------------------------------------------
// Sensor read functions — replace with real hardware reads
// -----------------------------------------------------------------------------
float read_current() {
  // TODO: replace with ADC read from your current sensor
  // Example: return analogRead(CURRENT_PIN) * (3.3 / 4095.0) * AMPS_PER_VOLT;
  return 10.15f;  // placeholder
}

float read_rpm()  {
  // TODO: replace with encoder or hall sensor read
  return 1500.0f;  // placeholder
}

float read_temperature()  {
  // TODO: replace with NTC thermistor or sensor read
  return 45.2f;  // placeholder
}


// -----------------------------------------------------------------------------
// Send full MotorStatus frame (responds to REQ_ALL)
// -----------------------------------------------------------------------------
void send_motor_status() {
  float current     = read_current();
  float rpm         = read_rpm();
  float temperature = read_temperature();

  // Scale to raw integers
  int16_t  raw_current = (int16_t)(current / SCALE_CURRENT);
  int16_t  raw_rpm     = (int16_t)(rpm);
  uint16_t raw_temp    = (uint16_t)(temperature / SCALE_TEMPERATURE);

  // Build flags byte
  byte flags = 0;
  if (motor_enabled) flags |= FLAG_ENABLED;
  if (motor_fault)   flags |= FLAG_FAULT;

  // Pack into 8-byte buffer using shared macros
  byte data[8] = {0};
  PACK_INT16 (data, BYTE_CURRENT_HIGH, raw_current);
  PACK_INT16 (data, BYTE_RPM_HIGH,     raw_rpm);
  PACK_UINT16(data, BYTE_TEMP_HIGH,    raw_temp);
  data[BYTE_FLAGS]    = flags;
  data[BYTE_RESERVED] = 0;

  byte result = CAN.sendMsgBuf(MSG_MOTOR_STATUS, 1, 8, data);

  if (result == CAN_OK) {
    Serial.println("MotorStatus sent");
  } else {
    Serial.println("MotorStatus send FAILED");
  }
}

// -----------------------------------------------------------------------------
// Send current only — packs only current into the frame.
// Still uses MSG_MOTOR_STATUS ID so master decode stays the same,
// but only bytes 0-1 are valid. RPM and temp will read as 0.
// -----------------------------------------------------------------------------
void send_current_only() {
  float current    = read_current();
  int16_t raw      = (int16_t)(current / SCALE_CURRENT);

  byte data[8] = {0};
  PACK_INT16(data, BYTE_CURRENT_HIGH, raw);

  CAN.sendMsgBuf(MSG_MOTOR_STATUS, 1, 8, data);
  Serial.printf("Current sent: %.2f A\n", current);
}

// -----------------------------------------------------------------------------
// Send RPM only
// -----------------------------------------------------------------------------
void send_rpm_only() {
    float rpm    = read_rpm();
    int16_t raw  = (int16_t)(rpm);

    byte data[8] = {0};
    PACK_INT16(data, BYTE_RPM_HIGH, raw);

    CAN.sendMsgBuf(MSG_MOTOR_STATUS, 1, 8, data);
    Serial.printf("RPM sent: %.0f RPM\n", rpm);
}


// -----------------------------------------------------------------------------
// Send temperature only
// -----------------------------------------------------------------------------
void send_temperature_only() {
    float temperature = read_temperature();
    uint16_t raw      = (uint16_t)(temperature / SCALE_TEMPERATURE);

    byte data[8] = {0};
    PACK_UINT16(data, BYTE_TEMP_HIGH, raw);

    CAN.sendMsgBuf(MSG_MOTOR_STATUS, 1, 8, data);
    Serial.printf("Temperature sent: %.1f degC\n", temperature);
}

