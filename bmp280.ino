/***************************************************************************
  This is a library for the BMP280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// Replace with your network credentials
const char* ssid = "SSID"; // replace with your ssid
const char* password = "PASSWORD"; // replace with your key
const String hostname = "Home.ESP.BMP280.1"; // hostname for zabbix

WiFiServer AgentServer(10050);
AsyncWebServer server(80);

/*
SDO - MISO - D4 - 2
SDA - MOSI - D2 - 4
SCL - SCK - D1 - 5
CSB - CS - D3 - 0
 */

#define BMP_SCK  (5)
#define BMP_MISO (2)
#define BMP_MOSI (4)
#define BMP_CS   (0)


Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

float t = 0.0;
float p = 0.0;

unsigned long previousMillis = 0;
const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>%HOSTNAME%</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-arrow-circle-down" style="color:#00add6;"></i> 
    <span class="dht-labels">Pressure</span>
    <span id="pressure">%PRESSURE%</span>
    <sup class="units">Mm</sup>
  </p>
</body>
<script>
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
      document.getElementById("pressure").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pressure", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";


// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "PRESSURE"){
    return String(p);
  }
  else if(var == "HOSTNAME"){
    return hostname;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("BMP280 test"));

  //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(100);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(p).c_str());
  });

  // Start web-server
  server.begin();

  // Start agent-server
  AgentServer.begin();
  AgentServer.setNoDelay(true);
  
}

void zabbixAgentAnswer(WiFiClient *client, String data) {
  unsigned int tmp;
  
  // protocol + flags
  client->write((unsigned char *) "ZBXD\x01", 5);
  
  // packet size
  tmp = data.length();
  client->write((unsigned char *) &tmp, 4);
  
  // reserved
  tmp = 0;
  client->write((unsigned char *) &tmp, 4);
  
  // payload
  client->print(data);
}

void zabbixAgent() {
  char tmp[4];
  
  WiFiClient client = AgentServer.available();
  if (!client)
    return;
  
  Serial.println("New client! Waiting for data...");
  int timeout = 1000, i = 0;
  while (client.connected() && !client.available() && i < timeout) {
    delay(1);
    i++;
  }
  
  if (!client.connected() || !client.available()) {
    Serial.println("Client timeout...");
    client.stop();
    return;
  }
  
  // Protocol name
  if (client.read((unsigned char *) tmp, 4) != 4 || strncmp(tmp, "ZBXD", 4) != 0) {
    Serial.println("Invalid zabbix protocol name...");
    client.stop();
    return;
  }
  
  // Flags
  int flags = client.read();
  if (flags < 0) {
    Serial.println("Unexpected EOF when flags read...");
    client.stop();
    return;
  }
  
  // Packet size
  unsigned int packet_size;
  if (client.read((unsigned char *) &packet_size, 4) != 4) {
    Serial.println("Unexpected EOF when packet_size read...");
    client.stop();
    return;
  }
  
  // Reserved
  if (client.read((unsigned char *) tmp, 4) != 4) {
    Serial.println("Unexpected EOF when reserved read...");
    client.stop();
    return;
  }
  
  Serial.print("flags=");
  Serial.print(flags);
  Serial.print("\n");
  
  Serial.print("packet_size=");
  Serial.print(packet_size);
  Serial.print("\n");
  
  char command[64];
  
  if (packet_size >= sizeof(command)) {
    Serial.println("Packet size overflow...");
    client.stop();
    return;
  }
  
  if (client.read((unsigned char *) command, packet_size) != packet_size) {
    Serial.println("Unexpected EOF when command read...");
    client.stop();
    return;
  }
  
  command[packet_size] = '\0';
  
  Serial.print("command=");
  Serial.println(command);
  
  if (strcmp(command, "agent.ping") == 0) {
    Serial.println("->ping");
    zabbixAgentAnswer(&client, "1");
  } else if (strcmp(command, "agent.version") == 0) {
    Serial.println("->version");
    zabbixAgentAnswer(&client, "Esp.Agent/1.0");
  } else if (strcmp(command, "agent.uptime") == 0) {
    Serial.println("->uptime");
    zabbixAgentAnswer(&client, String(millis() / 1000).c_str());
  } else if (strcmp(command, "agent.hostname") == 0) {
    Serial.println("->hostname");
    zabbixAgentAnswer(&client, hostname);
  } else if (strcmp(command, "read.temperature") == 0) {
    Serial.println("->temperature");
    zabbixAgentAnswer(&client, String(t).c_str());
  } else if (strcmp(command, "read.pressure") == 0) {
    Serial.println("->pressure");
    zabbixAgentAnswer(&client, String(p).c_str());
  } else {
    Serial.println("->unknown command");
    zabbixAgentAnswer(&client, "ZBX_NOTSUPPORTED");
  }
  
  Serial.println("flush and close...");
  
  client.flush();
  client.stop();
}


void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you updated the values
      previousMillis = currentMillis;
      float newT = bmp.readTemperature();
      if (isnan(newT)) {
        Serial.println("Failed to read from BMP sensor!");
      }
      else {
        t = newT;
        Serial.println(t);
      }

      float newP = bmp.readPressure() / 133.33;
      if (isnan(newP)) {
        Serial.println("Failed to read from BMP sensor!");
      }
      else {
        p = newP;
        Serial.println(p);
      }
      Serial.println();
    } 

    /*
    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1013.25)); // Adjusted to local forecast! 
    Serial.println(" m");
    */
    zabbixAgent();
}
