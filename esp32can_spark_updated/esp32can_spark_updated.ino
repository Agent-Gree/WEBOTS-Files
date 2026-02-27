#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

// Define CS pin for MCP2515
#define CS_PIN 5

MCP2515 mcp2515(CS_PIN);

struct can_frame tx_frame;
struct can_frame rx_frame;

unsigned long previousMillis = 0;
const int interval = 20;
bool btPrintCanRx = 0;
bool btEnabled = 0;
bool btH_Enabled = 0;
bool CANBUS_Loss_Error = 0;

unsigned char ubDeviceType;
unsigned char ubManufacturer;
unsigned char ubAPIClass;
unsigned char ubAPIIndex;
unsigned char ubDeviceNumber;
unsigned char ubDeviceNumberToPrint = 99;

unsigned char ubDeviceConnected[20];
int iMotorCurrent;
float fMotorSpeed[20];
float fMotorCurrent[20];

unsigned char ubUSB_Rx_Buffer[10];
float fUSB_Rx_Speed;

union FloatIntUnion {
    float f;
    int i;
};
union FloatIntUnion fiu;

bool btIsWEB = 0;
char bMotorSpeed[6][4];

unsigned long ulCurrentMicros;
unsigned long ulPreviousMicros;
unsigned long ul5mS = 0;

void setup() {
  Serial.begin(921600);
  Serial.println("ESP32-CAN with MCP2515");
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);  // Change to MCP_16MHZ if needed
  mcp2515.setNormalMode();
  
  // Initialize motor speeds
  for(int MotorID = 0; MotorID < 6; MotorID++)
  {
    bMotorSpeed[MotorID][0] = 0;
    bMotorSpeed[MotorID][1] = 0;
    bMotorSpeed[MotorID][2] = 0;
    bMotorSpeed[MotorID][3] = 0;
  }
  
  Serial.println("MCP2515 CAN Initialized");
}

void loop() {
 
  ulCurrentMicros = micros();
  if((ulCurrentMicros - ulPreviousMicros) >= 500)
  {
    ulPreviousMicros = ulCurrentMicros;
    ul5mS = ul5mS + 5;
  }

  // Serial command processing
  while (Serial.available() > 0) 
  {
    char cIncomingByte = Serial.read();
    
    switch(cIncomingByte)
    {
     case 'S':
     case 's':   //emergency stop all
     {
       tx_frame.can_id = 0x00000000 | CAN_EFF_FLAG;
       tx_frame.can_dlc = 0;
       mcp2515.sendMessage(&tx_frame);
       break;
     }
     
     case 'h':
     case 'H':  //heart beat toggle
     {
       btH_Enabled ^= 1;
       Serial.println(btH_Enabled ? "Heart ON" : "Heart OFF");
       break;
     }
     
     case 'E':
     case 'e':    //enable can 
     {
       btEnabled = 1;
       btH_Enabled = 0;
       break;
     }
     
     case '1': ubDeviceNumberToPrint = 1; break;
     case '2': ubDeviceNumberToPrint = 2; break;
     case '3': ubDeviceNumberToPrint = 3; break;
     case '4': ubDeviceNumberToPrint = 4; break;
     case '5': ubDeviceNumberToPrint = 5; break;
     case '6': ubDeviceNumberToPrint = 6; break;
     
     case 'a':
     case 'A':
       ubDeviceNumberToPrint = 99;
       break;
     
     case 'P':
     case 'p':  //toggle printing 
     {
       btPrintCanRx ^= 1;
       Serial.println(btPrintCanRx ? "Print ON" : "Print OFF");
       break;
     }
     
     case 'D':
     case 'd':   //disable motors
     {
       btEnabled = 0;
       btH_Enabled = 1;
       break;
     } 
     
     case 'M':
     case 'm':   //motor drive command
     {
       btEnabled = 1;
       btH_Enabled = 0;
       btIsWEB = 0;
       cIncomingByte = Serial.read();
       
       if((cIncomingByte == 'L') || (cIncomingByte == 'l'))  //all left motors
       {
         cIncomingByte = Serial.read();
         if(cIncomingByte == ',')
         {
           for(int i = 0; i < 5; i++) {
             ubUSB_Rx_Buffer[i] = Serial.read();
           }
           ubUSB_Rx_Buffer[5] = 0;
           
           Serial.printf("USB RX: %s\n", ubUSB_Rx_Buffer);
           fUSB_Rx_Speed = atof((char*)ubUSB_Rx_Buffer);
           fiu.f = fUSB_Rx_Speed;
           
           for(int i = 0; i < 3; i++)
           {
             bMotorSpeed[i][0] = (fiu.i & 0x000000FF);
             bMotorSpeed[i][1] = (fiu.i & 0x0000FF00) >> 8;
             bMotorSpeed[i][2] = (fiu.i & 0x00FF0000) >> 16;
             bMotorSpeed[i][3] = (fiu.i & 0xFF000000) >> 24;
           }
         }
       }
       
       if((cIncomingByte == 'R') || (cIncomingByte == 'r'))  //all right motors
       {
         cIncomingByte = Serial.read();
         if(cIncomingByte == ',')
         {
           for(int i = 0; i < 5; i++) {
             ubUSB_Rx_Buffer[i] = Serial.read();
           }
           ubUSB_Rx_Buffer[5] = 0;
           
           Serial.printf("USB RX: %s\n", ubUSB_Rx_Buffer);
           fUSB_Rx_Speed = atof((char*)ubUSB_Rx_Buffer);
           fiu.f = fUSB_Rx_Speed;
           
           for(int i = 3; i < 6; i++)
           {
             bMotorSpeed[i][0] = (fiu.i & 0x000000FF);
             bMotorSpeed[i][1] = (fiu.i & 0x0000FF00) >> 8;
             bMotorSpeed[i][2] = (fiu.i & 0x00FF0000) >> 16;
             bMotorSpeed[i][3] = (fiu.i & 0xFF000000) >> 24;
           }
         }
       }
       break;
     } 
    }
  }

  // Receive CAN frame
  if (mcp2515.readMessage(&rx_frame) == MCP2515::ERROR_OK) 
  {
    if (rx_frame.can_id & CAN_EFF_FLAG)  // Extended frame
    {
      uint32_t id = rx_frame.can_id & 0x1FFFFFFF;
      
      ubDeviceType = (id >> 24) & 0x1F;
      ubManufacturer = (id >> 16) & 0xFF;
      ubAPIClass = (id >> 10) & 0x3F;
      ubAPIIndex = (id >> 6) & 0x0F;
      ubDeviceNumber = id & 0x3F;

      if(btPrintCanRx) 
      {
        if((ubDeviceNumber == ubDeviceNumberToPrint) || (ubDeviceNumberToPrint == 99))
        {
          Serial.printf("%lu\t0x%08X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\t0x%02X\tDLC %d\t",
                 micros(), id, ubDeviceType, ubManufacturer, 
                 ubAPIClass, ubAPIIndex, ubDeviceNumber, rx_frame.can_dlc);
          
          for (int i = 0; i < rx_frame.can_dlc; i++)
          {
            Serial.printf("0x%02X\t", rx_frame.data[i]);
          }
          Serial.printf("\n");
        }
      }
   
      if((ubDeviceType == 0x02) && (ubAPIClass == 0x06) && (ubAPIIndex == 0x01))
      {
        ubDeviceConnected[ubDeviceNumber-1] = ubDeviceNumber;
        
        fMotorSpeed[ubDeviceNumber-1] = (float)((rx_frame.data[3]<<24) | 
                                                (rx_frame.data[2]<<16) | 
                                                (rx_frame.data[1]<<8) | 
                                                (rx_frame.data[0]));
        
        iMotorCurrent = (rx_frame.data[6] << 8) | rx_frame.data[7];
        fMotorCurrent[ubDeviceNumber-1] = (iMotorCurrent / 4095.0) * 80;
      }
    }
  }

  // Send heartbeat every 20mS
  if((ul5mS % 20) == 0) 
  {
    if(btH_Enabled)
    {
      tx_frame.can_id = 0x000502C0 | CAN_EFF_FLAG;
      tx_frame.can_dlc = 1;
      tx_frame.data[0] = 1;
      mcp2515.sendMessage(&tx_frame);
    } 
  }

  // Send run heartbeat every 10mS
  if((ul5mS % 10) == 0) 
  {
    if(btEnabled)
    {
      tx_frame.can_id = 0x02052C80 | CAN_EFF_FLAG;
      tx_frame.can_dlc = 8;
      tx_frame.data[0] = 0x7E;
      tx_frame.data[1] = 0x00;
      tx_frame.data[2] = 0x00;
      tx_frame.data[3] = 0x00;
      tx_frame.data[4] = 0x00;
      tx_frame.data[5] = 0x00;
      tx_frame.data[6] = 0x80;
      tx_frame.data[7] = 0x00;
      mcp2515.sendMessage(&tx_frame);
    } 
  }

  // Send motor speeds every 30mS
  if((ul5mS % 30) == 0) 
  {
    if(btEnabled)
    {
      uint32_t motorIDs[6] = {0x02050081, 0x02050082, 0x02050083, 
                               0x02050084, 0x02050085, 0x02050086};
      
      for(int motor = 0; motor < 6; motor++)
      {
        tx_frame.can_id = motorIDs[motor] | CAN_EFF_FLAG;
        tx_frame.can_dlc = 8;
        tx_frame.data[0] = bMotorSpeed[motor][0];
        tx_frame.data[1] = bMotorSpeed[motor][1];
        tx_frame.data[2] = bMotorSpeed[motor][2];
        tx_frame.data[3] = bMotorSpeed[motor][3];
        tx_frame.data[4] = 0x00;
        tx_frame.data[5] = 0x00;
        tx_frame.data[6] = 0x00;
        tx_frame.data[7] = 0x00;
        
        mcp2515.sendMessage(&tx_frame);
        delayMicroseconds(100);
      }
    }
  }
}