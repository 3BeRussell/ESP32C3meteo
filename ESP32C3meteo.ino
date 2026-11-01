/* Simple Web-server on ESP32C3 for remote monitoring purpose */
/* 2026.11.01 User`s choice of sensor type (Thanks DeepSeek for idea of realisation) */
/* 2026.10.31 First git commit */
  
// https://arduino-tex.ru/news/18/urok-2-podklyuchaem-dht11-dht22-k-esp32-esp8266-pokazanie-na.html
// https://habr.com/ru/companies/otus/articles/894174/

/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/


/*
Me-Chiel
https://github.com/me-chiel/Wifimanager-Preferences
Updated this tutorial to eliminate dependency on SPIFFS (as it's cumbersome in Arduino IDE 2) and utilize the Preferences library instead.
This tutorial leverages a segment of the ESP32's built-in non-volatile memory (NVS) for data storage. 
This data persists even through system restarts and power loss events.

Additionally, a feature has been incorporated to allow resetting of this memory by holding down the BOOT button for 10 seconds
(or 3 seconds for a standard reset).

Important note:https://github.com/me-no-dev/ESPAsyncWebServer/issues/1434
*/    
           
//#include <Arduino.h>
//#include <WiFi.h>
//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

// Assign variables to GPIO pins
const int ONBOARD_LED   = 8;
const int ONBOARD_BTN   = 9;
const int SENSOR_PIN    = 4;

/* for debug via serial switch Tools/USB CDC On Boot: "Enable" */
const bool DEBUG        = false;  // Print debugging info via serial port

int buttonState;                  // Boot button state
unsigned long debounceDelay = 50; // debounce delay on push Boot button

// Important! Sensors assigned to the one pin!
//----------------------------- DS18B20 ------------------------------
#include <OneWire.h>               // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>     // https://github.com/milesburton/Arduino-Temperature-Control-Library
OneWire oneWire(SENSOR_PIN);
DallasTemperature ds(&oneWire);
//---------------------------- DS18B20 ------------------------------
//------------------------------- DHT --------------------------------
#include <DHT.h>                  // https://github.com/adafruit/DHT-sensor-library
DHT dht11(SENSOR_PIN, DHT11);
DHT dht21(SENSOR_PIN, DHT21);
DHT dht22(SENSOR_PIN, DHT22);
//------------------------------- DHT --------------------------------
                     
float temperature;
String sTemperature = "--";

//Variables to save values from HTML form
String ssid             = "";
String pass             = "";
String ip               = "";
String gateway          = "";
String sensor           = "";
String narod            = "";
Preferences preferences;

AsyncWebServer server(80); // Create AsyncWebServer object on port 80
// IP address WebServer`s Start page 192.168.4.1

IPAddress local_IP; // hardcoded localIP(192, 168, 0, 200); 
IPAddress local_gateway; // hardcoded gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 0, 0);

const char* host = "narodmon.ru";
const int httpPort = 8283;

String deviceName; //look like ESPAABBCCDDEEFF (ESP+mac).

unsigned long counter;
int publishingCounter = 303;

/*--------------------- server main page ---------------------------*/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<meta http-equiv="Content-type" content="text/html; charset=utf-8">
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Open+Sans:300,400,600">
  <link rel="shortcut icon" href="https://img.icons8.com/doodle/48/000000/partly-cloudy-day.png" type="image/png">
  <style>
    html {font-family: "Open Sans", sans-serif; display: inline-block; margin: 0px auto; text-align: center;}
    h2 {font-size: 3.0rem;}
    h3 {font-size: 2.5rem;}
    h4 {font-size: 2.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .units {font-size: 1.2rem;}
    .dht-labels{font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <p>
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";
/*--------------------- server main page ---------------------------*/

/*--------------------- replaces placeholder -----------------------*/
String processor(const String& var){
  if (var == "BUTTONPLACEHOLDER") {
    String buttons ="<h3>LED <span id=\"outputState\"></span></h3><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" ";
    if (!digitalRead(ONBOARD_LED)) buttons+="checked";
    buttons += "><span class=\"slider\"></span></label>";
    buttons += "<h4>";
    buttons += deviceName;
    buttons += "</h4>";
    return buttons;
  }
  else if(var == "TEMPERATURE") {
    return sTemperature;
  }
  return String();
}
/*--------------------- replaces placeholder -----------------------*/

/*--------------------- initial server page ------------------------*/
const char init_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP Wi-Fi Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {font-family: Arial, Helvetica, sans-serif;display: inline-block; text-align: center;}
h1 {font-size: 1.8rem; color: white;}
p { font-size: 1.4rem;}
.topnav { overflow: hidden;  background-color: #0A1128;}
body {    margin: 0;}
.content {   padding: 5%;}
.card-grid {   max-width: 800px;   margin: 0 auto;   display: grid;   grid-gap: 2rem;   grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));}
.card {   background-color: white;   box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}
.card-title {   font-size: 1.2rem;  font-weight: bold;  color: #034078}
input[type=submit] {  border: none;  color: #FEFCFB;  background-color: #034078;  padding: 15px 15px;  text-align: center;  text-decoration: none;  display: inline-block;  font-size: 16px;  width: 100px;  margin-right: 10px;  border-radius: 4px;  transition-duration: 0.4s;  }
input[type=submit]:hover {  background-color: #1282A2;}
input[type=text], input[type=number], select {  width: 50%;  padding: 12px 20px;  margin: 18px;  display: inline-block;  border: 1px solid #ccc;  border-radius: 4px;  box-sizing: border-box;}
label {  font-size: 1.2rem; }
.value{  font-size: 1.2rem;  color: #1282A2;  }
.state {  font-size: 1.2rem;  color: #1282A2;}
button {  border: none;  color: #FEFCFB;  padding: 15px 32px;  text-align: center;  font-size: 16px;  width: 100px;  border-radius: 4px;  transition-duration: 0.4s;}
.button-on {  background-color: #034078;}
.button-on:hover {  background-color: #1282A2;}
.button-off {  background-color: #858585;}
.button-off:hover {  background-color: #252524;} 
  </style>
</head>
<body>
  <div class="topnav">
    <h1>ESP WiFi Manager</h1>
  </div>
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <form action="/p">
          <p>
            <label for="ssid">SSID</label>
            <input type="text" id ="ssid" name="ssid"><br>
            <label for="pass">Password</label>
            <input type="text" id ="pass" name="pass"><br>
            <label for="ip">IP Address</label>
            <input type="text" id ="ip" name="ip" value="192.168.0.200"><br>
            <label for="gateway">Gateway Address</label>
            <input type="text" id ="gateway" name="gateway" value="192.168.0.1"><br>
            <br>
            <label for="sensor">Sensor</label>
            <select name="sensor" id="sensor">
              <option value="NONE">NONE</option>
              <option value="DS18B20">DS18B20</option>
              <option value="DHT11">DHT11</option>
              <option value="DHT21">DHT21(AM2301)</option>
              <option value="DHT22">DHT22(AM2302,AM2321)</option>
            </select><br>
            <br>
            <input type="checkbox" name="narod" value="on">narodmon.ru<br>
            <input type ="submit" value ="Submit">
          </p>
        </form>
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";
/*--------------------- initial server page ------------------------*/

/*--------------------- initialize WiFi ----------------------------*/
bool initWiFi() {
  if(ssid=="" || pass==""){
    if (DEBUG) Serial.println("Undefined SSID or IP address.");
    return false;
  }

  //WiFi.mode(WIFI_STA);
  local_IP.fromString(ip.c_str());
  local_gateway.fromString(gateway.c_str());
  if (!WiFi.config(local_IP, local_gateway, subnet)){
    if (DEBUG) Serial.println("STA Failed to configure");
    return false;
  }
  
  //connect to your local Wi-Fi network with SSID and password
  WiFi.begin(ssid.c_str(), pass.c_str());
  if (DEBUG) Serial.print("Connecting to ");
  if (DEBUG) Serial.println(ssid);
  int cnt = 10;
  //check wi-fi is connected to wi-fi network
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (DEBUG) Serial.print(".");
    cnt--;
    if (cnt < 0) {
      if (DEBUG) Serial.println("Failed to connect.");
      return false;
    }
  }
  if (DEBUG) Serial.println("WiFi connected..!");
  if (DEBUG) Serial.print("Got IP: ");
  if (DEBUG) Serial.println(WiFi.localIP());
  deviceName = "ESP" + WiFi.macAddress();
  deviceName.replace(":","");
  if (DEBUG) Serial.println(deviceName);
  return true;
}
/*--------------------- initialize WiFi ----------------------------*/

/*--------------------- setup --------------------------------------*/
void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  // read settings from NVS memory
  preferences.begin("memory", false);
  unsigned int counter = preferences.getUInt("counter", 0);  // Get the counter value, if the key does not exist, return a default value of 0
  counter++;                                                 // Increase Reset counter by 1
  if (DEBUG) Serial.printf("Current reboots: %u\n", counter);           // Print the counter to Serial Monitor
  preferences.putUInt("counter", counter);                   // Store the counter to the Preferences
  ssid    = preferences.getString("ssid","");                // get ssid, or return "" if empty
  if (DEBUG) Serial.print("ssid:"); Serial.println(ssid);
  pass    = preferences.getString("pass","");
  Serial.print("pass:"); Serial.println(pass);
  ip      = preferences.getString("ip","");
  if (DEBUG) Serial.print("ip:"); Serial.println(ip);
  gateway = preferences.getString("gateway","");
  if (DEBUG) Serial.print("gateway:"); Serial.println(gateway);
  sensor = preferences.getString("sensor","");
  if (DEBUG) Serial.print("sensor:"); Serial.println(sensor);
  narod = preferences.getString("narod","");
  if (DEBUG) Serial.print("narod:"); Serial.println(narod);
  preferences.end();                                         // close the Preferences


  // Initialize GPIO
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(ONBOARD_BTN, INPUT_PULLUP);
  pinMode(ONBOARD_LED, OUTPUT);                              
  digitalWrite(ONBOARD_LED, HIGH);                           // Set onboard led to off
  // onboard LED on ESP32C3 board lights when out is off (LOW)
  
  // Initialize sensor
//----------------------------- DS18B20 ------------------------------
  if (sensor ==  "DS18B20") {
    ds.begin();  
    ds.requestTemperatures(); 
  }
//----------------------------- DS18B20 ------------------------------  
//------------------------------- DHT --------------------------------
  else if (sensor == "DHT11") {
    dht11.begin();  
  }            
  else if (sensor == "DDHT21") {
    dht21.begin();  
  }            
  else if (sensor == "DHT22") {
    dht22.begin();  
  }            
//------------------------------- DHT --------------------------------

//------------------- setup if wifi initialized ----------------------
  if (initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
    });
    
    // Route to update temperature
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", sTemperature.c_str());
    });

    // Route to set LED state
    server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (request->hasParam("state")) {
        digitalWrite(ONBOARD_LED, !(request->getParam("state")->value().toInt())); //
      }
      request->send(200, "text/plain", "OK");
    });

    // Route to get LED state
    server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send(200, "text/plain", String(!digitalRead(ONBOARD_LED)).c_str());
    });

    server.begin();
    if (DEBUG) Serial.println("HTTP server started");
    counter = millis();
  }
//------------------- setup if wifi initialized ----------------------
  
//------------------- setup if wifi not initialized ------------------
  else {
    if (DEBUG) Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);    // NULL sets an open Access Point
    IPAddress IP = WiFi.softAPIP(); // IP address WebServer`s Start page 192.168.4.1
    if (DEBUG) Serial.print("AP IP address: ");
    if (DEBUG) Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", init_html);
      if (DEBUG) Serial.print("Setup page printed.");
    });
    
    // Web Server request
    server.on("/p", HTTP_GET, [](AsyncWebServerRequest *request) {
      int paramsNr = request->params();
      if (DEBUG) Serial.println("Get parameters");
      for(int i=0;i<paramsNr;i++){
        const AsyncWebParameter* p = request->getParam(i);
        // HTTP GET ssid value
        if (p->name() == "ssid") {
          ssid = p->value().c_str();
          if (DEBUG) Serial.print("SSID set to: ");
          if (DEBUG) Serial.println(ssid);
          preferences.begin("memory", false);
          preferences.putString("ssid", ssid);
          preferences.end();
        }
        // HTTP GET pass value
        if (p->name() == "pass") {
          pass = p->value().c_str();
          if (DEBUG) Serial.print("Password set to: ");
          if (DEBUG) Serial.println(pass);
          preferences.begin("memory", false);
          preferences.putString("pass", pass);
          preferences.end();
        }
        // HTTP GET ip value
        if (p->name() == "ip") {
          ip = p->value().c_str();
          if (DEBUG) Serial.print("IP Address set to: ");
          if (DEBUG) Serial.println(ip);
          preferences.begin("memory", false);
          preferences.putString("ip", ip);
          preferences.end();
        }
        // HTTP GET gateway value
        if (p->name() == "gateway") {
          gateway = p->value().c_str();
          if (DEBUG) Serial.print("Gateway set to: ");
          if (DEBUG) Serial.println(gateway);
          preferences.begin("memory", false);
          preferences.putString("gateway", gateway);
          preferences.end();
        }
        // HTTP GET sensor value
        if (p->name() == "sensor") {
          sensor = p->value().c_str();
          if (DEBUG) Serial.print("Sensor set to: ");
          if (DEBUG) Serial.println(sensor);
          preferences.begin("memory", false);
          preferences.putString("sensor", sensor);
          preferences.end();
        }
        // HTTP GET narod value
        if (p->name() == "narod") {
          narod = p->value().c_str();
          if (DEBUG) Serial.print("Narod set to: ");
          if (DEBUG) Serial.println(narod);
          preferences.begin("memory", false);
          preferences.putString("narod", narod);
          preferences.end();
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
//------------------- setup if wifi initialized ----------------------
}
/*--------------------- setup --------------------------------------*/

/*--------------------- main loop ----------------------------------*/
void loop() {
  if (millis() - counter > 1000) { // read temperature each 1 second
    counter = millis();
//------------------------------- DHT --------------------------------
    if (sensor == "DHT11") {
      temperature = dht11.readTemperature(); // get the value of the temperature
    }
    else if (sensor == "DHT21") {
      temperature = dht21.readTemperature(); // get the value of the temperature
    }
    else if (sensor == "DHT22") {
      temperature = dht22.readTemperature(); // get the value of the temperature
    }
//------------------------------- DHT --------------------------------
//----------------------------- DS18B20 ------------------------------
    else if (sensor == "DS18B20") {
      temperature = ds.getTempCByIndex(0);
      ds.requestTemperatures(); 
//----------------------------- DS18B20 ------------------------------
      if (isnan(temperature)) {    
        if (DEBUG) Serial.println("Failed to read from sensor!");
        sTemperature = "--";
      }
      else {
        //if (DEBUG) Serial.println(temperature);
        sTemperature = String(temperature);
      }
    }
    
    if (narod == "on") publishingCounter--;
  }
  
//--------------------- send to narodmon.ru --------------------------
  if (publishingCounter < 0) {
    publishingCounter = 303;
    String buf = "#" + deviceName + "\n";
    buf += "#T#" + sTemperature + "\n";
    buf += "##\n";
    
    WiFiClient client;

    unsigned long timeout = millis();
    while (!client.connect(host, httpPort)) {
      if (millis() - timeout > 5000) {
        if (DEBUG) Serial.print("Client timeout!");
        client.stop();
        break;
      } else {
        if (DEBUG) Serial.print("Connection to ");
        if (DEBUG) Serial.print(host);
        if (DEBUG) Serial.println(" successfully!");
        client.print(buf); // connected, send
        while (client.available()) {
          String line = client.readStringUntil('\r'); // recieve all answers and print to Serial
          if (DEBUG) Serial.println(line);
        }
        break;
      }
    }
  }
//--------------------- send to narodmon.ru --------------------------
  
//--------------------- get Boot button state ------------------------
  if (digitalRead(ONBOARD_BTN) != buttonState) { // 
    delay(debounceDelay); // 50
    if (digitalRead(ONBOARD_BTN) != buttonState) { //
      buttonState = digitalRead(ONBOARD_BTN);
      if (buttonState == LOW) {
        if (DEBUG) Serial.println("Boot button pressed"); 
        unsigned long  startTime = millis();
        while (digitalRead(ONBOARD_BTN) == LOW) { // still pressed
          delay(10);
        }
        if ((millis() - startTime) > 10000) { // If key pressed for more than 10secs, reset all settings
          if (DEBUG) Serial.println("Reset NVS");
          preferences.begin("memory", false); 
          preferences.clear();                
          if (DEBUG) Serial.println("NVS ereased!");     
          preferences.end(); 
          ESP.restart();                 
        }
        else {
          digitalWrite(ONBOARD_LED, !digitalRead(ONBOARD_LED));
        }
      }
    }
  }
//--------------------- get Boot button state ------------------------
}
/*--------------------- main loop ----------------------------------*/
