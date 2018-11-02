#include <WEMOS_DHT12.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include "FS.h"



DHT12 dht12;
WiFiClient espClient;
PubSubClient client(espClient);

String settings[7] = {"", "", "", "", "", "", ""};
char* host = "Mineduino-dht12";
boolean wifisetted = false;
boolean wificonnected = false;

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", "<html> \
 <head><style>label { display: inline-block; width: 160px; }</style> \
  </head> \
  <body> \
    <form action=\"set\"> \
      <p><label>Wi-Fi name: </label><input name='wifiname' value='" + settings[0] + "'></input></p> \
      <p><label>Wi-Fi password: </label><input name='wifipassword' value='" + settings[1] + "'></input></p> \
      <p><label>Mineduino user: </label><input name='mqttuser' value='" + settings[2] + "'></input></p> \
      <p><label>Mineduino password: </label><input name='mqttpassword' value='" + settings[3] + "'></input></p> \
      <p><label>Mineduino server ip: </label><input name='mineduinoserver' value='" + settings[4] + "'></input></p> \
      <p><label>Mineduino server port: </label><input name='mineduinoserverport' value='" + settings[5] + "'></input></p> \
      <p><label>Mineduino topic: </label><input name='mineduinoservertopic' value='" + settings[6] + "'></input></p> \
      <p><input type='submit'></input></p> \
    </form> \
  </body> \
</html>");
}

void(* resetFunc) (void) = 0;

void readConfig(int section) {
  String itosed = String(section);
  if(SPIFFS.exists(itosed)) {
    File f = SPIFFS.open(itosed, "r");
    String building = "";
    if(f && f.size()) {
      while(f.available()) {
        building += char(f.read());
      }
    }
    f.close();
    settings[section] = building;
    Serial.println(itosed + " " + building);
  }
}

void writeConfig(int section) {
  File f = SPIFFS.open(String(section), "w");
  if(f) {
    f.print(settings[section]);
    f.close();
  }
}

void handleSet() {
  if(server.hasArg("wifiname")) {
    settings[0] = server.arg("wifiname");
    writeConfig(0);
  }
  if(server.hasArg("wifipassword")) {
    settings[1] = server.arg("wifipassword");
    writeConfig(1);
  }
  if(server.hasArg("mqttuser")) {
    settings[2] = server.arg("mqttuser");
    writeConfig(2);
  }
  if(server.hasArg("mqttpassword")) {
    settings[3] = server.arg("mqttpassword");
    writeConfig(3);
  }
  if(server.hasArg("mineduinoserver")) {
    settings[4] = server.arg("mineduinoserver");
    writeConfig(4);
  }
  if(server.hasArg("mineduinoserverport")) {
    settings[5] = server.arg("mineduinoserverport");
    writeConfig(5);
  }
  if(server.hasArg("mineduinoservertopic")) {
    settings[6] = server.arg("mineduinoservertopic");
    writeConfig(6);
  }
  
  
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  bool result = SPIFFS.begin();
  if(result) {
    Serial.println("SPIFFS opened: TRUE");
  }
  if(SPIFFS.exists("0")) {
    wifisetted = true;
  }
  for(int j = 0; j < 7; j++) {
    readConfig(j);
  }
  
  pinMode(2, OUTPUT);
  pinMode(0, INPUT);
  WiFi.hostname(host);
  
  if(!wifisetted) {
    boolean result = WiFi.softAP(host, host);
    if(result) {
      Serial.println("WI-FI AP ready!");
    }
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings[0].c_str(), settings[1].c_str());
    
    Serial.print("Conneting to WiFi ");
    short delayed = 0;
    while (WiFi.status() != WL_CONNECTED && delayed <= 50) {
      digitalWrite(2, 1);
      delay(50);
      digitalWrite(2, 0);
      delay(450);
      Serial.print(".");
      delayed++;
    }
    if(delayed > 50) {
      wificonnected = false;
      Serial.println("Cannot connect to WI-FI");
      boolean result = WiFi.softAP(host, host);
      if(result) {
        Serial.println("WI-FI AP ready!");
      }
    } else {
      Serial.println("WI-FI connected!");
      wificonnected = true;
    }
    digitalWrite(2, 1);
  }

  client.setServer(settings[4].c_str(), settings[5].toInt());
  if(wificonnected) {
    while(!client.connected()) {
      client.connect(host);
    }
  }

  server.on("/", handleRoot);  
  server.on("/set", handleSet); 
  server.begin();
}

void reconnect() {
  while(!client.connected()) {
    if(!client.connect(host)) {
      delay(5000);
    }
  }
}

void loop() {
  server.handleClient();

  if(dht12.get()==0){
      client.publish(settings[6].c_str(), String("{\"temp\": " + String(dht12.cTemp) + ", \"humidity\": " + String(dht12.humidity) + "}" ).c_str());
  } else {
      client.publish(settings[6].c_str(), String("{\"temp\": -1.0, \"humidity\": -1.0 }" ).c_str());
  }
  digitalWrite(2, 0);
  delay(50);
  digitalWrite(2, 1);


  if(wificonnected) {
    if(!client.connected()) {
      reconnect();
    } 
  }
  delay(950);

}
