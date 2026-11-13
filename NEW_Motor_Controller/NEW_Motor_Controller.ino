#include <SPI.h>
#include <mcp_can.h>
#include "can_protocol.h"

MCP_CAN CAN(CAN_CS_PIN);

// -----------------------------------------------------------------------------
// GLOBAL VARIABLES
// -----------------------------------------------------------------------------
bool motor_enabled = true;
bool motor_fault   = false;

int32_t POSITION = 0; // global encoder counter (read value from encoder on motor)   -> encoder_count = esp32.readencoder()  


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
// Sensor read functions — replace with real hardware reads
// -----------------------------------------------------------------------------
float read_current() {
  // TODO: replace with ADC read from your current sensor
  // Example: return analogRead(CURRENT_PIN) * (3.3 / 4095.0) * AMPS_PER_VOLT;
  return 10.15f;  // placeholder
}

float read_temperature()  {
  // TODO: replace with NTC thermistor or sensor read
  return 45.2f;  // placeholder
}

float read_position() {
  // TODO: replace with encoder or hall sensor readings
  POSITION = encoder_value; // Jaylen said that drivers will give position in degrees, so no need to do any math! 
  return POSITION;
}

float read_voltage()  {
  // TODO: replace with some voltage readings
  return 17.3f;
}

// -----------------------------------------------------------------------------
// Send full MotorTelemetry frame (responds to REQ_ALL)
// -----------------------------------------------------------------------------
void send_motor_telemetry() {
  float current     = read_current();
  float voltage  = read_voltage();
  float temperature = read_temperature();

  // Scale to raw integers
  int16_t  raw_current = (int16_t)(current / SCALE_CURRENT);
  uint16_t raw_temp    = (uint16_t)(temperature / SCALE_TEMPERATURE);
  uint16_t raw_voltage = (uint16_t)(voltage / SCALE_VOLTAGE);

  // Build flags byte
  byte flags = 0;
  if (motor_enabled) flags |= FLAG_ENABLED;
  if (motor_fault)   flags |= FLAG_FAULT;

  // Pack into 8-byte buffer using shared macros
  byte data[8] = {0};
  PACK_INT16 (data, BYTE_CURRENT_HIGH, raw_current);
  PACK_UINT16(data, BYTE_VOLTAGE_HIGH, raw_voltage);
  PACK_UINT16(data, BYTE_TEMP_HIGH,    raw_temp);
  data[BYTE_FLAGS_TELEMETRY] = flags;
  data[BYTE_RESERVED_TELEMETRY] = 0;

  byte result = CAN.sendMsgBuf(MSG_MOTOR_TELEMETRY, 1, 8, data);

  if (result == CAN_OK) {
    Serial.println("MotorTelemetry sent");
  } else {
    Serial.println("MotorTelemetry send FAILED");
  }
}


// -----------------------------------------------------------
// Send motor position
// -----------------------------------------------------------
void send_motor_position() {
    int32_t position = POSITION;

    // Build flags byte
    byte flags = 0;
    if (motor_enabled) flags |= FLAG_ENABLED;
    if (motor_fault)   flags |= FLAG_FAULT;

  // Pack into 8-byte buffer 
    byte data[8] = {0};
    PACK_INT32 (data, BYTE_POSITION_3, position);
    data[BYTE_FLAGS_POSITION] = flags;
    data[BYTE_RESERVED_POSITION] = 0;

    byte result = CAN.sendMsgBuf(MSG_MOTOR_POSITION, 1, 8, data);
      
    if (result == CAN_OK) {
      Serial.println("MotorPosition sent");
    } else {
      Serial.println("MotorPosition send FAILED");
    }

}


// -----------------------------------------------------------
// Decode motor position frame
// -----------------------------------------------------------
void decode_and_set_motor_position(unsigned char *data, unsigned char len) {
    if (len < 6) {
        Serial.println("Telemetry frame too short");
        return;
    }

    int32_t  position    = UNPACK_INT32 (data, BYTE_POSITION_3);
    byte     flags       = data[BYTE_FLAGS];


    // Decode flags
    bool enabled = flags & FLAG_ENABLED;  // indictes if motor is alive 
    bool fault   = flags & FLAG_FAULT;    // indicates if something is wrong

    // Setting new motor position
    POSITION = position; 

    Serial.printf("New position received: %ld counts\n", position);
}


// -----------------------------------------------------------------------------
// Main loop — listen for requests and respond
// -----------------------------------------------------------------------------
void loop() {
  
  
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    long unsigned int rxId;
    unsigned char rxLen;
    unsigned char rxBuf[8];


    // Send motor telemetry every 2000ms (2 sec) --> requirement for RL
    static unsigned long lastRequest = 0; 
    if (millis() - lastRequest >= 2000) {
        send_motor_telemetry();
        lastRequest = millis();
    }
    
    /*
    // Send motor position at 1000hz (0.001 sec) --> requirement for RL
    may have to figure out how to send it faster than loop? (how fast does the loop fnc run?)
    static unsigned long lastRequest = 0; 
    if (millis() - lastRequest >= 2000) {
        send_motor_position();
        lastRequest = millis();
    }
    */

    CAN.readMsgBuf(&rxId, &rxLen, rxBuf);

    // Strip the extended frame flag bit before routing (so it reads 0x18FF0010 as the ID instead of 0x98FF0010)
    rxId &= 0x1FFFFFFF;

    Serial.printf("Raw frame received — ID: 0x%03lX  Len: %d\n", rxId, rxLen);

    switch (rxId) {
      case MSG_MOTOR_POSITION:
        decode_and_set_motor_position();
        break;

      default:
        // Ignore unknown messages
        break;
    }
  }

  // Add temporarily to loop() to catch MCP2515 errors
  byte error = CAN.getError();
  if (error) {
      Serial.printf("CAN error register: 0x%02X\n", error);
  }
}
