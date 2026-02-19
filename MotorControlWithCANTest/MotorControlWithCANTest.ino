#include <SPI.h>
#include <mcp2515.h>

// SPI pins for MCP2515
#define CS_PIN 5

MCP2515 mcp2515(CS_PIN);

// SPARK MAX configuration
#define SPARK_MAX_CAN_ID 1
#define SPARK_API_SETPOINT 0x02050080

void setup() {
    Serial.begin(115200);
    
    // Initialize MCP2515
    mcp2515.reset();
    mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);  // Adjust MCP_8MHZ based on your crystal
    mcp2515.setNormalMode();
    
    Serial.println("MCP2515 initialized");
}

void setSparkMaxDutyCycle(uint8_t deviceID, float dutyCycle) {
    dutyCycle = constrain(dutyCycle, -1.0, 1.0);
    int16_t value = (int16_t)(dutyCycle * 32767.0);
    
    struct can_frame frame;
    frame.can_id = SPARK_API_SETPOINT | deviceID;
    frame.can_id |= CAN_EFF_FLAG;  // Extended frame
    frame.can_dlc = 6;
    
    frame.data[0] = value & 0xFF;
    frame.data[1] = (value >> 8) & 0xFF;
    frame.data[2] = 0x00;
    frame.data[3] = 0x00;
    frame.data[4] = 0x00;
    frame.data[5] = 0x00;
    
    mcp2515.sendMessage(&frame);
    Serial.printf("Sent: %.2f\n", dutyCycle);
}

void loop() {
    static float speed = 0.0;
    static float increment = 0.05;
    
    setSparkMaxDutyCycle(SPARK_MAX_CAN_ID, speed);
    
    speed += increment;
    if (speed >= 0.5 || speed <= -0.5) {
        increment = -increment;
    }
    
    delay(50);
}