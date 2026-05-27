// =============================================================================
// can_protocol.h
// Shared CAN protocol definition — include this in BOTH ESP32 projects.
// Any change to message structure must be updated here and reflashed to both.
// =============================================================================

#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

// -----------------------------------------------------------------------------
// CAN Bus Configuration
// Both devices must use the same speed and crystal frequency.
// -----------------------------------------------------------------------------
#define CAN_SPEED           CAN_500KBPS
#define CAN_CRYSTAL         MCP_8MHZ        // Check your MCP2515 module (8 or 16MHz)
#define CAN_CS_PIN          5               // SPI chip select pin

// -----------------------------------------------------------------------------
// Message IDs
// -----------------------------------------------------------------------------
#define MSG_MOTOR_REQUEST   0x18FF0010           // Master → Motor Controller
#define MSG_MOTOR_STATUS    0x18FF0101           // Motor Controller → Master
#define MSG_MOTOR_TELEMETRY 0x102   // Motor → Master: voltage, position
#define MSG_SETPOINT        0x105   // Master → Motor: target value + mode
#define MSG_HOME            0x106   // Master → Motor: reset position

// Format: 0x18FF [MessageType] [DeviceID]
#define DEVICE_MOTOR_1      0x01
#define DEVICE_MASTER       0x10

// -----------------------------------------------------------------------------
// Request IDs (sent in byte 0 of MSG_MOTOR_REQUEST)
// The master sends one of these to ask for a specific value or all values.
// -----------------------------------------------------------------------------
#define REQ_CURRENT         0x01
#define REQ_RPM             0x02
#define REQ_TEMPERATURE     0x03
#define REQ_ALL             0xFF            // Request full MotorStatus frame

// -----------------------------------------------------------------------------
// MSG_MOTOR_STATUS byte layout (8 bytes total)
//
//  Byte 0-1 : Current      int16   scale 0.01    unit: A
//  Byte 2-3 : RPM          int16   scale 1       unit: RPM
//  Byte 4-5 : Temperature  uint16  scale 0.1     unit: degC
//  Byte 6   : Flags        uint8   see flag bits below
//  Byte 7   : Reserved
// -----------------------------------------------------------------------------
#define BYTE_CURRENT_HIGH   0
#define BYTE_CURRENT_LOW    1
#define BYTE_RPM_HIGH       2
#define BYTE_RPM_LOW        3
#define BYTE_TEMP_HIGH      4
#define BYTE_TEMP_LOW       5
#define BYTE_FLAGS          6
#define BYTE_RESERVED       7


// MSG_MOTOR_TELEMETRY layout (8 bytes):
//  Byte 0-1 : Voltage   uint16  scale 0.01   unit: V
//  Byte 2-5 : Position  int32   scale 1      unit: encoder counts
//  Byte 6-7 : Reserved
#define BYTE_VOLTAGE_HIGH   0
#define BYTE_VOLTAGE_LOW    1
#define BYTE_POSITION_3     2   // most significant byte
#define BYTE_POSITION_2     3
#define BYTE_POSITION_1     4
#define BYTE_POSITION_0     5   // least significant byte


// MSG_SETPOINT layout (6 bytes):
//  Byte 0   : Mode
//  Byte 1-4 : Value  int32
//  Byte 5   : Reserved
#define BYTE_SETPOINT_MODE  0
#define BYTE_SETPOINT_VAL   1   // int32 starts here

// Setpoint modes
#define SETPOINT_RPM        0x01
#define SETPOINT_POSITION   0x02


// Scaling
#define SCALE_VOLTAGE       0.01f
#define SCALE_POSITION      1       // raw encoder counts, no scaling needed



// Flag bits in BYTE_FLAGS
#define FLAG_ENABLED        (1 << 0)        // Bit 0: motor is enabled
#define FLAG_FAULT          (1 << 1)        // Bit 1: fault condition active

// -----------------------------------------------------------------------------
// Scaling factors
// raw → physical:  physical = raw * SCALE
// physical → raw:  raw = (int)(physical / SCALE)  or  (int)(physical * (1/SCALE))
// -----------------------------------------------------------------------------
#define SCALE_CURRENT       0.01f           // 1 unit = 0.01 A
#define SCALE_TEMPERATURE   0.1f            // 1 unit = 0.1 degC

// -----------------------------------------------------------------------------
// Encode/decode helper macros
// -----------------------------------------------------------------------------

// Pack a signed 16-bit value into two bytes (big-endian, high byte first)
#define PACK_INT16(buf, idx, val) \
    (buf)[(idx)]     = ((int16_t)(val) >> 8) & 0xFF; \
    (buf)[(idx) + 1] = ((int16_t)(val))      & 0xFF;

// Pack an unsigned 16-bit value into two bytes (big-endian)
#define PACK_UINT16(buf, idx, val) \
    (buf)[(idx)]     = ((uint16_t)(val) >> 8) & 0xFF; \
    (buf)[(idx) + 1] = ((uint16_t)(val))       & 0xFF;

// Unpack a signed 16-bit value from two bytes (big-endian)
#define UNPACK_INT16(buf, idx) \
    ((int16_t)(((uint16_t)(buf)[(idx)] << 8) | (buf)[(idx) + 1]))

// Unpack an unsigned 16-bit value from two bytes (big-endian)
#define UNPACK_UINT16(buf, idx) \
    ((uint16_t)(((uint16_t)(buf)[(idx)] << 8) | (buf)[(idx) + 1]))

#define PACK_INT32(buf, idx, val) \
    (buf)[(idx)]     = ((int32_t)(val) >> 24) & 0xFF; \
    (buf)[(idx) + 1] = ((int32_t)(val) >> 16) & 0xFF; \
    (buf)[(idx) + 2] = ((int32_t)(val) >> 8)  & 0xFF; \
    (buf)[(idx) + 3] = ((int32_t)(val))        & 0xFF;

#define UNPACK_INT32(buf, idx) \
    ((int32_t)( ((uint32_t)(buf)[(idx)]     << 24) \
              | ((uint32_t)(buf)[(idx) + 1] << 16) \
              | ((uint32_t)(buf)[(idx) + 2] << 8)  \
              |  (uint32_t)(buf)[(idx) + 3]))

#endif // CAN_PROTOCOL_H

// -----------------------------------------------------------------------------
// PID Controller Stuff
// -----------------------------------------------------------------------------
// New message IDs
#define MSG_PID_SET         0x102   // Master → Motor: write gains
#define MSG_PID_REQUEST     0x103   // Master → Motor: ask for gains
#define MSG_PID_STATUS      0x104   // Motor → Master: current gains

// MSG_PID_SET and MSG_PID_STATUS share the same layout (8 bytes):
//
//  Byte 0-1 : Kp   int16   scale 0.001
//  Byte 2-3 : Ki   int16   scale 0.001
//  Byte 4-5 : Kd   int16   scale 0.001
//  Byte 6   : Reserved
//  Byte 7   : Reserved
#define BYTE_KP_HIGH        0
#define BYTE_KP_LOW         1
#define BYTE_KI_HIGH        2
#define BYTE_KI_LOW         3
#define BYTE_KD_HIGH        4
#define BYTE_KD_LOW         5

#define SCALE_PID_GAIN      0.001f  // 3 decimal places, range ±32.767

