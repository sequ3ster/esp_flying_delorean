/*********
  Wifi Control for Flying Delorean from Did3d. (https://www.did3d.fr)
  You can buy the 3D Model here: https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model
  or here: https://cults3d.com/en/3d-model/art/delorean-volante-v2-hq-1-8-scale-530mm-print-3d

  Benjamin Heimann
  Complete project details at https://github.com/sequ3ster/esp_flying_delorean
  https://www.facebook.com/groups/599517350568861
*********/

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <PubSubClient.h>
#include <ElegantOTA.h>
#include "animation.h" 

#ifdef ESP32
  #include <SPIFFS.h>
#endif

// Set web server port number to 80
const unsigned int port = 80;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_user[40];
char mqtt_password[40];
char mqtt_ha_topic[40] = "homeassistant";

// --------------------------------------------------------------
// Please do not change anything from here on
// --------------------------------------------------------------
Adafruit_IS31FL3731_Wing matrix = Adafruit_IS31FL3731_Wing();

ESP8266WebServer server(port);

//flag for saving data
bool shouldSaveConfig = false;

uint8_t MatrixBitmap=1;
uint16_t ServoValue = 0; 
uint16_t LastServoValue = 0; 
int16_t powerstateState = 0;

bool MatrixConnected=false;
bool pushbuttonState = HIGH;
bool poweronoffState = LOW;
bool StatusIndicatorState = HIGH; 
bool DeloreanIsFlying = false; 

unsigned long previousTime = 0; 
unsigned long previousBitmapTime = 0; 
unsigned long TimeOutButton = 0; 
unsigned long startTimeButton = 0; 
unsigned long LastTimeFlying = 0; 
unsigned long timeoutTimeButton = 0;

const char swversion[15] = "0.8 (beta)";

const uint8_t PulseInPin = 13; // ESP8266 PulseIn = D7 --- 180k ohm
const uint8_t pushbutton = 12; //ESP8266 GPIO12 = D6 --- 470-1k ohm
const uint8_t poweronoff = 14; //ESP8266 GPIO14 = D5 --- Mosfet IRL510
const uint8_t powerstate = A0; //ESP8266 ADC0 = A0 --- 180k ohm
const uint8_t StatusIndicator = 2; //ESP8266 GPIO2 = D4
const uint8_t eqModOnly = 3; // ESP8266 GPIO3 = RX --- to Ground if only the EQ mod is required

// --- MQTT Device ---
String mac = WiFi.macAddress(); // Eindeutige Basis für IDs
String deviceId;
String shortcmndTopic;
String longcmndTopic;
String mqttDeviceConfigTopic;
String deviceConfigTopic;
String ipStateTopic;
String motionStateTopic;
String servoStateTopic;
String powerStateTopic;
String powerCommandTopic;
int8_t wifiRssi;
int8_t freeRam;
IPAddress ipAddress;
#define MSG_BUFFER_SIZE	(2432)

WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastMsg = 0;
unsigned long lastTryConnect = 0;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;        
}

void SaveConfig () {
  //save the custom parameters to FS
    Serial.println("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_ha_topic"] = mqtt_ha_topic;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("\033[1;31mfailed to open config file for writing\033[0m");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
}

void setupMosfet() {   
  // IRF9540 datasheet https://www.vishay.com/docs/91078/91078.pdf
  poweronoffState = LOW;    
  pinMode(poweronoff, OUTPUT);
  digitalWrite(poweronoff, poweronoffState);  
}

void setupReadConfig() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_ha_topic, json["mqtt_ha_topic"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          Serial.println("Password: "+String(json["password"]));
        } else {
          Serial.println("\033[1;31mfailed to load json config\033[0m");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("\033[1;31mfailed to mount FS\033[0m");
  }
  //end read
}

void setupWifi() {  
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);      
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);  
  WiFiManagerParameter custom_mqtt_ha_topic("topic", "HA Discovery topic", mqtt_ha_topic, 40);    

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setCustomHeadElement("<style>form[action='/info'],form[action='/exit']:nth-of-type(2){display:none}</style>");
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);   
  wifiManager.addParameter(&custom_mqtt_ha_topic);   

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Flying Delorean")) {
    Serial.println("\033[1;31mfailed to connect and hit timeout\033[0m");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());  
  strcpy(mqtt_ha_topic, custom_mqtt_ha_topic.getValue());  

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    SaveConfig();
  }
}

void setupWebServer() {  
  // Print local IP address and start web server   
  server.on("/", handle_root);
  server.on("/btn", HTTP_POST, handle_btn);
  server.on("/saveconfig", HTTP_POST, handle_savecfg);
  server.on("/ota", handle_ota);
  server.on("/config", handle_cfg);
  server.on("/apmode", handle_ap);
  server.on("/restart", handle_restart);  
  server.on("/hadiscoveryon", handle_publishHaDiscovery);
  server.on("/hadiscoveryoff", handle_unpublishHaDiscovery);  
  server.on("/powerevents", handle_powerSSE);
  server.on("/longevents", handle_longSSE);
  server.on("/shortevents", handle_shortSSE);
  server.onNotFound(handleWebRequests);
  
  ElegantOTA.begin(&server);

  server.begin();
  Serial.println("\033[1;32mHTTP Server started\033[0m");
}

void setupMqtt() {
  // Init MQTT Client
  mac = WiFi.macAddress(); 
  mac.replace(":", "");
  deviceId = "delorean_" + mac.substring(mac.length() - 4);  
  
  mqttDeviceConfigTopic = String(mqtt_ha_topic) + "/device/" + deviceId + "/config";
  ipStateTopic = "stat/" + deviceId + "/ip";
  motionStateTopic = "stat/" + deviceId + "/motion";
  servoStateTopic = "stat/" + deviceId + "/servo";
  shortcmndTopic = "cmnd/" + deviceId + "/short";
  longcmndTopic = "cmnd/" + deviceId + "/long";
  powerStateTopic = "stat/" + deviceId + "/switch";
  powerCommandTopic = "cmnd/" + deviceId + "/switch";
  
  mqttClient.setServer(mqtt_server, String(mqtt_port).toInt());
  mqttClient.setCallback(mqttCallback);  
  mqttClient.setBufferSize(MSG_BUFFER_SIZE);
}

void setup() {
  Serial.begin(74800);

  setupMosfet();

  pinMode(pushbutton, OUTPUT);
  pinMode(StatusIndicator, OUTPUT);
  pinMode(PulseInPin, INPUT);
  pinMode(powerstate, INPUT);
  pushbuttonState = HIGH;  
  powerstateState = 0;
  StatusIndicatorState = HIGH;
  digitalWrite(pushbutton, pushbuttonState);  
  digitalWrite(StatusIndicator, StatusIndicatorState);

  pinMode(eqModOnly, INPUT_PULLUP);
  if (digitalRead(eqModOnly) == 1) {
    setupReadConfig();
    setupWifi();
    setupMqtt();
    setupWebServer();
  }

  // Check if is something connected to SDA (D4)
  pinMode(D4, OUTPUT);
  digitalWrite(D4, LOW);
  pinMode(D4, INPUT);
  if(!digitalRead(D4)) {
    return;
  }
  
  Serial.println("Connecting Adafruit IS31 CharlieWing");  
  if(! matrix.begin()) {
    Serial.println("\033[1;31mIS31 not found\033[0m");
    return;  
  } else {
    MatrixConnected = true;
    Serial.println("\033[1;32mIS31 Connected\033[0m");
  } 
}

void handle_root() {  
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", sendHTMLHead());
  server.sendContent(sendHTMLBody());    
  server.sendContent("");
}

void handle_ota() {  
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", sendHTMLHead());
  server.sendContent(sendHTMLota());    
  server.sendContent("");
}

void PushButton() {
  if ((startTimeButton == 0) && (timeoutTimeButton > 0)) {      
    startTimeButton = millis();
    pushbuttonState = LOW;    
    digitalWrite(pushbutton, pushbuttonState);    
  }  else if (millis() > (startTimeButton + timeoutTimeButton)) {     
    pushbuttonState = HIGH;    
    timeoutTimeButton = 0;
    startTimeButton = 0;
    digitalWrite(pushbutton, pushbuttonState);    
  }
}

uint8_t evaluate_btn_state(String btnState) {
  uint8_t state;
  String lowbtnState = btnState;
  lowbtnState.toLowerCase();
  if (lowbtnState == "off") {
    state = HIGH;
  } else if (lowbtnState == "on") {
    state = LOW;
  } else {
    Serial.println("unknow btn state: "+btnState);
  }  
  return state;
}

void handle_btn() {
  String btnPin = server.arg("btnPin");
  String btnState = server.arg("btnState");
  String powerButton = server.arg("powerbtnState");
  String httpResponse;

  if (!DeloreanIsFlying) {
    if (btnPin == "1") {
      Serial.println("power on/off");    
      if (!DeloreanIsFlying) {
        poweronoffState = evaluate_btn_state(powerButton);
        digitalWrite(poweronoff, poweronoffState);
        httpResponse = "power " ;
        httpResponse += poweronoffState ? "on" : "off";
      }
      publishPowerState();
    } else if (btnPin == "2") {      
      timeoutTimeButton = btnState.toInt();
      Serial.println("press "+String(timeoutTimeButton)+" ms"); 
      PushButton();
      httpResponse = btnState;
    } else {
      Serial.println("btn unknow: "+btnPin);
    }
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  if (DeloreanIsFlying) {
    server.send(409, "text/plain", "Conflict: Delorean is flying");
    server.sendContent("");
  } else {
    server.send(200, "text/plain", httpResponse);
    server.sendContent("");
  }  
}

void handle_savecfg() {
  char mqtthatopic[40];
  strcpy(mqtthatopic, server.arg("mqtthatopic").c_str());  

  if (mqtt_ha_topic != mqtthatopic) {
    unpublishHADiscoveryConfig();
    strcpy(mqtt_ha_topic, mqtthatopic);    
  }
  
  strcpy(mqtt_server, server.arg("mqttserver").c_str());
  strcpy(mqtt_port, server.arg("mqttport").c_str());
  strcpy(mqtt_user, server.arg("mqttuser").c_str());
    
  if (server.arg("mqttpass") != "") {
    strcpy(mqtt_password, server.arg("mqttpass").c_str());
  }

  if (server.arg("mqtthatopic") != "") mqttHaDiscoveryConfig();

  SaveConfig();

  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", ""); 

  delay(100);
  ESP.restart();
  delay(5000);
}

void handle_ap() {
  server.sendHeader("Location", "http://192.168.4.1/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Start AP Mode");  

  WiFiManager wifiManager;
  wifiManager.resetSettings();  
  delay(200);
  ESP.restart();
  delay(5000);
}

void handle_restart() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Restart");    
  delay(100);
  ESP.restart();
  delay(5000);
}

void handle_cfg() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", sendHTMLHead());
  server.sendContent(sendConfigHTMLBody());    
  server.sendContent("");  
}

void handle_NotFound() {
  server.send(404, "text/html", sendPageUnknown());
}

void handle_publishHaDiscovery() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("MQTT Home Assistant Discovery Config");

  mqttHaDiscoveryConfig();        
}

void handle_unpublishHaDiscovery() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("MQTT Home Assistatnt Reset Discovery Config");

  unpublishHADiscoveryConfig();        
}

void handle_powerSSE() {
  WiFiClient client = server.client();
  
  if (client) {    
    serverSentEventHeader(client);    
    serverSentPowerEvent(client);      

    // give the web browser time to receive the data
    delay(10); // round about 100 messages per second            
    
    // close the connection:
    client.stop();
  }
}

void handle_longSSE() {
  WiFiClient client = server.client();
  
  if (client) {      
    serverSentEventHeader(client);    
    serverSentLongEvent(client);      

    // give the web browser time to receive the data
    delay(10); // round about 100 messages per second            
    
    // close the connection:
    client.stop();
  }
}

void handle_shortSSE() {
  WiFiClient client = server.client();
  
  if (client) {    
    serverSentEventHeader(client);    
    serverSentShortEvent(client);      

    // give the web browser time to receive the data
    delay(10); // round about 100 messages per second            
    
    // close the connection:
    client.stop();
  }
}

void serverSentEventHeader(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/event-stream");
  client.println("Cache-Control: no-cache");  // refresh the page automatically every 5 sec
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();
}

void serverSentPowerEvent(WiFiClient client) {
  String Output = "data: " + String(poweronoffState ? "ON" : "OFF");
  client.println(Output);
  client.println();
}

void serverSentLongEvent(WiFiClient client) {  
  String Output;
  if ((timeoutTimeButton > 3000) && (!pushbuttonState)){
    Output = "data: ON";
  } else {
    Output = "data: OFF";
  }
  client.println(Output);
  client.println();
}

void serverSentShortEvent(WiFiClient client) {
  String Output;
  if ((timeoutTimeButton < 3000) && (!pushbuttonState)){
    Output = "data: ON";
  } else {
    Output = "data: OFF";
  }
  client.println(Output);
  client.println();
}

String sendPageUnknown() {
    String ptr = "<!DOCTYPE html> <html>\n";
  
  ptr += "<head>";
  ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Page unknown</title>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>Page unknown</h1>\n";
  ptr += "<a href=\"/\">Return to main page</a>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void handleWebRequests(){
  if(loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain"; 
  if(path.endsWith("/")) path += "index.htm";
 
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".webp")) dataType = "image/webp";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
  
  dataFile.close();
  return true;
}

String build_btn_form(String btnPin, uint8_t btnState) {
  String ptr = "<form action=\"btn\" method=\"post\" id=\"powerForm\">\n";
  String btnStateStr = String(btnState ? "ON" : "OFF");  
  String lowbtnStateStr = btnStateStr;  
  lowbtnStateStr.toLowerCase();

  ptr += "  <input type=\"hidden\" name=\"btnPin\" value=\""+btnPin+"\">\n";
  ptr += "  <input type=\"hidden\" name=\"powerbtnState\" id=\"powerbtnState\" value=\""+btnStateStr+"\">\n";
  ptr += "  <input type=\"submit\" id=\"powerButton\" class=\"button button-" + lowbtnStateStr + "\" value=\"" + btnStateStr + "\">\n";

  ptr += "</form>\n";
  ptr += "<script type=\"text/javascript\">"; 
  ptr += "  const powerButton = document.getElementById('powerButton');";    
  ptr += "  document.addEventListener('DOMContentLoaded', function() {";
  ptr += "    const powerForm = document.getElementById('powerForm');"; 
  ptr += "    powerForm.addEventListener('submit', function(event) {";
  ptr += "      event.preventDefault();"; 
  ptr += "      const formData = new FormData(powerForm);"; 
  ptr += "      fetch('/btn', {"; 
  ptr += "        method: 'POST',"; 
  ptr += "        body: formData,"; 
  ptr += "      })";
  ptr += "      .then(response => {";
  ptr += "        if (!response.ok) {";
  ptr += "          throw new Error(`HTTP error! status: ${response.status}`);";
  ptr += "        }";
  ptr += "        return response.text();"; 
  ptr += "      })";
  ptr += "      .then(data => {";
  ptr += "        console.log('Success:', data);";
  ptr += "        if (powerButton.value == 'OFF') { powerButton.style.color = 'white'; powerButton.className = \"button button-on\"; powerbtnState.value = 'ON'; powerButton.value = 'ON'; }";
  ptr += "        else { powerButton.style.color = 'black'; powerButton.className = \"button button-off\"; powerbtnState.value = 'OFF'; powerButton.value = 'OFF'; }";  
  ptr += "      })";
  ptr += "      .catch(error => {";
  ptr += "        console.error('Error on seding formular:', error);";
  ptr += "        powerButton.style.color = 'black'; powerButton.className = \"button button-off\"; powerbtnState.value = 'OFF'; powerButton.value = 'OFF';";
  ptr += "      });";
  ptr += "    });";
  ptr += "  });";
  ptr += "  const powerEventSource = new EventSource('/powerevents');";  
  ptr += "  powerEventSource.onmessage = function(event) { ";
  ptr += "    const powerbtnState = document.getElementById('powerbtnState');";
  ptr += "    powerButton.value = event.data;";  
  ptr += "    if (event.data == 'ON') { powerButton.style.color = 'white'; powerButton.className = \"button button-on\"; powerbtnState.value = event.data;}";
  ptr += "    else { powerButton.style.color = 'black'; powerButton.className = \"button button-off\"; powerbtnState.value = event.data; }";  
  ptr += "  }";    
  ptr += "</script>";
  return ptr;
}

String build_push_btn_form(String btnStr, String functionStr, int btnDelay) {
  String ptr = "<form action=\"btn\" method=\"post\" id=\"" + functionStr + "Form\">\n";
  ptr += "  <input type=\"hidden\" name=\"btnPin\" value=\"2\">\n";
  ptr += "  <input type=\"hidden\" name=\"btnState\" value=\""+String(btnDelay)+"\">\n";
  ptr += "  <input type=\"submit\" id=\"" + functionStr + "Button\" class=\"button button-" + functionStr + "\" value=\"";
  ptr += btnStr+"\">\n";
  ptr += "</form>\n";
  ptr += "<script type=\"text/javascript\">";
  ptr += "  const " + functionStr + "Button = document.getElementById('" + functionStr + "Button');";
  ptr += "  document.addEventListener('DOMContentLoaded', function() {";
  ptr += "    const " + functionStr + "Form = document.getElementById('" + functionStr + "Form');"; 
  ptr += "    " + functionStr + "Form.addEventListener('submit', function(event) {";
  ptr += "      event.preventDefault();"; 
  ptr += "      const formData = new FormData(" + functionStr + "Form);"; 
  ptr += "      fetch('/btn', {"; 
  ptr += "        method: 'POST',"; 
  ptr += "        body: formData,"; 
  ptr += "      })";
  ptr += "      .then(response => {";
  ptr += "        if (!response.ok) {";
  ptr += "          console.error('Error ${response.status}:', response.text());";  
  ptr += "          throw new Error(`HTTP error! status: ${response.status}`);";
  ptr += "        }";
  ptr += "        return response.text();";
  ptr += "      })";
  ptr += "      .then(data => {";
  ptr += "        console.log('Success:', data);";
  ptr += "        " + functionStr + "Button.style.color = 'white';";
  ptr += "        " + functionStr + "Button.className = \"button button-" + functionStr + "-on\";";
  ptr += "        setTimeout(Set" + functionStr + "ButtonOff, " + String(btnDelay) + ")";
  ptr += "      })";
  ptr += "      .catch(error => {";
  ptr += "        console.error('Fehler beim Senden des Formulars:', error);";
  ptr += "      });";
  ptr += "    });";
  ptr += "  });";  
  ptr += "  function Set" + functionStr +"ButtonOff() {" + functionStr + "Button.style.color = 'black'; " + functionStr + "Button.className = \"button button-" + functionStr + "-off\"; }";
  ptr += "</script>";
  return ptr;
}

String sendHTMLHead() {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head>";
  ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Flying Delorean Did3D</title>\n";  
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 25px; background-color: black; background-image: url('/BTTF2.webp'); background-repeat: no-repeat; background-position: center 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;} form {margin-bottom: 30px;} img{max-width:300px;}\n";  
  ptr += ".button {display: block;width: 150px;background-color: #deea26;border: none;text-decoration: none;font-size: 25px;margin: auto;cursor: pointer;border-radius: 4px;}\n";  
  ptr += ".did3d {background-color: #black; position: fixed;bottom: 15px;right: 15px;}\n";  
  ptr += ".swversion {background-color: #black; position: fixed;bottom: 10px;left: 10px; color: gray;}\n";  
  ptr += ".config {background-color: #black; color: lightgray;}\n";  
  ptr += ".configtable {margin-left: auto;margin-right: auto; text-align: left;}";
  ptr += ".readonly {background-color: gray; color: lightgray;}";

  /* off-screen-menu */
  ptr += ".off-screen-menu {background-color: #696969; opacity: 0.85; height: 420px; width: 210px;position: fixed;top: 20px;right: -450px;display: flex;flex-direction: column;align-items: center;justify-content: center;text-align: center;font-size: 1.5rem;transition: .3s ease;}\n";
  ptr += ".off-screen-menu.active {right: 0;}\n";

  /* nav */
  ptr += "nav {padding: 1rem;display: flex;background-color: transparent;}\n";

  /* ham menu */
  ptr += ".ham-menu {height: 40px;width: 40px;margin-left: auto;position: relative;}\n";
  ptr += ".ham-menu span {height: 5px;width: 100%;background-color: #696969;border-radius: 25px;position: absolute;left: 50%;top: 50%;transform: translate(-50%, -50%);transition: .3s ease;}\n";
  ptr += ".ham-menu span:nth-child(1) {top: 25%;}\n";
  ptr += ".ham-menu span:nth-child(3) {top: 75%;}\n";
  ptr += ".ham-menu.active span {background-color: white;}\n";
  ptr += ".ham-menu.active span:nth-child(1) {top: 50%;transform: translate(-50%, -50%) rotate(45deg);}\n";
  ptr += ".ham-menu.active span:nth-child(2) {opacity: 0;}\n";
  ptr += ".ham-menu.active span:nth-child(3) {top: 50%;transform: translate(-50%, -50%) rotate(-45deg);}\n";

  /* button menu*/
  ptr += ".button-menu {background-color: #606060;color: white; text-align: left;}\n";
  ptr += ".button-menu:active {background-color: #565656;color: white; text-align: left;}\n";
  ptr += ".button-config {background-color: #606060;color: white;}\n";
  ptr += ".button-config:active {background-color: #565656;color: white;}\n";
  ptr += ".button-ap {background-color: #606060;color: white;}\n";
  ptr += ".button-ap:active {background-color: #565656;color: white;}\n";
  
  /* button power on/off */
  ptr += ".button-off {background-color: #7F151A;color: black;}\n";
  ptr += ".button-off:active {background-color: #7F151A;color: black;}\n";
  ptr += ".button-on {background-color: #cb2129;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-on:active {background-color: #A01B21;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";

  /* push button */
  ptr += ".button-short {background-color: #70f74f;color: black;}\n";
  ptr += ".button-short:active {background-color: #56bc3c;color: black;}\n";
  ptr += ".button-short-off {background-color: #70f74f;color: black;}\n";
  ptr += ".button-short-off:active {background-color: #70f74f;color: black;}\n";
  ptr += ".button-short-on {background-color: #56bc3c;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-short-on:active {background-color: #60ce42;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-long {background-color: #dfc048;color: black;}\n";
  ptr += ".button-long:active {background-color: #b59a3b;color: black;}\n";
  ptr += ".button-long-off {background-color: #dfc048;color: black;}\n";
  ptr += ".button-long-off:active {background-color: #dfc048;color: black;}\n";
  ptr += ".button-long-on {background-color: #b59a3b;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-long-on:active {background-color: #ccac43;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";

  /* button reset */
  ptr += ".button-reset {background-color: black;color: #7f94a1;position: absolute;bottom: 125px;left: 25px;font-size: 12px}\n";
  ptr += ".button-reset:active {background-color: #black;color: blue;position: absolute;bottom: 125px;left: 25px;font-size: 12px}\n";

  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  return ptr;
}

String sendConfigHTMLBody() {
  String ptr = "<body>\n";    
  ptr += sendHTMLMenu();   
  ptr += sendHTMLFooter();
  ptr += "<p><br><br><br><br><br><br></p>\n";  
  ptr += "<form action=\"saveconfig\" method=\"post\" class=\"config\">\n";
  ptr += "  <table class=\"configtable\"><tr>";
  ptr += "  <td><label for=\"wifissid\">Wifi SSID</label></td><td><input id=\"wifissid\" class=\"readonly\" value=\"" + String(WiFi.SSID()) + "\" readonly></td>\n";     
  ptr += "  </tr><tr>";    
  ptr += "  <td></td><td><br></td>"; 
  ptr += "  </tr><tr>";  
  ptr += "  <td></td><td><input type=\"button\" class=\"button button-ap\" value=\"AP mode\" id=\"startApMode\"></td>\n";  
  ptr += "  </tr><tr>";  
  ptr += "  <td></td><td><br></td>";  
  ptr += "  </tr><tr>";  
  ptr += "  <td style=\"color: white;\"><b>MQTT-Brocker</b></td><td></td>";  
  ptr += "  </tr><tr>";  
  ptr += "  <td><label for=\"mqttdevice\">Device</label></td><td><input id=\"mqttdevice\" class=\"readonly\" value=\"" + deviceId + "\" readonly></td>\n";  
  ptr += "  </tr><tr>";
  ptr += "  <td><label for=\"mqttconnected\">Connected</label></td><td><input id=\"mqttconnected\" class=\"readonly\" value=\"" + String(mqttClient.connected() ? "true" : "false") + "\" readonly></td>\n";  
  ptr += "  </tr><tr>";
  ptr += "  <td><label for=\"mqttserver\">Server</label></td><td><input name=\"mqttserver\" value=\"" + String(mqtt_server) + "\"></td>\n";  
  ptr += "  </tr><tr>";
  ptr += "  <td><lable for=\"mqttport\">Port</lable></td><td><input name=\"mqttport\" value=\"" + String(mqtt_port) + "\"></td>\n";    
  ptr += "  </tr><tr>";
  ptr += "  <td><lable for=\"mqttuser\">User</lable></td><td><input name=\"mqttuser\" value=\"" + String(mqtt_user) + "\"></td>\n";  
  ptr += "  </tr><tr>";
  ptr += "  <td><lable for=\"mqttpass\">Password</label></td><td><input name=\"mqttpass\" type=\"password\" value=\"\"></td>\n";      
  ptr += "  </tr><tr>";
  ptr += "  <td></td><td><br></td>";  
  ptr += "  </tr><tr>";  
  ptr += "  <td style=\"color: white;\"><b>Home Assistant</b></td><td></td>";    
  ptr += "  </tr><tr>";
  ptr += "  <td><lable for=\"mqtthatopic\">Discovery Topic</label></td><td><input name=\"mqtthatopic\" value=\"" + String(mqtt_ha_topic) + "\"></td>\n";  
  ptr += "  </tr><tr>";
  ptr += "  <td></td><td><br></td>";  
  ptr += "  </tr><tr>";  
  ptr += "  <td><input type=\"button\" class=\"button button-config\" value=\"Restart\" onclick=\"location.href='/restart';\"></td>";
  ptr += "  <td><input type=\"submit\" class=\"button button-config\" value=\"Save\"></td>\n";  
  ptr += "  </tr></table>";
  ptr += "</form>\n";    

  ptr += "<script type=\"text/javascript\">\n";  
  ptr += "const apMode = document.querySelector(\".button-ap\");\n";
  ptr += "apMode.addEventListener(\"click\", () => {";
  ptr += "if(confirm('This activates the Wifi access point. You must then connect to the \"Flying Delorean\" AP and go to http://192.168.4.1. You can then adjust the Wifi settings. Do you want to activate the AP mode now?'))";
  ptr += "window.location.href=\"/apmode\";})\n";  
  ptr += "</script>\n";

  ptr += "</body>\n";
  ptr += "</html>\n";    
  return ptr;  
}

String sendHTMLMenu() {  
  String ptr = "<div class=\"off-screen-menu\">\n";    
  ptr += "  <form>\n";
  ptr += "    <input type=\"button\" class=\"button button-menu\" value=\"Home\" onclick=\"location.href='/';\">\n";  
  ptr += "    <br>\n";  
  ptr += "    <input type=\"submit\" class=\"button button-menu\" value=\"Did3D.fr\" onclick=\"window.open('https://www.did3d.fr', '_blank');\">\n";  
  ptr += "    <br>\n";  
  ptr += "    <input type=\"submit\" class=\"button button-menu\" value=\"Facebook\" onclick=\"window.open('https://www.facebook.com/groups/599517350568861', '_blank');\">\n";  
  ptr += "    <br>\n";
  ptr += "    <input type=\"submit\" class=\"button button-menu\" value=\"Github\" onclick=\"window.open('https://github.com/sequ3ster/esp_flying_delorean\', '_blank');\">\n";  
  ptr += "    <br>\n";  
  ptr += "    <input type=\"button\" class=\"button button-menu\" value=\"Update\" onclick=\"location.href='/ota';\">\n";    
  ptr += "    <br>\n";    
  ptr += "    <input type=\"button\" class=\"button button-menu\" value=\"Config\" onclick=\"location.href='/config';\">\n";  
  ptr += "  </form>\n";
  ptr += "</div>\n";

  ptr += "  <nav>\n";
  ptr += "    <div class=\"ham-menu\">\n";
  ptr += "      <span></span>\n";
  ptr += "      <span></span>\n";
  ptr += "      <span></span>\n";
  ptr += "    </div>\n";
  ptr += "  </nav>\n";

  ptr += "<script type=\"text/javascript\">\n";  
  ptr += "const hamMenu = document.querySelector(\".ham-menu\");\n";
  ptr += "const offScreenMenu = document.querySelector(\".off-screen-menu\");\n";
  ptr += "hamMenu.addEventListener(\"click\", () => {\n";
  ptr += "  hamMenu.classList.toggle(\"active\");\n";
  ptr += "  offScreenMenu.classList.toggle(\"active\");\n";
  ptr += "});\n";  
  ptr += "</script>\n";  
  return ptr;
}

String sendHTMLFooter() {  
  String ptr = "<p><img src=\"/Did3d.webp\" alt=\"Did3D.fr\" class=\"did3d\" width=\"32\" height=\"32\"></p>\n";
  ptr += "<div class=\"swversion\">v. " + String(swversion) + "</div>";
  return ptr;
}

String sendHTMLBody() {  
  String ptr = "<body>\n";  
  ptr += sendHTMLMenu();
  ptr += sendHTMLFooter();
  ptr += "<br>\n";
  ptr += "<br>\n";
  ptr += "<br>\n";
  ptr += "<br>\n";
  ptr += "<br>\n";
  ptr += "<br>\n";
  ptr += build_btn_form("1", poweronoffState); 
  ptr += build_push_btn_form("Scene", "short", 1000);
  ptr += build_push_btn_form("Mode", "long", 3100);     
  
  ptr += "</body>\n";
  ptr += "</html>\n";  
  return ptr;
}

String sendHTMLota() {  
  String ptr = "<body>\n";  
  ptr += sendHTMLMenu();
  ptr += sendHTMLFooter();
  ptr += "<br>\n";
  ptr += "<br>\n";  
  ptr += "<h1>Firmware Update</h1>\n";
  ptr += "<p>You can download the latest version for the D1 mini here.</p>\n";
  ptr += "<input type=\"button\" class=\"button button-config\" value=\"Download\" onclick=\"location.href='https://github.com/sequ3ster/esp_flying_delorean/releases/latest/download/esp8266_D1_Mini_EspFlyingDelorean.bin';\">\n";
  ptr += "<br><p>After a successful update, please press Restart.</p>\n";
  ptr += "<iframe src=\"/update\" height=\"120\" width=\"350\" title=\"Update\" frameBorder=\"0\" id=\"otaiframe\" scrolling=\"no\" onload=\"this.contentWindow.document.documentElement.scrollTop=130\"></iframe>\n";    
  ptr += "<br><br><input type=\"button\" class=\"button button-config\" value=\"Restart\" onclick=\"location.href='/restart';\">\n";
  ptr += "</body>\n";
  ptr += "</html>\n";  
  return ptr;
}

void drawmatrix(const uint8_t bmp[], const uint8_t inv_bmp[]) {     
  if (MatrixConnected) {  
    //matrix.clear();
    matrix.drawBitmap(-1, 0, bmp, 8, 15, 255);
    matrix.drawBitmap(-1, 0, inv_bmp, 8, 15, 0);
  }
}

void clearmatrix() {
  if (MatrixConnected) {
    matrix.clear();
  }
}

void matrixloop() {  
  if (poweronoffState == LOW) {
    clearmatrix();
    return;
  } 
  if (millis() - previousBitmapTime >= 200) {    
    previousBitmapTime = millis();   
    if (MatrixConnected) {
      matrix.setRotation(1);              
      switch(MatrixBitmap) {
        case 1: 
          drawmatrix(eq1_bmp, eq1_inv_bmp);
          break;
        case 2: 
          drawmatrix(eq2_bmp, eq2_inv_bmp);
          break;
        case 3: 
          drawmatrix(eq3_bmp, eq3_inv_bmp);
          break;
        case 4: 
          drawmatrix(eq4_bmp, eq4_inv_bmp);
          break;
        case 5: 
          drawmatrix(eq5_bmp, eq5_inv_bmp);
          break;
        case 6: 
          drawmatrix(eq6_bmp, eq6_inv_bmp);
          break;
        case 7: 
          drawmatrix(eq7_bmp, eq7_inv_bmp);
          break;
        case 8: 
          drawmatrix(eq8_bmp, eq8_inv_bmp);
          break;
        case 9: 
          drawmatrix(eq9_bmp, eq9_inv_bmp);
          break;
        case 10: 
          drawmatrix(eq10_bmp, eq10_inv_bmp);
          break;
        case 11: 
          drawmatrix(eq11_bmp, eq11_inv_bmp);
          break;
        case 12: 
          drawmatrix(eq12_bmp, eq12_inv_bmp);
          break;
        case 13: 
          drawmatrix(eq13_bmp, eq13_inv_bmp);
          break;
        case 14: 
          drawmatrix(eq14_bmp, eq14_inv_bmp);
          break;
        case 15: 
          drawmatrix(eq15_bmp, eq15_inv_bmp);
          break;
        case 16: 
          drawmatrix(eq16_bmp, eq16_inv_bmp);
          break;
        case 17: 
          drawmatrix(eq17_bmp, eq17_inv_bmp);
          break;
        case 18: 
          drawmatrix(eq18_bmp, eq18_inv_bmp);
          break;
        case 19: 
          drawmatrix(eq19_bmp, eq19_inv_bmp);
          break;
        case 20: 
          drawmatrix(eq20_bmp, eq20_inv_bmp);
          break;
      }
      
      MatrixBitmap++;
      if ((MatrixBitmap < 1) || (MatrixBitmap > 20)) { 
        MatrixBitmap = 1; 
      }  
    }         
  }  
}

void reconnect() {
  if ((!mqttClient.connected()) & (0 < strlen(mqtt_server))) {
    unsigned long now = millis();
    if (now - lastTryConnect > 5000) {
      lastTryConnect = now;
      Serial.print("Attempting MQTT connection...");       
      String clientId = deviceId;
      if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {      
        Serial.println("\033[1;32mconnected\033[0m");
        if (mqtt_ha_topic != "") mqttHaDiscoveryConfig();
        publishIpState();
        publishMacState();
        publishHostnameState();
        publishSsidState();
        publishBssidState();
        
        // Topic abo
        mqttClient.subscribe(powerCommandTopic.c_str());
        mqttClient.subscribe(longcmndTopic.c_str());
        mqttClient.subscribe(shortcmndTopic.c_str());
      } else {
        Serial.print("\033[1;31mfailed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds\033[0m");
      }
    }
  }
}

void mqttloop()
{
  if (!mqttClient.connected()) {
    reconnect();
    if (!mqttClient.connected()) return;
  } 

  if (ipAddress != WiFi.localIP()) { 
    ipAddress = WiFi.localIP();
    publishIpState();
    publishMacState();
    publishHostnameState();
    publishSsidState();
    publishBssidState();
  }  
  
  mqttClient.loop();  
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    publishRssiStateChange();
    publishFreeRamStateChanged();
  }
}

void publishRssiStateChange() {
  if ((wifiRssi <= WiFi.RSSI() -4 ) || (wifiRssi >= WiFi.RSSI() +4)) { 
    wifiRssi = WiFi.RSSI();
    publishRssiState();
  }
}

void publishFreeRamStateChanged() {
  if (freeRam != (ESP.getFreeHeap()/1024)) {
    freeRam = (ESP.getFreeHeap()/1024);
    publishFreeRamState();
  }
}

void CheckInputs() {
  powerstateState = analogRead(powerstate);  
  boolean newPoweronoffState = (powerstateState > 300);
  if(poweronoffState != newPoweronoffState) {
    poweronoffState = newPoweronoffState;
    publishPowerState();
  }
    
  ServoValue = pulseIn(PulseInPin, HIGH, 100000);
  if (LastServoValue != ServoValue) {
    publishServoState();
    LastServoValue = ServoValue;
  }
  
  if (DeloreanIsFlying != (ServoValue > 0)) {
    DeloreanIsFlying = ServoValue > 0;
    Serial.println("DeloreanIsFlying = " + String(DeloreanIsFlying));
    publishMotionState();
  }  
}

void mqttHaDiscoveryConfig() {  
  StaticJsonDocument<MSG_BUFFER_SIZE> doc; 
  JsonObject jsonDevice = doc.createNestedObject("dev"); // Device
  JsonArray identifiers = jsonDevice.createNestedArray("ids");
  identifiers.add(deviceId); 
  jsonDevice["name"] = "Flying Delorean";  
  jsonDevice["model"] = "Wifi Controller for Did3D Flying DELOREAN";
  jsonDevice["mf"] = "sequ3ster";  
  jsonDevice["sn"] = ESP.getChipId();

  #ifdef ESP8266
    jsonDevice["hw"] = "ESP8266 " + String(ESP.getFlashChipSize()/1048576) + " MB";
  #endif

  #ifdef ESP32
    jsonDevice["hw"] = "ESP32 " + String(ESP.getFlashChipSize()/1048576) + " MB";
  #endif

  jsonDevice["sw"] = String(swversion);  
  jsonDevice["cu"] = "http://" + WiFi.localIP().toString();

  JsonObject jsonOrigin = doc.createNestedObject("o"); // Origin
  jsonOrigin["name"] = "delorean2mqtt";
  jsonOrigin["sw"] = String(swversion);
  jsonOrigin["url"] = "https://github.com/sequ3ster/esp_flying_delorean";
  
  JsonObject components = doc.createNestedObject("cmps"); // Components
  JsonObject jsonPowerBtn = components.createNestedObject(deviceId + "_switch");
  jsonPowerBtn["p"] = "switch"; 
  jsonPowerBtn["name"] = "Power";
  jsonPowerBtn["cmd_t"] = powerCommandTopic;  
  jsonPowerBtn["stat_t"] = powerStateTopic;
  jsonPowerBtn["uniq_id"] = deviceId + "_switch";

  JsonObject jsonShortBtn = components.createNestedObject(deviceId + "_short");
  jsonShortBtn["p"] = "button"; 
  jsonShortBtn["name"] = "Scene";
  jsonShortBtn["cmd_t"] = shortcmndTopic;  
  jsonShortBtn["uniq_id"] = deviceId + "_short";

  JsonObject jsonLongBtn = components.createNestedObject(deviceId + "_long");
  jsonLongBtn["p"] = "button"; 
  jsonLongBtn["name"] = "Mode";
  jsonLongBtn["cmd_t"] = longcmndTopic;
  jsonLongBtn["uniq_id"] = deviceId + "_long";
  
  JsonObject jsonMotion = components.createNestedObject(deviceId + "_motion");
  jsonMotion["p"] = "binary_sensor"; 
  jsonMotion["name"] = "Motion";  
  jsonMotion["stat_t"] = motionStateTopic;
  jsonMotion["uniq_id"] = deviceId + "_motion";
  jsonMotion["ic"] = "mdi:airplane";
  jsonMotion["pl_on"] = "on";
  jsonMotion["pl_off"] = "off";

  JsonObject jsonServo = components.createNestedObject(deviceId + "_servo");
  jsonServo["p"] = "sensor"; 
  jsonServo["name"] = "Servo";  
  jsonServo["stat_t"] = servoStateTopic;
  jsonServo["ic"] = "mdi:turbine";
  jsonServo["uniq_id"] = deviceId + "_servo";  
  jsonServo["unit_of_meas"] = "°";

  JsonObject jsonMacAdd = components.createNestedObject(deviceId + "_mac");
  jsonMacAdd["p"] = "sensor"; 
  jsonMacAdd["name"] = "MAC Address";  
  jsonMacAdd["ent_cat"] = "diagnostic";
  jsonMacAdd["ic"] = "mdi:network-pos";  
  jsonMacAdd["stat_t"] = "stat/" + deviceId + "/mac";
  jsonMacAdd["uniq_id"] = deviceId + "_mac";

  JsonObject jsonBssid = components.createNestedObject(deviceId + "_bssid");
  jsonBssid["p"] = "sensor"; 
  jsonBssid["name"] = "Wifi BSSID";  
  jsonBssid["ent_cat"] = "diagnostic";
  jsonBssid["ic"] = "mdi:network-pos";  
  jsonBssid["stat_t"] = "stat/" + deviceId + "/bssid";
  jsonBssid["uniq_id"] = deviceId + "_bssid";

  JsonObject jsonIpadd = components.createNestedObject(deviceId + "_ip");
  jsonIpadd["p"] = "sensor"; 
  jsonIpadd["name"] = "IP Address";  
  jsonIpadd["ent_cat"] = "diagnostic";
  jsonIpadd["ic"] = "mdi:ip-network";
  jsonIpadd["stat_t"] = ipStateTopic;
  jsonIpadd["uniq_id"] = deviceId + "_ip";

  JsonObject jsonHostname = components.createNestedObject(deviceId + "_host");
  jsonHostname["p"] = "sensor"; 
  jsonHostname["name"] = "Hostname";  
  jsonHostname["ent_cat"] = "diagnostic";
  jsonHostname["ic"] = "mdi:lan-connect";  
  jsonHostname["stat_t"] = "stat/" + deviceId + "/hostname";
  jsonHostname["uniq_id"] = deviceId + "_host";

  JsonObject jsonRssi = components.createNestedObject(deviceId + "_rssi");
  jsonRssi["p"] = "sensor"; 
  jsonRssi["name"] = "Wifi RSSI";  
  jsonRssi["ent_cat"] = "diagnostic";
  jsonRssi["ic"] = "mdi:wifi";
  jsonRssi["unit_of_meas"] = "db";
  jsonRssi["stat_t"] = "stat/" + deviceId + "/rssi";
  jsonRssi["uniq_id"] = deviceId + "_rssi";

  JsonObject ssid = components.createNestedObject(deviceId + "_ssid");
  ssid["p"] = "sensor"; 
  ssid["name"] = "Wifi SSID";  
  ssid["ent_cat"] = "diagnostic";
  ssid["ic"] = "mdi:router-network-wireless";  
  ssid["stat_t"] = "stat/" + deviceId + "/ssid";
  ssid["uniq_id"] = deviceId + "_ssid";

  JsonObject freeram = components.createNestedObject(deviceId + "_freeram");
  freeram["p"] = "sensor"; 
  freeram["name"] = "Free RAM";  
  freeram["ent_cat"] = "diagnostic";
  freeram["ic"] = "mdi:memory";
  freeram["unit_of_meas"] = "KByte";
  freeram["stat_t"] = "stat/" + deviceId + "/freeram";
  freeram["uniq_id"] = deviceId + "_freeram";

  String output;
  serializeJson(doc, output);

  Serial.print("Publishing config to: ");
  Serial.print(mqttDeviceConfigTopic);
  Serial.println(" " + output); 

  
  if (!mqttClient.publish(mqttDeviceConfigTopic.c_str(), output.c_str(), false)) { 
     Serial.println("\033[1;31mFailed to publish power config!\033[0m");
  } 
  publishPowerState();  
}

void unpublishHADiscoveryConfig() {
  mqttClient.publish(mqttDeviceConfigTopic.c_str(), "");
}

void publishIpState() {
  if(!mqttClient.connected()) return;

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(ipStateTopic);
  Serial.println("\033[1;37m \033[44m " + ipAddress.toString() + " \033[0m");

  if (!mqttClient.publish(ipStateTopic.c_str(), ipAddress.toString().c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishMacState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/mac";

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + WiFi.macAddress() + " \033[0m");
  
  if (!mqttClient.publish(topic.c_str(), WiFi.macAddress().c_str(), true)) {
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishHostnameState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/hostname";

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + WiFi.hostname() + " \033[0m");
  
  if (!mqttClient.publish(topic.c_str(), WiFi.hostname().c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishRssiState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/rssi";

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + String(WiFi.RSSI()) + " \033[0m");
  
  if (!mqttClient.publish(topic.c_str(), String(WiFi.RSSI()).c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishSsidState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/ssid";

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + String(WiFi.SSID()) + " \033[0m");
  
  if (!mqttClient.publish(topic.c_str(), String(WiFi.SSID()).c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishBssidState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/bssid";
  String output = macBytesToString(WiFi.BSSID());

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + output + " \033[0m");
  
  if (!mqttClient.publish(topic.c_str(), output.c_str(), true)) {
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

String macBytesToString(byte mac[6]) {
  String macAddress = "";
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) {
      macAddress += "0";
    }
    macAddress += String(mac[i], HEX);
    if (i < 5) {
      macAddress += ":";
    }
  }
  macAddress.toUpperCase();
  return macAddress;
}

void publishFreeRamState() {
  if(!mqttClient.connected()) return;

  String topic = "stat/" + deviceId + "/freeram";

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(topic);
  Serial.println("\033[1;37m \033[44m " + String(freeRam) + " \033[0m");  

  if (!mqttClient.publish(topic.c_str(), String(freeRam).c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishMotionState() {
  if(!mqttClient.connected()) return;

  // StaticJsonDocument<30> doc;
  // doc["motion"] = DeloreanIsFlying;

  String output = (DeloreanIsFlying ? "on" : "off");

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(motionStateTopic);
  Serial.println("\033[1;37m \033[44m " + String(DeloreanIsFlying) + " \033[0m");

  //if (!mqttClient.publish(motionStateTopic.c_str(), String(DeloreanIsFlying).c_str(), true)) {    
  if (!mqttClient.publish(motionStateTopic.c_str(), output.c_str(), true)) {  
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishServoState() {
  if(!mqttClient.connected()) return;

  // StaticJsonDocument<30> doc;
  // doc["servo"] = ServoValue;

  // String output;
  // serializeJson(doc, output);

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(servoStateTopic);
  Serial.println("\033[1;37m \033[44m" + String(ServoValue / 10) + " \033[0m");

  if (!mqttClient.publish(servoStateTopic.c_str(), String(ServoValue / 10).c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishPowerState() {
  if(!mqttClient.connected()) return;

  StaticJsonDocument<30> doc; 
  String output = String(poweronoffState ? "ON" : "OFF");
  
  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(powerStateTopic);
  Serial.println("\033[1;37m \033[44m " + output + " \033[0m");

  if (!mqttClient.publish(powerStateTopic.c_str(), output.c_str(), true)) { 
     Serial.println("\033[1;31mMQTT Failed to publish power state!\033[0m");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\033[1;36mMQTT Message arrived\033[0m [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("\033[0;30m]\033[46m " + message + " \033[0m");

  if (!DeloreanIsFlying) {  
    if (String(topic) == powerCommandTopic) {            
      poweronoffState = message == "ON";
      digitalWrite(poweronoff, poweronoffState);    
      publishPowerState();
      server.sendHeader("Location", "/",true);  
    } else if(String(topic) == longcmndTopic ) {
      timeoutTimeButton = 3100;
      PushButton();
      server.sendHeader("Location", "/",true); 
    } else if(String(topic) == shortcmndTopic) {
      timeoutTimeButton = 1000;
      PushButton();
      server.sendHeader("Location", "/",true); 
    }
  }
}

void loop() {    
  if (digitalRead(eqModOnly) == 1) {
    CheckInputs();  
    server.handleClient();      
    PushButton();
    mqttloop();  
  }
  matrixloop();  
}

