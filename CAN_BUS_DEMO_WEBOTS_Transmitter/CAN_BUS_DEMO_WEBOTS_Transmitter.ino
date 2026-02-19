#include <SPI.h>
#include <mcp_can.h>

#define CS_PIN 5
#define INT_PIN 4

MCP_CAN CAN(CS_PIN);

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, CS_PIN); // SCK, MISO, MOSI, SS

  while (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ)) {
    Serial.println("CAN init failed, retrying...");
    delay(500);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(INT_PIN, INPUT);

  Serial.println("CAN init ok");
}

void loop() {
  byte data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CAN.sendMsgBuf(0x100, 0, 8, data);
  Serial.println("Message sent");
  delay(1000);
}
