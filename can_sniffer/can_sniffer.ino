#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

CanFrame rx_frame;

// Statistics
unsigned long messageCount = 0;
unsigned long lastStatsTime = 0;
unsigned long messagesPerSecond = 0;
unsigned long tempMessageCount = 0;

// Filter settings
bool filterEnabled = false;
uint32_t filterID = 0;
bool showTimestamp = true;
bool showRaw = false;
bool showDecoded = true;

void printHelp() {
  Serial.println("\n=== CAN Bus Sniffer ===");
  Serial.println("Commands:");
  Serial.println("  h - Show this help");
  Serial.println("  c - Clear screen");
  Serial.println("  s - Toggle statistics");
  Serial.println("  t - Toggle timestamp");
  Serial.println("  r - Toggle raw hex display");
  Serial.println("  d - Toggle decoded display");
  Serial.println("  f [ID] - Filter by CAN ID (hex, e.g., 'f 02050081')");
  Serial.println("  u - Unfilter (show all messages)");
  Serial.println("  p - Pause/Resume");
  Serial.println("=======================\n");
}

void printCANMessage(CanFrame &frame) {
  // Timestamp
  if(showTimestamp) {
    Serial.printf("[%010lu] ", millis());
  }
  
  // CAN ID
  Serial.printf("ID: 0x%08X ", frame.identifier);
  
  // Frame type
  if(frame.extd) {
    Serial.print("EXT ");
  } else {
    Serial.print("STD ");
  }
  
  // RTR flag
  if(frame.rtr) {
    Serial.print("RTR ");
  }
  
  // Data length
  Serial.printf("DLC: %d ", frame.data_length_code);
  
  // Data bytes (raw hex)
  if(showRaw) {
    Serial.print("Data: ");
    for(int i = 0; i < frame.data_length_code; i++) {
      Serial.printf("%02X ", frame.data[i]);
    }
  }
  
  // Decoded fields (if extended frame)
  if(showDecoded && frame.extd) {
    uint8_t deviceType = (frame.identifier >> 24) & 0x1F;
    uint8_t manufacturer = (frame.identifier >> 16) & 0xFF;
    uint8_t apiClass = (frame.identifier >> 10) & 0x3F;
    uint8_t apiIndex = (frame.identifier >> 6) & 0x0F;
    uint8_t deviceNumber = frame.identifier & 0x3F;
    
    Serial.printf("| Dev:%02X Mfg:%02X API:%02X/%02X #%d", 
                  deviceType, manufacturer, apiClass, apiIndex, deviceNumber);
  }
  
  Serial.println();
  messageCount++;
  tempMessageCount++;
}

void printStatistics() {
  unsigned long now = millis();
  if(now - lastStatsTime >= 1000) {
    messagesPerSecond = tempMessageCount;
    tempMessageCount = 0;
    lastStatsTime = now;
    
    Serial.printf("\n--- Stats: %lu msg/s | Total: %lu ---\n", 
                  messagesPerSecond, messageCount);
  }
}

void setup() {
  Serial.begin(921600);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 CAN Bus Sniffer ===");
  Serial.println("Initializing CAN bus...");
  
  // Initialize CAN at 1 Mbps, RX=GPIO22, TX=GPIO21
  if(ESP32Can.begin(ESP32Can.convertSpeed(1000), GPIO_NUM_22, GPIO_NUM_21, 10, 10)) {
    Serial.println("CAN bus initialized successfully!");
    Serial.printf("Speed: 1000 kbps\n");
    Serial.printf("RX Pin: GPIO 22\n");
    Serial.printf("TX Pin: GPIO 21\n");
  } else {
    Serial.println("ERROR: CAN bus initialization failed!");
  }
  
  printHelp();
}

bool paused = false;

void loop() {
  
  // Handle serial commands
  if(Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if(cmd == "h") {
      printHelp();
    }
    else if(cmd == "c") {
      Serial.print("\033[2J\033[H"); // Clear screen (ANSI)
      messageCount = 0;
      tempMessageCount = 0;
    }
    else if(cmd == "t") {
      showTimestamp = !showTimestamp;
      Serial.printf("Timestamp: %s\n", showTimestamp ? "ON" : "OFF");
    }
    else if(cmd == "r") {
      showRaw = !showRaw;
      Serial.printf("Raw hex: %s\n", showRaw ? "ON" : "OFF");
    }
    else if(cmd == "d") {
      showDecoded = !showDecoded;
      Serial.printf("Decoded: %s\n", showDecoded ? "ON" : "OFF");
    }
    else if(cmd == "p") {
      paused = !paused;
      Serial.printf("Sniffer: %s\n", paused ? "PAUSED" : "RUNNING");
    }
    else if(cmd == "u") {
      filterEnabled = false;
      Serial.println("Filter disabled - showing all messages");
    }
    else if(cmd.startsWith("f ")) {
      // Parse hex ID
      String idStr = cmd.substring(2);
      idStr.trim();
      filterID = strtoul(idStr.c_str(), NULL, 16);
      filterEnabled = true;
      Serial.printf("Filter enabled for ID: 0x%08X\n", filterID);
    }
    else if(cmd == "s") {
      printStatistics();
    }
  }
  
  // Read CAN messages
  if(!paused && ESP32Can.readFrame(rx_frame, 1)) {
    // Apply filter if enabled
    if(filterEnabled) {
      if(rx_frame.identifier == filterID) {
        printCANMessage(rx_frame);
      }
    } else {
      printCANMessage(rx_frame);
    }
  }
  
  // Auto print statistics every second
  static unsigned long lastStatsPrint = 0;
  if(millis() - lastStatsPrint >= 1000) {
    messagesPerSecond = tempMessageCount;
    tempMessageCount = 0;
    lastStatsPrint = millis();
  }
}
/*
```

## **Features:**

### **1. Real-time CAN monitoring**
- Displays all CAN messages with timestamps
- Shows message count and rate

### **2. Display modes:**
- **Timestamp** - Shows when message was received
- **Raw hex** - Shows data bytes in hex
- **Decoded** - Breaks down extended CAN ID fields

### **3. Interactive commands:**

Type in Serial Monitor:
- `h` - Help menu
- `c` - Clear screen
- `p` - Pause/Resume
- `t` - Toggle timestamps
- `r` - Toggle raw hex data
- `d` - Toggle decoded fields
- `f 02050081` - Filter to show only messages with ID 0x02050081
- `u` - Remove filter (show all)
- `s` - Show statistics

### **4. Statistics:**
- Messages per second
- Total message count

---

## **Example Output:**
```
[0000012345] ID: 0x02050081 EXT DLC: 8 Data: 00 00 00 00 00 00 00 00 | Dev:02 Mfg:05 API:00/00 #1
[0000012367] ID: 0x02050082 EXT DLC: 8 Data: 00 00 00 00 00 00 00 00 | Dev:02 Mfg:05 API:00/00 #2
[0000012389] ID: 0x000502C0 EXT DLC: 1 Data: 01 | Dev:00 Mfg:05 API:00/0B #0

--- Stats: 145 msg/s | Total: 1523 ---*/