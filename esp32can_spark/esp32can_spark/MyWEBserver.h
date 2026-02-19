
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

bool btWS_Fellover = true;

// Replace with your network credentials
const char *ssid = "Rover";
const char *password = "12345678";  //must be 8 characters long


IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

extern bool btEnabled;
extern bool btH_Enabled;
extern char bMotorSpeed[6][4];

unsigned char ucButtonState;
unsigned char ucWorkingButtonState;


String buttonState = "0";


String DistanceMeas = "0";
  
String sliderValue = "0";
int iSliderId;


const char* PARAM_INPUT = "value";

union FloatIntUnion {
    float f;
    uint32_t i;
};


String CAN_message;

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 
<meta charset="utf-8">
<title>WE MARS Rover Wifi server</title>


<style>
:root {
  --main-bg-color: gray;
  
}
html {
  margin: 0;
  padding: 0;
  
}

body{ 
    width: 80%;
    margin: 0 auto;
    font: 100% Arial, Helvetica, sans-serif;
    padding: 1em 1em;
    background: white;
    
    }
.form input {
    position: absolute;
    left: -9999px;
}


.Setup{
  position:absolute;
  
  top:1%;
  left:10%;
  height:80%;
  width:80%;
  background:white;
}

 #Forward, #Left, #Right, #Reverse {
  position: absolute;
  top: 1%;
  height: 30%;
  width: 18%;
  text-align: center;
  padding: 10px 20px;
  background-color: blue;
  border: none;
  border-radius: 5px;
  cursor: pointer;
  color: white;
  font-size: 3vmax;
  font-weight: bold;
  transition: background-color 0.3s, color 0.3s, font-weight 0.3s;
  -webkit-user-select: none; /* For Safari */
  -moz-user-select: none; /* For Firefox */
  -ms-user-select: none; /* For Internet Explorer */
  user-select: none; /* For other browsers */
}
#Forward.pressed, #Left.pressed, #Right.pressed, #Reverse.pressed {
  background-color: red;
  color: white;
  font-weight: bold;
  -webkit-user-select: none; /* For Safari */
  -moz-user-select: none; /* For Firefox */
  -ms-user-select: none; /* For Internet Explorer */
  user-select: none; /* For other browsers */
}
#Forward { left: 10%; }
#Left { left: 31%; }
#Right { left: 52%; }
#Reverse { left: 73%; }

.Table1 {
  position:absolute;
  top:35%;
  left:10%;
  height:30%;
}

table, th, td {
  border: 1px solid black;
  border-collapse: collapse;
  width:90%;
  font: Arial, Helvetica, sans-serif;
  font-weight: bold;
  font-size: 1vmax;
  
  table-layout: fixed;
}
th, td {
  padding: 8px;
  text-align: left;
  
}
table#DP02 tr:nth-child(even) {
  background-color: #eee;
}

table#DP02 tr:nth-child(even) {
  background-color: #eee;
}
table#DP02 tr:nth-child(odd) {
 background-color: #fff;
}
table#DP02 th {
  background-color: grey;
  color: white;
}
table#DP01 th {
  background-color: grey;
  color: white;
}

.slider-container {
  position: absolute;
  top: 65%;
  width: 75%;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.slider-left, .slider-right {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.slider-wrapper {
  display: flex;
  align-items: center;
}

.slider-label {
  margin-right: 10px;
}

input[type=range]{
  width: 600px;
}

.value-display {
  margin-left: 10px;
}
</style>

<body>
 

<div class="form">
  <div>
    <button id="Forward" onmousedown="startSending(1)" onmouseup="stopSending(1)" onmouseleave="stopSending(1)" onselectstart="return false" ontouchstart="startSending(1)" ontouchend="stopSending(1)">FORWARD</button>
    <button id="Left"  onmousedown="startSending(2)" onmouseup="stopSending(2)" onmouseleave="stopSending(2)" onselectstart="return false" ontouchstart="startSending(2)" ontouchend="stopSending(2)">LEFT</button>
    <button id="Right" onmousedown="startSending(3)" onmouseup="stopSending(3)" onmouseleave="stopSending(3)"  onselectstart="return false" ontouchstart="startSending(3)" ontouchend="stopSending(3)">RIGHT</button>
    <button id="Reverse"  onmousedown="startSending(4)" onmouseup="stopSending(4)" onmouseleave="stopSending(4)" onselectstart="return false" ontouchstart="startSending(4)" ontouchend="stopSending(4)">REVERSE</button>

  </div>
 
</div>

<div class="slider-container">
  <div class="slider-left">
    <div class="slider-wrapper">
      <label class="slider-label" for="slider1">1</label>
      <input type="range" id="slider1" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value1" class="value-display">0</span>
    </div>
    <div class="slider-wrapper">
      <label class="slider-label" for="slider2">2</label>
      <input type="range" id="slider2" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value2" class="value-display">0</span>
    </div>
    <div class="slider-wrapper">
      <label class="slider-label" for="slider3">3</label>
      <input type="range" id="slider3" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value3" class="value-display">0</span>
    </div>
  </div>
  <div class="slider-right">
    <div class="slider-wrapper">
      <label class="slider-label" for="slider4">4</label>
      <input type="range" id="slider4" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value4" class="value-display">0</span>
    </div>
    <div class="slider-wrapper">
      <label class="slider-label" for="slider5">5</label>
      <input type="range" id="slider5" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value5" class="value-display">0</span>
    </div>
    <div class="slider-wrapper">
      <label class="slider-label" for="slider6">6</label>
      <input type="range" id="slider6" onchange="updateSlider(this)" min="0" max="1" step="0.1" value="0">
      <span id="value6" class="value-display">0</span>
    </div>
  </div>
</div>
<label class = "Table1">
 
     <table id="DP02">
      <tr>
        <th>CAN ID</th> 
        <th>Data</th> 
        <th>Data</th> 
        <th>CAN ID</th> 
        <th>Data</th> 
        <th>Data</th> 
        <th>CAN ID</th> 
        <th>Data</th> 
        <th>Data</th> 
               
      </tr>
      <tr>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
       
        
        
      </tr>
      <tr>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
       
      </tr>
      <tr>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
       
      </tr>
      <tr>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
       
      </tr>
      <tr>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"> Empty</td>
        <td name = CANID style= "word-wrap:break-word;"></td>
        <td name = CANID style= "word-wrap:break-word;"></td>

      </tr>
    
    </table>  

<script>
  

 var Flipped;
 var CBID = document.getElementsByName("CANID")
 //var CBD = document.getElementsByName("CANData");
 var CANCommandIndex = 0;
 var CANVariableIndex = 21;
 var CANColumHaltedAt = 6;
 var CANIndexer = 0; 
 var vCANData;
 var ChartLowerLimits = [0,0,0,0,0,0];
 var ChartUpperLimits = [0,0,0,0,0,0];
 var ChartWatchIndex = [9,9,9,9,9,9];
 var ChartVariableIndex;


function sendData(ButtonPressed) {
  var xhttp = new XMLHttpRequest();
  
  if(ButtonPressed == 5)
  {
    if(lineColour == "red")
    {
      lineColour = "green";
      lineColourChanged = 1;
    }
    else
    {
      lineColour = "red";
      lineColourChanged = 1;
    }
  }
 
  
  xhttp.open("GET", "setPressedButton?StateButton="+ButtonPressed, true);
  xhttp.send();
}

function fetchMessage() {
    fetch('/get-message')
        .then(response => response.text())
        .then(data => {
             vCANData = (data).split(";");
             getCANID();
           
        })
        .catch(error => console.error('Error:', error));
}

setInterval(fetchMessage, 250); // Fetch message every second

function updateSlider(slider) {
  var sliderId = slider.id.replace('slider', '');
  document.getElementById('value' + sliderId).textContent = slider.value;
  sendSliderData(sliderId, slider.value);
}

function sendSliderData(sliderId, value) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      console.log(this.responseText);
    }
  };
  xhttp.open("GET", "slider?slider=" + sliderId + "&value=" + value, true);
  xhttp.send();
}

function getCANID() 
{
 
  var vCAN_ID;
 
 
     vCAN_ID = vCANData;
     console.log(vCAN_ID);
     if(vCAN_ID.length > 1)
     {
       for (CANIndexer=0;CANIndexer<vCAN_ID.length;CANIndexer+=3)  
       {
         if(vCAN_ID[CANIndexer] != "END")
         {

          CBID[CANIndexer].innerHTML = vCAN_ID[CANIndexer];
          CBID[CANIndexer+1].innerHTML = vCAN_ID[CANIndexer+1];
          CBID[CANIndexer+2].innerHTML = vCAN_ID[CANIndexer+2];
         }
        

       }
     }
 }

let timerIds = {};

function sendRequest(buttonId) {
  fetch(`/button${buttonId}`)
    .then(response => response.text())
    .then(data => console.log(data))
    .catch(error => console.error('Error:', error));
}

function startSending(buttonId) {
  sendRequest(buttonId);
  timerIds[buttonId] = setInterval(() => sendRequest(buttonId), 50);
  document.getElementById(getButtonId(buttonId)).classList.add('pressed');
}

function stopSending(buttonId) {
  clearInterval(timerIds[buttonId]);
  document.getElementById(getButtonId(buttonId)).classList.remove('pressed');
}

function getButtonId(buttonId) {
  switch (buttonId) {
    case 1: return 'Forward';
    case 2: return 'Left';
    case 3: return 'Right';
    case 4: return 'Reverse';
    default: return '';
  }
}
</script>
</body>
</html>


)=====";

void handleButtonPress(int buttonId) {
  //Serial.printf("Button %d pressed!\n", buttonId);
  ucButtonState = buttonId;
}



// uint32_t convertToLittleEndian(const char* floatStr) {
//     // Convert string to float
//     float value = strtof(floatStr, NULL);
    
//     // Use a union to reinterpret the float as a 32-bit integer
//     union FloatIntUnion fiu;
//     fiu.f = value;
    
//     // Check the endianness of the system
//     uint32_t test = 1;
//     char* testPtr = (char*)&test;
    
//     if (testPtr[0] == 1) {
//         // System is already little-endian
//         Serial.println("here");
//         return fiu.i;
        
//     } else {
//         // System is big-endian, convert to little-endian
//         uint32_t result = 0;
//         result |= (fiu.i & 0x000000FF) << 24;
//         result |= (fiu.i & 0x0000FF00) << 8;
//         result |= (fiu.i & 0x00FF0000) >> 8;
//         result |= (fiu.i & 0xFF000000) >> 24;
//         Serial.println("There");
//         return result;
//     }
// }



void setupWEbServer(void)
{

  
  
  Serial.print(F("Configuring access point..."));

	
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
  
  
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", MAIN_page);
  });
    
   server.on("/button1", HTTP_GET, [](AsyncWebServerRequest *request){
    handleButtonPress(1);
    request->send(200, "text/plain", "Button 1 pressed");
  });

  server.on("/button2", HTTP_GET, [](AsyncWebServerRequest *request){
    handleButtonPress(2);
    request->send(200, "text/plain", "Button 2 pressed");
  });

  server.on("/button3", HTTP_GET, [](AsyncWebServerRequest *request){
    handleButtonPress(3);
    request->send(200, "text/plain", "Button 3 pressed");
  });

  server.on("/button4", HTTP_GET, [](AsyncWebServerRequest *request){
    handleButtonPress(4);
    request->send(200, "text/plain", "Button 4 pressed");
  });

  // server.on("/setPressedButton", HTTP_GET, [](AsyncWebServerRequest *request)
  // {
      
  //     AsyncWebParameter* p;
     
  //    if(request->hasParam("StateButton"))
  //      p = request->getParam("StateButton");
  //      // Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());

  //    String t_state = p->value().c_str();//server->arg("StateButton"); //Refer  xhttp.open("GET", "setButton?StateButton="+buttonPressed, true);
  //    buttonState = p->value().c_str();
     
  //    ucButtonState = buttonState.toInt();
    
     
  //   request->send(200, "text/plain", buttonState); //Send web page
   
  // });


 //server.on("/slider", []() {
  //  String slider = server.arg("slider");
 //   String value = server.arg("value");
//    Serial.println("Slider " + slider + " value: " + value);
    // Handle slider change logic here
 //   server.send(200, "text/plain", "OK");
 // });




   // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
         
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      iSliderId = (request->getParam("slider")->value().toInt()) - 1;
      sliderValue = request->getParam(PARAM_INPUT)->value();
      float value = strtof(sliderValue.c_str(), NULL);
      union FloatIntUnion fiu;
      fiu.f = value;
      bMotorSpeed[iSliderId][0] = (fiu.i & 0x000000FF);
      bMotorSpeed[iSliderId][1] = (fiu.i & 0x0000FF00) >> 8;
      bMotorSpeed[iSliderId][2] = (fiu.i & 0x00FF0000) >> 16;
      bMotorSpeed[iSliderId][3] = (fiu.i & 0xFF000000) >> 24;
      //printf("0x%02x 0x%02x 0x%02x 0x%02x\n", bMotorSpeed[3],bMotorSpeed[2],bMotorSpeed[1],bMotorSpeed[0]);
    }
    else {
      sliderValue = "No message sent";
    }
    Serial.printf("Slide ID: %i, sliderValue: %s\n",iSliderId, sliderValue);
    request->send(200, "text/plain", "OK");
  }); 
    
  // Handle GET request for message
    server.on("/get-message", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", CAN_message);
        
    });

  server.begin();
  Serial.println(F("HTTP server started"));


  Serial.println(F(""));
}

 


void loopWEBServerButtonresponce(void)
{

   
    switch(ucButtonState)
    {
      case 0:
      default:
      {
     
      Serial.println("Stop");
       ucWorkingButtonState = 0;
       ucButtonState = 9;
       btH_Enabled = 1;
       btEnabled = 0;
       
        break;
      }
      case 1:
      {
      
       Serial.println("Forward");
        ucButtonState = 9;
        ucWorkingButtonState = 1;
       
        break;
      }
      case 2:
      {
        
       Serial.println("Left");
        ucButtonState = 9;
        ucWorkingButtonState = 2;
       
        break;
      }
      case 3:
      {
      
       Serial.println("Right");
       ucButtonState = 9;
       ucWorkingButtonState = 3;
       
        break;
      }
      case 4:
      {
       
    
        Serial.println("Reverse");
        ucButtonState = 9;
        ucWorkingButtonState = 4;
       
        break;
      }
      case 9:
      {
        
        ucButtonState = 9;
        break;
      }
    }

}


