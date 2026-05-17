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


void loop() {
  // put your main code here, to run repeatedly:

}


void request_motor_data(byte requestID) {
    byte data[1] = {requestID};
    byte result = CAN.sendMsgBuf(MSG_MOTOR_REQUEST, 1, 1, data);

  if (result == CAN_OK) {
        Serial.printf("Request sent: 0x%02X\n", requestId);
    } else {
        Serial.println("Request send FAILED");
    }
}

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

void decode_motor_status(uns)

