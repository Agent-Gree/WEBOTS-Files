#include <SPI.h>
#include <mcp_can.h>

#define CS_PIN 5
#define INT_PIN 4

MCP_CAN CAN(CS_PIN);

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, CS_PIN);

  while (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ)) {
    Serial.println("CAN init failed, retrying...");
    delay(500);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(INT_PIN, INPUT);

  Serial.println("CAN init ok");
}

void loop() {
  if (!digitalRead(INT_PIN)) {
    long unsigned int rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    CAN.readMsgBuf(&rxId, &len, rxBuf);

    Serial.print("ID: 0x");
    Serial.print(rxId, HEX);
    Serial.print(" Data: ");

    for (int i = 0; i < len; i++) {
      Serial.print(rxBuf[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
}
