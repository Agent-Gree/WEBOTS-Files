#include <SPI.h>
#include <mcp_can.h>
#include "can_protocol.h"

MCP_CAN CAN(CAN_CS_PIN);

// -----------------------------------------------------------------------------
// Motor state — replace these with your real sensor reads
// -----------------------------------------------------------------------------
bool motor_enabled = true;
bool motor_fault   = false;

// PID struct
typedef struct {
    float Kp, Ki, Kd;      // gains — set via CAN
    float setpoint;         // target RPM — set via CAN
    float integral;         // accumulated error — internal
    float prev_error;       // last error — internal
    unsigned long last_time; // for dt calculation
} PID;
PID motor_pid = {0};  // global PID instance

int encoder_count = 0; // global encoder counter     


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
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    long unsigned int rxId;
    unsigned char rxLen;
    unsigned char rxBuf[8];

    CAN.readMsgBuf(&rxId, &rxLen, rxBuf);

    switch (rxId) {
      case MSG_MOTOR_REQUEST:
        handle_request(rxBuf[0]);
        break;

      case MSG_PID_SET:
        handle_pid_set(rxBuf);
        break;

      case MSG_PID_REQUEST:
        send_pid_status();
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
void handle_request(byte requestID) {
  Serial.printf("Request recieved: 0x%02X\n", requestID);

  switch (requestID) {
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
      Serial.printf("Unknown request ID: 0x%02X\n", requestID);
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

float read_position() {
  // TODO: replace with encoder or hall sensor readings
  return 35.3f;
}

float read_voltage()  {
  // TODO: replace with some voltage readings
  return 17.3f;
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


// -----------------------------------------------------------------------------
// PID struct, functions, and logic
// -----------------------------------------------------------------------------
float pid_compute(PID *pid, float actual) {
    unsigned long now = micros();
    float dt = (now - pid->last_time) / 1000000.0f;  // seconds
    pid->last_time = now;

    if (dt <= 0) return 0;  // guard against zero dt

    float error      = pid->setpoint - actual;
    pid->integral   += error * dt;
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error  = error;

    return (pid->Kp * error)
         + (pid->Ki * pid->integral)
         + (pid->Kd * derivative);
}


// -----------------------------------------------------------------------------
// Reading PID gains constants
// -----------------------------------------------------------------------------
void handle_pid_set(unsigned char *data) {
    int16_t raw_kp = UNPACK_INT16(data, BYTE_KP_HIGH);
    int16_t raw_ki = UNPACK_INT16(data, BYTE_KI_HIGH);
    int16_t raw_kd = UNPACK_INT16(data, BYTE_KD_HIGH);

    motor_pid.Kp = raw_kp * SCALE_PID_GAIN;
    motor_pid.Ki = raw_ki * SCALE_PID_GAIN;
    motor_pid.Kd = raw_kd * SCALE_PID_GAIN;

    // Reset integral and derivative state when gains change
    // otherwise the old accumulated values corrupt the new gains
    motor_pid.integral   = 0;
    motor_pid.prev_error = 0;

    Serial.printf("PID updated — Kp:%.3f Ki:%.3f Kd:%.3f\n",
                  motor_pid.Kp, motor_pid.Ki, motor_pid.Kd);
}


// -----------------------------------------------------------------------------
// Sending the PID gain constants
// -----------------------------------------------------------------------------
void send_pid_status() {
    byte data[8] = {0};
    PACK_INT16(data, BYTE_KP_HIGH, (int16_t)(motor_pid.Kp / SCALE_PID_GAIN));
    PACK_INT16(data, BYTE_KI_HIGH, (int16_t)(motor_pid.Ki / SCALE_PID_GAIN));
    PACK_INT16(data, BYTE_KD_HIGH, (int16_t)(motor_pid.Kd / SCALE_PID_GAIN));
    CAN.sendMsgBuf(MSG_PID_STATUS, 1, 8, data);
}



// Two PID loops — one for each control mode
PID rpm_pid      = {0};
PID position_pid = {0};
byte control_mode = SETPOINT_RPM;  // default mode

// Current setpoints
float    target_rpm      = 0;
int32_t  target_position = 0;

// -----------------------------------------------------------
// Receive and apply a setpoint from master
// -----------------------------------------------------------
void handle_setpoint(unsigned char *data) {
    byte mode       = data[BYTE_SETPOINT_MODE];
    int32_t value   = UNPACK_INT32(data, BYTE_SETPOINT_VAL);

    control_mode = mode;

    switch (mode) {
        case SETPOINT_RPM:
            target_rpm = (float)value;
            rpm_pid.setpoint = target_rpm;

            // Reset position PID state — no longer active
            position_pid.integral   = 0;
            position_pid.prev_error = 0;

            Serial.printf("Mode: RPM  Target: %.0f RPM\n", target_rpm);
            break;

        case SETPOINT_POSITION:
            target_position = value;
            position_pid.setpoint = (float)target_position;

            // Reset RPM PID state
            rpm_pid.integral   = 0;
            rpm_pid.prev_error = 0;

            Serial.printf("Mode: Position  Target: %ld counts\n", target_position);
            break;

        default:
            Serial.printf("Unknown setpoint mode: 0x%02X\n", mode);
            break;
    }
}

// -----------------------------------------------------------
// Handle home command — resets internal position counter
// -----------------------------------------------------------
void handle_home() {
    encoder_count    = 0;      // reset your encoder variable to zero
    position_pid.integral   = 0;
    position_pid.prev_error = 0;
    Serial.println("Homed — position reset to 0");
}

// -----------------------------------------------------------
// Send telemetry (voltage + position)
// -----------------------------------------------------------
void send_motor_telemetry() {
    float   voltage  = read_voltage();
    int32_t position = encoder_count;

    uint16_t raw_voltage = (uint16_t)(voltage / SCALE_VOLTAGE);

    byte data[8] = {0};
    PACK_UINT16(data, BYTE_VOLTAGE_HIGH, raw_voltage);
    PACK_INT32 (data, BYTE_POSITION_3,   position);

    CAN.sendMsgBuf(MSG_MOTOR_TELEMETRY, 1, 8, data);
}

// -----------------------------------------------------------
// Control loop — call this as fast as possible in loop()
// -----------------------------------------------------------
void run_control_loop() {
    float actual_rpm      = read_rpm();
    int32_t actual_pos    = encoder_count;
    float output          = 0;

    switch (control_mode) {
        case SETPOINT_RPM:
            output = pid_compute(&rpm_pid, actual_rpm);
            break;

        case SETPOINT_POSITION:
            output = pid_compute(&position_pid, (float)actual_pos);
            break;
    }

    // Clamp output to valid PWM range
    output = constrain(output, -255, 255);

    // TODO: apply output to your motor driver
    // e.g. analogWrite(MOTOR_PWM_PIN, (int)output);
}