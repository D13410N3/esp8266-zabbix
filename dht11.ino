#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <TM1637Display.h>

// Replace with your network credentials
const char* ssid = "SSID"; // replace with your ssid
const char* password = "PASSWORD"; // replace with your key
const String hostname = "Home.ESP.DHT11.1"; // hostname for zabbix

WiFiServer AgentServer(10050);

#define DHTPIN 5     // Digital pin connected to the DHT sensor - for me it was D1

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11

#define DISPLAY_CLK 6 // it was display, but it doesn't work for me :(
#define DISPLAY_DIO 7

DHT dht(DHTPIN, DHTTYPE);
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
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
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
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
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
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
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "HOSTNAME"){
    return hostname;
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  
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
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
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
  } else if (strcmp(command, "read.humidity") == 0) {
    Serial.println("->humidity");
    zabbixAgentAnswer(&client, String(h).c_str());
  } else {
    Serial.println("->unknown command");
    zabbixAgentAnswer(&client, "ZBX_NOTSUPPORTED");
  }
  
  Serial.println("flush and close...");
  
  client.flush();
  client.stop();
}

void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }
  
  zabbixAgent();
}
