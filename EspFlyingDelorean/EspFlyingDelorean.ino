/*********
  Wifi Control for Flying Delorean from Did3d. (https://www.did3d.fr)
  You can buy the 3D Model here: https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model
  or here: https://cults3d.com/en/3d-model/art/delorean-volante-v2-hq-1-8-scale-530mm-print-3d

  Benjamin Heimann
  Complete project details at https://github.com/sequ3ster/esp_flying_delorean
  https://www.facebook.com/groups/599517350568861


  *** Buy List ***
  Did3d 3D Model: https://www.cgtrader.com/3d-print-models/miniatures/vehicles/flying-delorean-v2-hq-1-8-scale-530mm-3d-print-model
                  https://cults3d.com/en/3d-model/art/delorean-volante-v2-hq-1-8-scale-530mm-print-3d
  ES8266 D1 mini: https://de.aliexpress.com/item/1005006890254253.html
                  https://www.amazon.de/dp/B0754N794H                  

  ESP8266 Board: http://arduino.esp8266.com/stable/package_esp8266com_index.json
  File: https://42project.net/esp8266-webserverinhalte-wie-bilder-png-und-jpeg-aus-dem-internen-flash-speicher-laden/
  Flash: https://espressif.github.io/esptool-js/
  MQTT Youtube: https://www.youtube.com/watch?v=n9QXRcFqbLY
*********/

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <PubSubClient.h>
#include "animation.h" 

#ifdef ESP32
  #include <SPIFFS.h>
#endif

// Set web server port number to 80
const unsigned int port = 80;

// LED Debug Options. Please select only one. For deactivate the LED, set all debug variables to false.
bool debugPower = false;
bool debugButton = false;
bool debugMatrix = false;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6];
char mqtt_user[40];
char mqtt_password[40];
//#define BROKER_ADDR IPAddress(192,168,1,11)

// --------------------------------------------------------------
// Please do not change anything from here on
// --------------------------------------------------------------
Adafruit_IS31FL3731_Wing matrix = Adafruit_IS31FL3731_Wing();

ESP8266WebServer server(port);

//flag for saving data
bool shouldSaveConfig = false;

int MatrixBitmap=1;
bool MatrixConnected=false;

unsigned long previousTime = 0; 
unsigned long previousBitmapTime = 0; 
unsigned long TimeOutButton = 0; 
unsigned long LastTimeFlying = 0; 

const long timeoutTime = 200;
long timeoutTimeButton = 0;

uint8_t PulseInPin = 13; // ESP8266 PulseIn = D7 --- 100k ohm
uint8_t pushbutton = 12; //ESP8266 GPIO12 = D6 --- Diode or 470-1k ohm
uint8_t poweronoff = 14; //ESP8266 GPIO14 = D5
uint8_t StatusIndicator = 2; //ESP8266 GPIO2 = D4

bool pushbuttonState = HIGH;
bool poweronoffState = LOW;
bool StatusIndicatorState = HIGH; 
bool DeloreanIsFlying = false; 
bool LastDeloreanIsFlying = false; 
int ServoValue = 0; 
int LastServoValue = 0; 

// --- MQTT Device ---
String mac = WiFi.macAddress(); // Eindeutige Basis für IDs
String deviceId;
String motionUniqueId;
String powerUniqueId;
String deviceName = "Flying Delorean";
String motionConfigTopic;
String deviceConfigTopic;
String motionStateTopic;
String servoStateTopic;
String powerConfigTopic;
String powerStateTopic;
String powerCommandTopic;
String powerTopic;

WiFiClient espClient;
PubSubClient client(espClient);
//unsigned long lastMsg = 0;
unsigned long lastTryConnect = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;        
}

void setup() {
  Serial.begin(74800);
  // Initialize the output variables as outputs
  pinMode(pushbutton, OUTPUT);
  pinMode(poweronoff, OUTPUT);
  pinMode(StatusIndicator, OUTPUT);
  pinMode(PulseInPin, INPUT);
  // Set outputs to LOW
  pushbuttonState = HIGH;
  poweronoffState = LOW;
  StatusIndicatorState = HIGH;
  digitalWrite(pushbutton, pushbuttonState);
  digitalWrite(poweronoff, poweronoffState);
  digitalWrite(StatusIndicator, StatusIndicatorState);
  //deviceMac.replace(":", "");

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
          //strcpy(api_token, json["api_token"]);
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
  
  //Serial.println("mqtt_server1: " + String(mqtt_server) + ", mqtt_port: " + String(mqtt_port));
  //Serial.print("mqtt_user1: " + String(mqtt_user) + ", mqtt_password: " + String(mqtt_password));  
  
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);      
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);  

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);   

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
  //mqtt_port = atoi(custom_mqtt_port.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());  

//save the custom parameters to FS
  if (shouldSaveConfig) {
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

  // Print local IP address and start web server   
  server.on("/", handle_root);
  server.on("/buttons", handle_buttons);
  server.on("/btn", HTTP_POST, handle_btn);
  //server.on("/config", HTTP_POST, handle_cfg);
  server.on("/config", handle_cfg);
  server.on("/resetconfig", handle_resetcfg);
  server.on("/hadiscoveryon", handle_publishHaDiscovery);
  server.on("/hadiscoveryoff", handle_unpublishHaDiscovery);  
  server.on("/events", handle_SSE);
  server.onNotFound(handleWebRequests);
  
  server.begin();
  Serial.println("\033[1;32mHTTP Server started\033[0m");

  // Init MQTT Client
  mac = WiFi.macAddress(); // MAC Adresse holen NACHDEM WLAN verbunden ist
  mac.replace(":", "");
  deviceId = "delorean_" + mac.substring(mac.length() - 4);
  //deviceId.replace(":", "");
  motionUniqueId = deviceId + "_motion";  
  motionConfigTopic = "homeassistant/binary_sensor/" + deviceId + "/config";
  motionStateTopic = "stat/" + deviceId + "/motion";
  servoStateTopic = "stat/" + deviceId + "/servo";
  powerUniqueId = deviceId + "_switch";
  deviceConfigTopic = "homeassistant/device/" + deviceId + "/config";
  powerConfigTopic = "homeassistant/switch/" + deviceId + "/config";
  powerStateTopic = "stat/" + deviceId + "/switch";
  powerCommandTopic = "cmnd/" + deviceId + "/switch";
  powerTopic = "homeassistant/switch/" + deviceId;
  
  client.setServer(mqtt_server, String(mqtt_port).toInt());
  client.setCallback(callback);  
  client.setBufferSize(512);

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

void handle_buttons() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", sendHTMLHead());
  server.sendContent(sendHTMLButtons());    
  server.sendContent(""); 
}

void PushButton() {
  if (timeoutTimeButton > 0) {
    TimeOutButton = millis() + timeoutTimeButton;
    pushbuttonState = LOW;    
    timeoutTimeButton = 0;
    digitalWrite(pushbutton, pushbuttonState);
    if (debugButton) {
      digitalWrite(StatusIndicator, pushbuttonState); 
    }
  }  else if (millis() > TimeOutButton) {
    pushbuttonState = HIGH;    
    digitalWrite(pushbutton, pushbuttonState);
    if (debugButton) {
      digitalWrite(StatusIndicator, pushbuttonState); 
    }
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

  if (btnPin == "1") {
    Serial.println("power on/off");    
    if (!DeloreanIsFlying) {
      poweronoffState = evaluate_btn_state(btnState);
      //Serial.println("btnState: " + btnState);
      //poweronoffState = !poweronoffState;
      digitalWrite(poweronoff, poweronoffState);
    }
    publishPowerState();
  } else if (btnPin == "2") {      
    timeoutTimeButton = btnState.toInt();
    Serial.println("press "+String(timeoutTimeButton)+" ms"); 
    pushbuttonState = LOW;
  } else {
    Serial.println("btn unknow: "+btnPin);
  }
  server.sendHeader("Location", "/buttons",true);  
  server.send(302, "text/plain", "");
}
  
void handle_resetcfg() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Reset Config");

  WiFiManager wifiManager;
  
  wifiManager.resetSettings();  
  delay(3000);
  ESP.restart();
  delay(5000);
}

void handle_cfg() {
  WiFiManager wifiManager;
  wifiManager.startWebPortal();  

  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Start Config");  
}

void handle_NotFound() {
  server.send(404, "text/html", sendPageUnknown());
}

void handle_publishHaDiscovery() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Reset Config");

  publishHADiscoveryConfig();        
}

void handle_unpublishHaDiscovery() {
  server.sendHeader("Location", "/",true);  
  server.send(302, "text/plain", "");
  Serial.println("Reset Config");

  unpublishHADiscoveryConfig();        
}

/*void handle_SSE() {
  server.sendHeader("Content-Type", "text/event-stream");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(202, "text/event-stream", "");
  //while (true) {
    server.sendContent("data: " + String(poweronoffState ? "On" : "Off") + "\n\n");
  //  delay(1000); // Sende den Status jede Sekunde
  //}
}*/

void handle_SSE() {
   WiFiClient client = server.client();
  
  if (client) {
    //Serial.println("new client");
    serverSentEventHeader(client);
    //while (client.connected()) {
      serverSentEvent(client);
      /*server.sendHeader("Content-Type", "text/event-stream");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendContent("");
      server.sendContent("data: " + String(poweronoffState ? "On" : "Off") + "\n\n");*/
  
      delay(16); // round about 60 messages per second
    //}
    
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    //Serial.println("client disconnected");
  }
}

void serverSentEventHeader(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  //client.println("Content-Type: text/event-stream;charset=UTF-8");  
  client.println("Content-Type: text/event-stream");
  //client.println("Access-Control-Allow-Origin: *");  // allow any connection. We don't want Arduino to host all of the website ;-)
  client.println("Cache-Control: no-cache");  // refresh the page automatically every 5 sec
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();
}

void serverSentEvent(WiFiClient client) {
  //client.println("Content-Type: text/event-stream;charset=UTF-8");  
  //client.println("event: message"); // this name could be anything, really.  
  String Output = "data: " + String(poweronoffState ? "ON" : "OFF");
  client.println(Output);
  client.println();
  //Serial.println("SSE: " + Output);
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
  String ptr = "<form action=\"btn\" method=\"post\">\n";
  ptr += "  <input type=\"hidden\" name=\"btnPin\" value=\""+btnPin+"\">\n";

  String btnStateStr = String(btnState ? "on" : "off");
  
  /*if (btnState)
  {
    btnStateStr = "off";
  }
  else
  {
    btnStateStr = "on";
  }*/

  ptr += "  <input type=\"hidden\" name=\"btnState\" value=\""+btnStateStr+"\">\n";
  ptr += "  <input type=\"submit\" id=\"powerButton\" class=\"button button-"+btnStateStr+"\" value=\"";
  btnStateStr.toUpperCase();
  ptr += btnStateStr+"\">\n";

  ptr += "</form>\n";
  //ptr += "<button id=\"powerButton\">Loading...</button>";
  //ptr += "<button id=\"powerButton\">Loading...</button>";
  ptr += "<script type=\"text/javascript\">";
  ptr += "  const powerButton = document.getElementById('powerButton');";
  ptr += "  const eventSource = new EventSource('/events');";
  //ptr += "  eventSource.onmessage = function(event) { powerButton.textContent = event.data; }";
  //ptr += "  eventSource.onmessage = function(event) { powerButton.innerText = event.data; }";
  //ptr += "  eventSource.onmessage = function(event) { powerButton.value = event.data; powerbutton.className = \"button button-\" + event.data.toLowerCase(); }";  
  ptr += "  eventSource.onmessage = function(event) { ";
  ptr += "  powerButton.value = event.data;";
  //ptr += "  powerButton.style.color = 'white';";
  //ptr += "  if (event.data == 'ON') { powerButton.style.color = 'white'; powerButton.style.textShadow = '0px 0px 10px white'; }";
  //ptr += "  else { powerButton.style.color = 'black'; powerButton.style.textShadow = ''; }";
  ptr += "  if (event.data == 'ON') { powerButton.style.color = 'white'; powerButton.className = \"button button-on\"; }";
  ptr += "  else { powerButton.style.color = 'black'; powerButton.className = \"button button-off\"; }";

  //ptr += "  eventSource.onmessage = function(event) { powerButton.value = event.data; powerbutton.classList.add(\"button button-\" + event.data.toLowerCase()); }";  
  //ptr += "  eventSource.onmessage = function(event) { window.location.reload() }";
  /*ptr += "  var powerButton = document.getElementById(\"powerButton\");";
  ptr += "  var source = new EventSource('/events');";
  ptr += "  source.addEventListener(\"message\", function(e) {";
  //ptr += "      powerButton.innerHTML = e.data + '<br>';";
  ptr += "      confirm(\"Event\")";
  ptr += "      powerButton.innerText = e.data;";*/
  ptr += "  }";
  ptr += "</script>";
  return ptr;
}

String build_push_btn_form(String btnStr, String functionStr, int btnDelay) {
  String ptr = "<form action=\"btn\" method=\"post\">\n";
  ptr += "  <input type=\"hidden\" name=\"btnPin\" value=\"2\">\n";
  ptr += "  <input type=\"hidden\" name=\"btnState\" value=\""+String(btnDelay)+"\">\n";
  ptr += "  <input type=\"submit\" class=\"button button-"+functionStr+"\" value=\"";
  ptr += btnStr+"\">\n";
  ptr += "</form>\n";
  return ptr;
}

String build_cfg_btn_form(String btnStr, String functionStr) {
  String ptr = "<div class=\"button button-reset\">Reset Config</div>";
  //ptr += "<script>\n";
  ptr += "<script type=\"text/javascript\">\n";  
  ptr += "let btnEl = document.querySelector(\"div\")\n";
  ptr += "btnEl.addEventListener(\"click\", () => {";
  ptr += "if(confirm('After the reset, you must connect to the AP again and reconfigure everything! Do you really want to reset the entire configuration?'))";
  ptr += "window.location.href=\"/resetconfig\";})\n";  
  //ptr += "window.location.href=\"/resetconfig\";})\n";  
  ptr += "</script>\n";

  return ptr;
}

String sendHTMLHead() {
  String ptr = "<!DOCTYPE html> <html>\n";
  
  ptr += "<head>";
  
  ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";

  ptr += "<title>Flying Delorean Did3D</title>\n";
  
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 25px; background-color: black;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;} form {margin-bottom: 30px;} img{max-width:300px;}\n";  
  ptr += ".button {display: block;width: 80px;background-color: #deea26;border: none;text-decoration: none;font-size: 25px;margin: auto;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-off {background-color: #7F151A;color: black;}\n";
  ptr += ".button-off:active {background-color: #7F151A;color: black;}\n";
  ptr += ".button-on {background-color: #cb2129;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-on:active {background-color: #A01B21;color: white; text-shadow: 0px 0px 10px white; box-shadow: 0px 0px 10px white; }\n";
  ptr += ".button-short {background-color: #70f74f;color: black;}\n";
  ptr += ".button-short:active {background-color: #56bc3c;color: black;}\n";
  ptr += ".button-long {background-color: #dfc048;color: black;}\n";
  ptr += ".button-long:active {background-color: #b59a3b;color: black;}\n";
  ptr += ".button-reset {background-color: black;color: #7f94a1;position: absolute;bottom: 125px;left: 25px;font-size: 12px}\n";
  ptr += ".button-reset:active {background-color: #black;color: blue;position: absolute;bottom: 125px;left: 25px;font-size: 12px}\n";
  ptr += ".did3d {background-color: #black; position: absolute;bottom: 100px;right: 25px;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  return ptr;
}

String sendHTMLBody() {  
  String ptr = "<body>\n";  
  ptr += "<p><img src=\"/BTTF2.webp\" alt=\"BTTF2\"></p>\n";
  ptr += "<p><a href=\"https://www.did3d.fr\" class=\"did3d\" target=\"_blank\" rel=\"noopener noreferrer\"><img src=\"/Did3d.webp\" alt=\"Did3D.fr\"></a></p>\n";
  ptr += "<br>\n";
  ptr += "<iframe src=\"/buttons\" height=\"220\" width=\"300\" title=\"Buttons\" frameBorder=\"0\"></iframe>\n";  
  ptr += build_cfg_btn_form("Reset Config", "reset");  
 
  ptr += "</body>\n";
  ptr += "</html>\n";  
  return ptr;
}

String sendHTMLButtons() {
  String ptr = "<body>\n";    
  ptr +=  build_btn_form("1", poweronoffState); 
  ptr += build_push_btn_form("Scene", "short", 1000);
  ptr += build_push_btn_form("Mode", "long", 3100);
 
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void drawmatrix(const uint8_t bmp[], const uint8_t inv_bmp[]) {    
  if (debugMatrix) {
    StatusIndicatorState = ! StatusIndicatorState;
    digitalWrite(StatusIndicator, StatusIndicatorState);  
  }
  
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
  if (millis() - previousBitmapTime >= timeoutTime) {    
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
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastTryConnect > 5000) {
      lastTryConnect = now;
      Serial.print("Attempting MQTT connection...");       
      String clientId = deviceId;
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {      
        Serial.println("\033[1;32mconnected\033[0m");        
//        publishDiscoveryConfig();
        // Hier ggf. Command Topics abonnieren
        // client.subscribe("mein_esp8266/switch/relay/command");
        client.subscribe(powerCommandTopic.c_str());
      } else {
        Serial.print("\033[1;31mfailed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds\033[0m");
      }
    }
  }
}

void mqttloop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  /*unsigned long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    publishMotionState();
    publishPowerState();  
  }*/
}

void CheckDeloreanIsFlying() {
  LastDeloreanIsFlying = DeloreanIsFlying;
  ServoValue = pulseIn(PulseInPin, HIGH, 100000);
  if (LastServoValue != ServoValue) {
    publishServoState();
    LastServoValue = ServoValue;
  }
  
  DeloreanIsFlying = ServoValue > 0;
  if (LastDeloreanIsFlying != DeloreanIsFlying) {
    Serial.println("DeloreanIsFlying = " + String(DeloreanIsFlying));
    publishMotionState();
  }
}

void publishMotionConfig() {
  // Verwende ArduinoJson für einfacheres Erstellen
  StaticJsonDocument<256> doc; // Größe anpassen nach Bedarf
  String docName = deviceName + " motion";
  doc["name"] = docName; // z.B. "Mein ESP8266 Projekt Temperatur"
  doc["dev_cla"] = "motion"; // z.B. "Mein ESP8266 Projekt Temperatur"
  doc["stat_t"] = motionStateTopic;  
  //doc["unit_of_measurement"] = "";
  doc["value_template"] = "{{ value_json.motion|default(false) }}"; // Falls JSON gesendet wird
  //doc["value_template"] = "{{ value }}"; // Wenn nur der Wert gesendet wird
  doc["uniq_id"] = motionUniqueId; // "esp8266_AABBCCDDEEFF_temp"
  //doc["force_update"] = true;  

  // --- Geräteinformationen ---
  JsonObject device = doc.createNestedObject("dev");
  JsonArray identifiers = device.createNestedArray("ids");
  identifiers.add(deviceId); // Füge den deviceId-String zum Array hinzu  
  //identifiers.add(deviceMac); // Füge den deviceId-String zum Array hinzu  
  device["name"] = deviceName; // "Mein ESP8266 Projekt"
  //device["manufacturer"] = "sequ3ster";
  //device["model"] = "Flysing Delorean V2";
  //device["sw_version"] = "0.1";

  String output;
  serializeJson(doc, output);

  Serial.print("Publishing config to: ");
  Serial.println(motionConfigTopic);
  Serial.println(output);
  
  // Konfiguration mit Retain-Flag senden
  if (!client.publish(motionConfigTopic.c_str(), output.c_str(), false)) { // true für retain
     Serial.println("\033[1;31mFailed to publish motion config!\033[0m");
  }
  publishMotionState();

  // *** Hier würden weitere Konfigurationen für andere Entitäten folgen ***
  // z.B. für einen Schalter, mit dem gleichen "device"-Block, aber anderem
  // "component", "name", "unique_id", "command_topic" etc.
}

void publishPowerConfig() {
  // Verwende ArduinoJson für einfacheres Erstellen
  StaticJsonDocument<180> doc; // Größe anpassen nach Bedarf
  String docName = deviceName + " Power";
  /*doc["name"] = docName; // z.B. "Mein ESP8266 Projekt Temperatur"
  doc["device_class"] = "switch"; // z.B. "Mein ESP8266 Projekt Temperatur"
  doc["command_topic"] = powerCommandTopic;  
  doc["state_topic"] = powerStateTopic;  
  //doc["unit_of_measurement"] = "";
  //doc["value_template"] = "{{ value_json.motion|default(false) }}"; // Falls JSON gesendet wird
  //doc["value_template"] = "{{ value }}"; // Wenn nur der Wert gesendet wird
  doc["unique_id"] = powerUniqueId; // "esp8266_AABBCCDDEEFF_temp"
  doc["force_update"] = true;*/

  //doc["~"] = powerTopic;
  doc["name"] = docName;
  //doc["cmd_t"] = "~/set";
  doc["cmd_t"] = powerCommandTopic;  
  //doc["stat_t"] = "~/state";
  doc["stat_t"] = powerStateTopic;
  doc["uniq_id"] = powerUniqueId;
  //doc["force_update"] = true;

  // --- Geräteinformationen ---
  JsonObject device = doc.createNestedObject("dev");
  JsonArray identifiers = device.createNestedArray("ids");
  identifiers.add(deviceId); // Füge den deviceId-String zum Array hinzu
  //identifiers.add(deviceMac); // Füge den deviceId-String zum Array hinzu  
  //device["name"] = deviceName; // "Mein ESP8266 Projekt"
  //device["manufacturer"] = "sequ3ster";
  //device["model"] = "Flysing Delorean V2";
  //device["sw_version"] = "0.1";

  String output;
  serializeJson(doc, output);

  Serial.print("Publishing config to: ");
  Serial.print(powerConfigTopic);
  Serial.println(" " + output); 

  // Konfiguration mit Retain-Flag senden
  if (!client.publish(powerConfigTopic.c_str(), output.c_str(), false)) { // true für retain
     Serial.println("\033[1;31mFailed to publish power config!\033[0m");
  } 
  publishPowerState();

  // *** Hier würden weitere Konfigurationen für andere Entitäten folgen ***
  // z.B. für einen Schalter, mit dem gleichen "device"-Block, aber anderem
  // "component", "name", "unique_id", "command_topic" etc.
}

void publishHADiscoveryConfig() {
  publishMotionConfig();
  publishPowerConfig();
}

void unpublishHADiscoveryConfig() {
  client.publish(motionConfigTopic.c_str(), ""); // Delete old config
  client.publish(powerConfigTopic.c_str(), ""); // Delete old config    
}

void publishMotionState() {
  if(!client.connected()) return;

  StaticJsonDocument<30> doc;
  doc["motion"] = DeloreanIsFlying;

  String output;
  serializeJson(doc, output);

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(motionStateTopic);
  Serial.println("\033[1;37m \033[44m " + output + " \033[0m");

  // Konfiguration mit Retain-Flag senden
  if (!client.publish(motionStateTopic.c_str(), output.c_str(), true)) { // true für retain
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishServoState() {
  if(!client.connected()) return;

  StaticJsonDocument<30> doc;
  doc["servo"] = ServoValue;

  String output;
  serializeJson(doc, output);

  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(servoStateTopic);
  Serial.println("\033[1;37m \033[44m" + output + " \033[0m");

  // Konfiguration mit Retain-Flag senden
  if (!client.publish(motionStateTopic.c_str(), output.c_str(), true)) { // true für retain
     Serial.println("\033[1;31mMQTT Failed to publish states!\033[0m");
  }  
}

void publishPowerState() {
  if(!client.connected()) return;

  StaticJsonDocument<30> doc; 
  String output = String(poweronoffState ? "ON" : "OFF");

  //doc["state"] = poweronoffState; 
  //doc["switch"] = poweronoffState; 

  /*if (poweronoffState) {
    output = "ON"; 
  } else {
    output = "OFF";
  }*/
  
  //output = String(poweronoffState);
  //output = "{\"" + String(poweronoffState) + "\"}";
  //serializeJson(doc, output);
  
  Serial.print("\033[1;34mMQTT Publishing State to: \033[0m");
  Serial.print(powerStateTopic);
  Serial.println("\033[1;37m \033[44m " + output + " \033[0m");

  // Konfiguration mit Retain-Flag senden  
  //if (!client.publish(powerCommandTopic.c_str(), output.c_str(), true)) { // true für retain
  //client.publish(powerCommandTopic.c_str(), output.c_str(), true);
  if (!client.publish(powerStateTopic.c_str(), output.c_str(), true)) { // true für retain
     Serial.println("\033[1;31mMQTT Failed to publish power state!\033[0m");
  }
}

// Dummy-Funktion für eingehende Nachrichten (Befehle)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\033[1;36mMQTT Message arrived\033[0m [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("\033[0;30m]\033[46m " + message + " \033[0m");

  // Hier Logik zur Befehlsverarbeitung einfügen
  // z.B. if (String(topic) == "mein_esp8266/switch/relay/command") { ... }
  if (String(topic) = powerCommandTopic) {  
    if (!DeloreanIsFlying) {
      poweronoffState = message == "ON";
      digitalWrite(poweronoff, poweronoffState);
    }
    publishPowerState();
    server.sendHeader("Location", "/",true);  
  }
}

void loop() {  
  CheckDeloreanIsFlying();
  server.handleClient();    
  matrixloop();  
  PushButton();
  mqttloop();
}

