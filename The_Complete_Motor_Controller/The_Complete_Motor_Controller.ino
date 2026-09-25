// =============================================================================
// The_Motor_Controller.ino
// Refactored for ESP32 Native CAN (TWAI) Driver
// =============================================================================

#include <driver/twai.h>
#include "can_protocol.h"

// -----------------------------------------------------------------------------
// Hardware Configuration
// Replace these GPIO numbers with the CAN pins on your development board!
// -----------------------------------------------------------------------------
#define CAN_TX_PIN          GPIO_NUM_1  
#define CAN_RX_PIN          GPIO_NUM_2  

// -----------------------------------------------------------------------------
// Motor State & Globals
// -----------------------------------------------------------------------------
bool motor_enabled = true;
bool motor_fault   = false;

// PID struct
typedef struct {
    float Kp, Ki, Kd;       // gains — set via CAN
    float setpoint;         // target RPM/Position — set via CAN
    float integral;         // accumulated error — internal
    float prev_error;       // last error — internal
    unsigned long last_time;// for dt calculation
} PID;

PID rpm_pid      = {0};
PID position_pid = {0};
PID motor_pid    = {0};     // global PID instance for gain requests

byte control_mode = SETPOINT_RPM; // default mode

// Current setpoints
float   target_rpm      = 0.0f;
int32_t target_position = 0;

int32_t encoder_count   = 0; // global encoder counter

// Forward Declarations
void send_current_only();
void send_rpm_only();
void send_temperature_only();
void send_motor_status();
void send_motor_telemetry();
void send_pid_status();

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Motor Controller ESP32 starting (Native TWAI CAN)...");

    // Configure CAN Pins, Speed (500 kbps), and Operating Mode
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install and Start the TWAI Driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK &&
        twai_start() == ESP_OK) {
        Serial.println("Native CAN (TWAI) Init OK");
    } else {
        Serial.println("CAN Init FAILED — check pin assignments and transceivers");
        while (1); // Halt
    }

    Serial.println("Motor controller ready.");
}

// -----------------------------------------------------------------------------
// Sensor Read Functions — Replace placeholders with real hardware reads
// -----------------------------------------------------------------------------
float read_current() {
    // TODO: replace with ADC read from your current sensor amplifier
    return 10.15f;  // placeholder
}

float read_rpm() {
    // TODO: replace with encoder velocity read
    return 1500.0f; // placeholder
}

float read_temperature() {
    // TODO: replace with thermistor / temperature sensor read
    return 45.2f;   // placeholder
}

float read_position() {
    return (float)encoder_count;
}

float read_voltage() {
    // TODO: replace with voltage divider ADC read
    return 17.3f;   // placeholder
}

// -----------------------------------------------------------------------------
// Request Router
// -----------------------------------------------------------------------------
void handle_request(byte requestID) {
    Serial.printf("Request received: 0x%02X\n", requestID);

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
// CAN Transmit Functions (TWAI Implementation)
// -----------------------------------------------------------------------------
void send_motor_status() {
    float current     = read_current();
    float rpm         = read_rpm();
    float temperature = read_temperature();

    int16_t  raw_current = (int16_t)(current / SCALE_CURRENT);
    int16_t  raw_rpm     = (int16_t)(rpm);
    uint16_t raw_temp    = (uint16_t)(temperature / SCALE_TEMPERATURE);

    byte flags = 0;
    if (motor_enabled) flags |= FLAG_ENABLED;
    if (motor_fault)   flags |= FLAG_FAULT;

    twai_message_t msg;
    msg.identifier = MSG_MOTOR_STATUS;
    msg.extd = 1; // 29-bit Extended Frame
    msg.data_length_code = 8;

    PACK_INT16 (msg.data, BYTE_CURRENT_HIGH, raw_current);
    PACK_INT16 (msg.data, BYTE_RPM_HIGH,     raw_rpm);
    PACK_UINT16(msg.data, BYTE_TEMP_HIGH,    raw_temp);
    msg.data[BYTE_FLAGS]    = flags;
    msg.data[BYTE_RESERVED] = 0;

    if (twai_transmit(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
        Serial.println("MotorStatus sent");
    } else {
        Serial.println("MotorStatus send FAILED");
    }
}

void send_current_only() {
    float current = read_current();
    int16_t raw   = (int16_t)(current / SCALE_CURRENT);

    twai_message_t msg;
    msg.identifier = MSG_MOTOR_STATUS;
    msg.extd = 1;
    msg.data_length_code = 8;
    memset(msg.data, 0, 8);

    PACK_INT16(msg.data, BYTE_CURRENT_HIGH, raw);

    twai_transmit(&msg, pdMS_TO_TICKS(10));
    Serial.printf("Current sent: %.2f A\n", current);
}

void send_rpm_only() {
    float rpm   = read_rpm();
    int16_t raw = (int16_t)(rpm);

    twai_message_t msg;
    msg.identifier = MSG_MOTOR_STATUS;
    msg.extd = 1;
    msg.data_length_code = 8;
    memset(msg.data, 0, 8);

    PACK_INT16(msg.data, BYTE_RPM_HIGH, raw);

    twai_transmit(&msg, pdMS_TO_TICKS(10));
    Serial.printf("RPM sent: %.0f RPM\n", rpm);
}

void send_temperature_only() {
    float temperature = read_temperature();
    uint16_t raw      = (uint16_t)(temperature / SCALE_TEMPERATURE);

    twai_message_t msg;
    msg.identifier = MSG_MOTOR_STATUS;
    msg.extd = 1;
    msg.data_length_code = 8;
    memset(msg.data, 0, 8);

    PACK_UINT16(msg.data, BYTE_TEMP_HIGH, raw);

    twai_transmit(&msg, pdMS_TO_TICKS(10));
    Serial.printf("Temperature sent: %.1f degC\n", temperature);
}

void send_motor_telemetry() {
    float   voltage  = read_voltage();
    int32_t position = (int32_t)read_position();

    uint16_t raw_voltage = (uint16_t)(voltage / SCALE_VOLTAGE);

    twai_message_t msg;
    msg.identifier = MSG_MOTOR_TELEMETRY;
    msg.extd = 1;
    msg.data_length_code = 8;

    PACK_UINT16(msg.data, BYTE_VOLTAGE_HIGH, raw_voltage);
    PACK_INT32 (msg.data, BYTE_POSITION_3,   position);
    msg.data[6] = 0;
    msg.data[7] = 0;

    if (twai_transmit(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
        Serial.println("MotorTelemetry sent");
    } else {
        Serial.println("MotorTelemetry send FAILED");
    }
}

void send_pid_status() {
    twai_message_t msg;
    msg.identifier = MSG_PID_STATUS;
    msg.extd = 1;
    msg.data_length_code = 8;
    memset(msg.data, 0, 8);

    PACK_INT16(msg.data, BYTE_KP_HIGH, (int16_t)(motor_pid.Kp / SCALE_PID_GAIN));
    PACK_INT16(msg.data, BYTE_KI_HIGH, (int16_t)(motor_pid.Ki / SCALE_PID_GAIN));
    PACK_INT16(msg.data, BYTE_KD_HIGH, (int16_t)(motor_pid.Kd / SCALE_PID_GAIN));

    twai_transmit(&msg, pdMS_TO_TICKS(10));
}

// -----------------------------------------------------------------------------
// PID Math & Receive Handlers
// -----------------------------------------------------------------------------
float pid_compute(PID *pid, float actual) {
    unsigned long now = micros();
    float dt = (now - pid->last_time) / 1000000.0f;
    pid->last_time = now;

    if (dt <= 0) return 0.0f;

    float error      = pid->setpoint - actual;
    pid->integral   += error * dt;
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error  = error;

    return (pid->Kp * error)
         + (pid->Ki * pid->integral)
         + (pid->Kd * derivative);
}

void handle_pid_set(unsigned char *data) {
    int16_t raw_kp = UNPACK_INT16(data, BYTE_KP_HIGH);
    int16_t raw_ki = UNPACK_INT16(data, BYTE_KI_HIGH);
    int16_t raw_kd = UNPACK_INT16(data, BYTE_KD_HIGH);

    motor_pid.Kp = raw_kp * SCALE_PID_GAIN;
    motor_pid.Ki = raw_ki * SCALE_PID_GAIN;
    motor_pid.Kd = raw_kd * SCALE_PID_GAIN;

    motor_pid.integral   = 0;
    motor_pid.prev_error = 0;

    Serial.printf("PID updated — Kp:%.3f Ki:%.3f Kd:%.3f\n",
                  motor_pid.Kp, motor_pid.Ki, motor_pid.Kd);
}

void handle_setpoint(unsigned char *data) {
    byte mode     = data[BYTE_SETPOINT_MODE];
    int32_t value = UNPACK_INT32(data, BYTE_SETPOINT_VAL);

    control_mode = mode;

    switch (mode) {
        case SETPOINT_RPM:
            target_rpm = (float)value;
            rpm_pid.setpoint = target_rpm;

            position_pid.integral   = 0;
            position_pid.prev_error = 0;

            Serial.printf("Mode: RPM  Target: %.0f RPM\n", target_rpm);
            break;

        case SETPOINT_POSITION:
            target_position = value;
            position_pid.setpoint = (float)target_position;

            rpm_pid.integral   = 0;
            rpm_pid.prev_error = 0;

            Serial.printf("Mode: Position  Target: %ld counts\n", target_position);
            break;

        default:
            Serial.printf("Unknown setpoint mode: 0x%02X\n", mode);
            break;
    }
}

void handle_home() {
    encoder_count           = 0;
    position_pid.integral   = 0;
    position_pid.prev_error = 0;
    Serial.println("Homed — position reset to 0");
}

// -----------------------------------------------------------------------------
// Control Loop
// -----------------------------------------------------------------------------
void run_control_loop() {
    float actual_rpm   = read_rpm();
    int32_t actual_pos = (int32_t)read_position();
    float output       = 0.0f;

    switch (control_mode) {
        case SETPOINT_RPM:
            output = pid_compute(&rpm_pid, actual_rpm);
            break;

        case SETPOINT_POSITION:
            output = pid_compute(&position_pid, (float)actual_pos);
            break;
    }

    output = constrain(output, -255.0f, 255.0f);

    // TODO: apply PWM output to your custom motor driver MOSFET gates
    // e.g. analogWrite(MOTOR_PWM_PIN, (int)abs(output));
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
    run_control_loop();

    twai_message_t rxMsg;

    // Receive waiting CAN frame (Non-blocking: pdMS_TO_TICKS(0))
    if (twai_receive(&rxMsg, 0) == ESP_OK) {

        uint32_t rxId = rxMsg.identifier;
        uint8_t  rxLen = rxMsg.data_length_code;
        uint8_t* rxBuf = rxMsg.data;

        Serial.printf("Raw frame received — ID: 0x%08X  Len: %d\n", rxId, rxLen);

        switch (rxId) {
            case MSG_MOTOR_REQUEST:
                handle_request(rxBuf[0]);
                break;

            case MSG_MOTOR_TELEMETRY:
                send_motor_telemetry();
                break;

            case MSG_SETPOINT:
                handle_setpoint(rxBuf);
                break;

            case MSG_HOME:
                handle_home();
                break;

            case MSG_PID_SET:
                handle_pid_set(rxBuf);
                break;

            case MSG_PID_REQUEST:
                send_pid_status();
                break;

            default:
                break;
        }
    }

    // Bus health status check
    twai_status_info_t status_info;
    twai_get_status_info(&status_info);
    if (status_info.state == TWAI_STATE_BUS_OFF) {
        Serial.println("CAN Bus Off error! Recovering...");
        twai_initiate_recovery();
    }
}