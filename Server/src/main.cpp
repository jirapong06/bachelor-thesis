#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define wifi_ssid "YawoottiWiFi"
#define wifi_password "0979233370"
//EACS_Soil
//jirapongg
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "fd69ffbd-2477-4c07-8a7d-1ecc56a2918f";
const char* mqtt_username = "kXbusCHtk9AvkfsMqAs3AZsUGNJYANP7";
const char* mqtt_password = "I$8ijtX3k8wUXCO1NkPd7xF#KSrwViPT";

int valve_on[4];
int valve_off[4];
int count_display = 0;
bool sending_time = false;
bool sensor[4] = {false, false, false, false};
int packetSize;

uint8_t id_sensor[4];
float batt_sensor[4];
uint8_t soil_sensor[4];
float ds_sensor[4];
float bme_t_sensor[4];
uint8_t bme_h_sensor[4];
uint16_t light_sensor[4];

int t_d[4], t_mo[4], t_y[4], t_h[4], t_m[4], t_s[4];

//------------------------------------------------------------------------------------------
#define sw1     33
#define usbwifi 4

int valve[4] = {0, 32, 25, 16};

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    433E6

#define SD_CS 23
#define SD_SCK 17
#define SD_MOSI 12
#define SD_MISO 13

//------------------------------------------------------------------------------------------

#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SPIClass sd_spi(HSPI);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

HTTPClient http;

byte device_id = 0x40;

File myFile;

RTC_DS3231 rtc;

void web_control(void);
void toggle_led(void);
bool wificonnect(void);
void reconnect(void);
bool googlesheet(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light);
bool netpie(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light);
bool save_offline(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light);
bool upload(uint8_t client_id);
void display_led(void);
void valve_control(uint8_t client_id);



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(usbwifi, OUTPUT);   digitalWrite(usbwifi, HIGH);
  pinMode(valve[1], OUTPUT);  digitalWrite(valve[1], LOW);
  pinMode(valve[2], OUTPUT);  digitalWrite(valve[2], LOW);
  pinMode(valve[3], OUTPUT);  digitalWrite(valve[3], LOW);
  delay(100);
//------------------------------------------------------------------------------------------
  Serial.begin(9600);
  Serial.println("Soil Server");
//------------------------------------------------------------------------------------------
  Wire.begin();
  OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  OLED.clearDisplay();
  OLED.setTextColor(WHITE, BLACK);
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.display();
//------------------------------------------------------------------------------------------
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sd_spi)) {
    Serial.println("SD Card failed!");
    OLED.println("SD Card Failed");
    OLED.println("Server doesn't start");
    OLED.display();
    while (1);
  }

  OLED.print("Loading config..");
  myFile = SD.open("/value.config");
  String load_conf;
  char buff;
  if (myFile) {
    Serial.println("Loading config data..");
    OLED.println(" Ok");
    OLED.display();
    while (myFile.available()) {
      buff = myFile.read();
      load_conf += buff;
    }
    myFile.close();

    int x, y;
    x = load_conf.indexOf("v1_on=") + 6;
    y = load_conf.indexOf("&v1_off");
    String valve1_on_raw = load_conf.substring(x, y);
    

    x = load_conf.indexOf("v1_off=") + 7;
    y = load_conf.indexOf("&v2_on");
    String valve1_off_raw = load_conf.substring(x, y);
    

    x = load_conf.indexOf("v2_on=") + 6;
    y = load_conf.indexOf("&v2_off");
    String valve2_on_raw = load_conf.substring(x, y);
    

    x = load_conf.indexOf("v2_off=") + 7;
    y = load_conf.indexOf("&v3_on");
    String valve2_off_raw = load_conf.substring(x, y);
    

    x = load_conf.indexOf("v3_on=") + 6;
    y = load_conf.indexOf("&v3_off");
    String valve3_on_raw = load_conf.substring(x, y);
    

    x = load_conf.indexOf("v3_off=") + 7;
    y = load_conf.indexOf("?");
    String valve3_off_raw = load_conf.substring(x, y);
    
    if ((valve1_on_raw.toInt() <= 0) || (valve1_off_raw.toInt() <= 0) || (valve2_on_raw.toInt() <= 0) || (valve2_off_raw.toInt() <= 0) || (valve3_on_raw.toInt() <= 0) || (valve3_off_raw.toInt() <= 0)) {
      OLED.println("Invalid config");
      OLED.println("Opening Setting..");
      OLED.display();
      delay(2000);
      web_control();
    } else {
      
      valve_on[1] = valve1_on_raw.toInt();
      valve_off[1] = valve1_off_raw.toInt();
      valve_on[2] = valve2_on_raw.toInt();
      valve_off[2] = valve2_off_raw.toInt();
      valve_on[3] = valve3_on_raw.toInt();
      valve_off[3] = valve3_off_raw.toInt();

      OLED.print("Valve1 On "); OLED.print(valve_on[1]);  OLED.print(" Off "); OLED.println(valve_off[1]);
      OLED.print("Valve2 On "); OLED.print(valve_on[2]);  OLED.print(" Off "); OLED.println(valve_off[2]);
      OLED.print("Valve3 On "); OLED.print(valve_on[3]);  OLED.print(" Off "); OLED.println(valve_off[3]);
      OLED.display();
    }

    Serial.println(valve_on[1]);
    Serial.println(valve_off[1]);
    Serial.println(valve_on[2]);
    Serial.println(valve_off[2]);
    Serial.println(valve_on[3]);
    Serial.println(valve_off[3]);

  } else {
    Serial.println("Error loading config data");
    OLED.println(" Error");
    OLED.println("No config.");
//    myFile = SD.open("/value.config", FILE_WRITE);
//    myFile.close();
    OLED.println("Opening Setting..");
    OLED.display();
    delay(2000);
    web_control();
  }
//------------------------------------------------------------------------------------------
  OLED.print("Lora.. ");
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    OLED.println("Fail");
    OLED.display();
    while (1);
  }
  OLED.println("Ok");
  OLED.display();
//------------------------------------------------------------------------------------------
  OLED.print("RTC.. ");
  OLED.display();
  if (rtc.begin()) {
    OLED.println("Ok");
    OLED.display();
  } else {
    OLED.println("Failed");
    OLED.display();
    while (1);
  }
//------------------------------------------------------------------------------------------
/*
  Serial.print("Connecting WiFi");
  OLED.print("Connecting WiFi.. ");
  OLED.display(); 
*/
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
/*  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  OLED.println("Ok");
  OLED.display();
*/
//------------------------------------------------------------------------------------------
  client.setServer(mqtt_server, mqtt_port);
//------------------------------------------------------------------------------------------
  delay(25000);
  digitalWrite(usbwifi, LOW);
  delay(1000);
  display_led();
}

void loop() {
  if (count_display >= 1000) {
    count_display = 0;

    DateTime now = rtc.now();
    if ((now.minute() % 5) != 0) {
      sending_time = true;
    }
    if (((now.minute() % 5) == 0) && (sending_time == true))  {
      sending_time = false;
      if ((sensor[1]) || (sensor[2]) || (sensor[3])) {
        digitalWrite(usbwifi, HIGH);
        delay(40000);
      } else {
        digitalWrite(usbwifi, LOW);
      }

      if (sensor[1]) {
        upload(1);
        sensor[1] = false;
      }
      if (sensor[2]) {
        upload(2);
        sensor[2] = false;
      }
      if (sensor[3]) {
        upload(3);
        sensor[3] = false;
      }

      delay(1000);
      digitalWrite(usbwifi, LOW);
      display_led();
    }
  } else {
    count_display++;
  }


//------------------------------------------------------------------------------------------
  digitalWrite(LED_BUILTIN, HIGH);

  if (digitalRead(33) == LOW) {
    web_control();
  }

  packetSize = LoRa.parsePacket();
  if (packetSize) {
    uint16_t buf1 = LoRa.read();
    uint16_t buf2 = LoRa.read();
    uint16_t buf3 = LoRa.read();
    uint16_t buf4 = LoRa.read();
    uint16_t buf5 = LoRa.read();
    uint16_t buf6 = LoRa.read();
    uint16_t buf7 = LoRa.read();
    uint16_t buf8 = LoRa.read();
    uint16_t buf9 = LoRa.read();
    uint16_t buf10 = LoRa.read();
    uint16_t buf11 = LoRa.read();
    uint16_t buf12 = LoRa.read();
    uint16_t buf13 = LoRa.read();
    LoRa.packetRssi();
    
    digitalWrite(LED_BUILTIN, LOW);
    uint8_t client_id = buf2;
    float v_batt = ((buf3 << 8) | buf4) / 100.0;
    uint8_t soil = buf5;
    float ds18b20_temp = ((buf6 << 8) | buf7) / 100.0;
    float bme_temp = ((buf8 << 8) | buf9) / 100.0;
    uint8_t bme_hum = buf10;
    uint16_t lux = ((buf11 << 8) | buf12);
    uint8_t sum = (buf2 + buf3 + buf4 + buf5 + buf6 + buf7 + buf8 + buf9 + buf10 + buf11 + buf12) & 0xff;
    
    if ( (buf1 == device_id) && (buf13 == sum) ) {
      /*
      Serial.println(client_id);
      Serial.println(v_batt);
      Serial.println(soil);
      Serial.println(ds18b20_temp);
      Serial.println(bme_temp);
      Serial.println(bme_hum);
      Serial.println(lux);

      OLED.clearDisplay();
      OLED.setCursor(0, 0);
      OLED.print("Recieved from "); OLED.println(client_id);
      OLED.print("soil moisture "); OLED.println(soil);
      OLED.print("soil Temp ");     OLED.println(ds18b20_temp);
      OLED.print("Air Temp ");      OLED.println(bme_temp);
      OLED.print("Air hunidity ");  OLED.println(bme_hum);
      OLED.print("Light ");         OLED.println(lux);
      OLED.display();
      */

      soil_sensor[client_id] = soil;
      ds_sensor[client_id] = ds18b20_temp;
      bme_t_sensor[client_id] = bme_temp;
      bme_h_sensor[client_id] = bme_hum;
      light_sensor[client_id] = lux;
      batt_sensor[client_id] = v_batt;
      sensor[client_id] = true;

      DateTime now = rtc.now();
      t_d[client_id] = now.day();
      t_mo[client_id] = now.month();
      t_y[client_id] = now.year();
      t_h[client_id] = now.hour();
      t_m[client_id] = now.minute();
      t_s[client_id] = now.second();

      for (int i = 0; i < 5; i++)
      {
        LoRa.beginPacket();
        LoRa.write(client_id);
        LoRa.write(device_id);
        LoRa.endPacket();
        delay(500);
      }
      
      display_led();
      valve_control(client_id);
      
    }
  }
  delay(1);
}

void web_control() {
  digitalWrite(usbwifi, HIGH);
  OLED.clearDisplay();
  OLED.setCursor(0, 0);
  OLED.println("Web Control");
  OLED.print("Connecting Wifi... ");
  OLED.display();
  delay(1000);
  wificonnect();
  WiFiServer server(80);
  if (WiFi.status() == WL_CONNECTED) {
    OLED.println("Ok");
    server.begin();
    Serial.println("Starting web control..");
    Serial.println(WiFi.localIP());

    OLED.println("");
    OLED.println("Available at");
    OLED.println(WiFi.localIP());
    OLED.display();
  } else {
    OLED.println("Fail");
    OLED.display();
  }

  bool invalid = true;
  while(1) {
    WiFiClient client = server.available();
    int buffer = client.available();
    if(buffer > 0) {
      char text[buffer];
      client.readBytes(text,buffer);
      String gettext = String(text);

      int a = gettext.indexOf("GET /")+5;
      int b = gettext.indexOf("HTTP/"); 
      String cuttext = gettext.substring(a,b);
      
      if (cuttext == " ") {
        String html = "<!DOCTYPE html>";
        html += "<html>";
        html += "<body>";
        html += "<h1>Valve Setting</h1>";
        html += "<form method=\"get\" target=\"_blank\">";
        html += "<h2>Valve1</h2>";
        html += "<label for=\"valve1_l\">Turn on : </label>";
        html += "<input type=\"text\" id=\"valve1_l\" name=\"valve1_l\"> %<br><br>";
        html += "<label for=\"valve1_h\">Turn off : </label>";
        html += "<input type=\"text\" id=\"valve1_h\" name=\"valve1_h\"> %<br><br>";
        html += "<h2>Valve2</h2>";
        html += "<label for=\"valve2_l\">Turn on : </label>";
        html += "<input type=\"text\" id=\"valve2_l\" name=\"valve2_l\"> %<br><br>";
        html += "<label for=\"valve2_h\">Turn off : </label>";
        html += "<input type=\"text\" id=\"valve2_h\" name=\"valve2_h\"> %<br><br>";
        html += "<h2>Valve3</h2>";
        html += "<label for=\"valve3_l\">Turn on : </label>";
        html += "<input type=\"text\" id=\"valve3_l\" name=\"valve3_l\"> %<br><br>";
        html += "<label for=\"valve3_h\">Turn off : </label>";
        html += "<input type=\"text\" id=\"valve3_h\" name=\"valve3_h\"> %<br><br>";
        html += "<input type=\"submit\" value=\"Submit\">";
        html += "</form>";
        html += "</body>";
        html += "</html>";
        client.println(html);
        Serial.println("Web Client Connected");

      } else if (cuttext == "favicon.ico ") {
        if (invalid != true) {
          Serial.println("Setting Saved");
          Serial.println("Restarting...");
          delay(5000);
          OLED.clearDisplay();
          OLED.display();
          ESP.restart();
        }
      } else {
        int c, b;
        invalid = false;
        c = cuttext.indexOf("valve1_l=") + 9;
        b = cuttext.indexOf("&valve1_h");
        String valve1_on_raw = cuttext.substring(c, b);
        
        c = cuttext.indexOf("valve1_h=") + 9;
        b = cuttext.indexOf("&valve2_l");
        String valve1_off_raw = cuttext.substring(c, b);

        if ((valve1_on_raw.toInt() != 0) && (valve1_off_raw.toInt() != 0) && (valve1_on_raw.toInt() < valve1_off_raw.toInt())) {
          if ((valve1_on_raw.toInt() <= 100) && (valve1_off_raw.toInt() <= 100)) {
            valve_on[1] = valve1_on_raw.toInt();
            valve_off[1] = valve1_off_raw.toInt();
          }
        } else {
          invalid = true;
        }

        c = cuttext.indexOf("valve2_l=") + 9;
        b = cuttext.indexOf("&valve2_h");
        String valve2_on_raw = cuttext.substring(c, b);

        c = cuttext.indexOf("valve2_h=") + 9;
        b = cuttext.indexOf("&valve3_l");
        String valve2_off_raw = cuttext.substring(c, b);

        if ((valve2_on_raw.toInt() != 0) && (valve2_off_raw.toInt() != 0) && (valve2_on_raw.toInt() < valve2_off_raw.toInt())) {
          if ((valve2_on_raw.toInt() <= 100) && (valve2_off_raw.toInt() <= 100)) {
            valve_on[2] = valve2_on_raw.toInt();
            valve_off[2] = valve2_off_raw.toInt();
          }
        } else {
          invalid = true;
        }        

        c = cuttext.indexOf("valve3_l=") + 9;
        b = cuttext.indexOf("&valve3_h");
        String valve3_on_raw = cuttext.substring(c, b);

        c = cuttext.indexOf("valve3_h=") + 9;
        b = cuttext.length();
        String valve3_off_raw = cuttext.substring(c, b);

        if ((valve3_on_raw.toInt() != 0) && (valve3_off_raw.toInt() != 0) && (valve3_on_raw.toInt() < valve3_off_raw.toInt())) {
          if ((valve3_on_raw.toInt() <= 100) && (valve3_off_raw.toInt() <= 100)) {
            valve_on[3] = valve3_on_raw.toInt();
            valve_off[3] = valve3_off_raw.toInt();
          }
        } else {
          invalid = true;
        }

        String summary; 
        
        Serial.println(valve_on[1]);
        Serial.println(valve_off[1]);
        Serial.println(valve_on[2]);
        Serial.println(valve_off[2]);
        Serial.println(valve_on[3]);
        Serial.println(valve_off[3]);

        if (invalid != true) {
          String sd_conf = "v1_on=" + String(valve_on[1]) + "&v1_off=" + String(valve_off[1]) + "&v2_on=" + String(valve_on[2]) + "&v2_off=" + String(valve_off[2]) + "&v3_on=" + String(valve_on[3]) + "&v3_off=" + String(valve_off[3]) + "?";
          for (int i = 0; i < 5; i++)
          {
            myFile = SD.open("/value.config", FILE_WRITE);
            if (myFile) {
              Serial.print("Saving setting to sd..... ");
              myFile.print(sd_conf);
              myFile.close();
              Serial.println("done");
              OLED.println("Setting Saved");
              OLED.display();
              break;
            } else {
              Serial.println("error");
              OLED.println("Save Setting Fail");
              OLED.println("Try again");
              OLED.display();
            }
            delay(1000);
          }
          
          summary = "<!DOCTYPE html>";
          summary += "<html>";
          summary += "<body>";
          summary += "<h2>Setting Complete.</h2>";
          summary += "<h3>Valve1</h3>";
          summary += "Turn on : " + String(valve_on[1]) + " %<br>";
          summary += "Turn off : " + String(valve_off[1]) + " %<br>";
          summary += "<h3>Valve2</h3>";
          summary += "Turn on : " + String(valve_on[2]) + " %<br>";
          summary += "Turn off : " + String(valve_off[2]) + " %<br>";
          summary += "<h3>Valve3</h3>";
          summary += "Turn on : " + String(valve_on[3]) + " %<br>";
          summary += "Turn off : " + String(valve_off[3]) + " %<br>";
          summary += "</body>";
          summary += "</html>";
        } else {
          summary = "<!DOCTYPE html>";
          summary += "<html>";
          summary += "<body>";
          summary += "<h2>Error! some value setting is incorrect</h2>";
          summary += "Plaes back to Valve setting page and try again";
          summary += "</body>";
          summary += "</html>";
        }
        client.println(summary);
      }
      client.flush();
    }
    delay(100);
  }

}

void toggle_led() {
  if(digitalRead(LED_BUILTIN) == HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

bool wificonnect() {
  bool connect_status = false;
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.println("Connecting WIFI");
    delay(500);
    for (int i = 0; i < 30; i++)
    {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
        toggle_led();
        connect_status = false;
      } else {
        Serial.println("Connected");
        digitalWrite(LED_BUILTIN, LOW);
        connect_status = true;
        break;
      }
    }

    digitalWrite(LED_BUILTIN, LOW);
  } else {
    connect_status = true;
  }
  return connect_status;
}

void reconnect() {
  for (int i = 0; i < 5; i++)
  {
    Serial.print("Attempting MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      break;
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
    }
    delay(500);
  }
}

bool googlesheet(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light) {
  wificonnect();
  Serial.println("Send to table");
  String url;
  switch (id)
  {
    case 1:
      url = "https://script.google.com/macros/s/AKfycbwAVdwMCPKHna5pxSdYpC928alJpkA2bUKfVgsnTLMYf32edrqM/exec?";
      break;
    case 2:
      url = "https://script.google.com/macros/s/AKfycbzhULOaVX-C_w17N7euiDPeqi_Lf_CnC5peJUpJdjDGPtFDPAg/exec?";
      break;
    case 3:
      url = "https://script.google.com/macros/s/AKfycbwJ8TKgAKGP5tItbUQ1CQElQi6zf6L3AXnyeq_0lhBzVmLDNya1/exec?";
      break;
    default:
      break;
  }
  
  String path = url + "soil=" + soil + "&18b20_t=" + ds + "&bme_t=" + bme_t + "&bme_h=" + bme_h + "&light=" + light; // + "&batt=" + batt;
  Serial.println(path);
  int httpCode = 0;
  bool send_status = false;
  for (int i = 0; i < 5; i++)
  {
    http.begin(path); //HTTP       
    httpCode = http.GET();       
    if (httpCode == 200) {
      String content = http.getString();
      Serial.print("Respond --- ");
      Serial.println(content);
      send_status = true;
      break;
    } else if (httpCode <= 0) {
      Serial.println("Can't access internet " + String(httpCode));
      send_status = false;
    } else {
      Serial.println("Fail. error code " + String(httpCode));
      send_status = true;
      break;
    }
    delay(500);
  }
  return send_status;
}

bool netpie(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light) {
  bool send_status = false;
  wificonnect();
  Serial.println("Send to netpie");
  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()) {
    client.loop();
    char msg[150];
    String data = "{\"data\": {\"soil" + String(id) + "\":" + String(soil) + ", \"ds18b20_t" + String(id) + "\":" + String(ds) + ", \"bme_t" + String(id) + "\":" + String(bme_t) + ", \"bme_h" + String(id) + "\":" + String(bme_h) + ", \"light" + String(id) + "\":" + String(light) + ", \"batt" + String(id) + "\":" + String(batt) + "}}";
    Serial.println(data);
    data.toCharArray(msg, (data.length() + 1));
    client.publish("@shadow/data/update", msg);
    send_status = true;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
  }
  return send_status;
}

bool save_offline(uint8_t id, float batt, uint8_t soil, float ds, float bme_t, uint8_t bme_h, uint16_t light) {
  String write_sd = String(t_d[id]) + "/" + String(t_mo[id]) + "/" + String(t_y[id]) + "," + String(t_h[id]) + ":" + String(t_mo[id]) + ":" + String(t_s[id]) + "," + String(soil) + "," + String(ds) + "," + String(bme_t) + "," + String(bme_h) + "," + String(light);
  String logfile;

  switch (id) {
    case 1:
      logfile = "/Sensor1_log.csv";
      break;
    case 2:
      logfile = "/Sensor2_log.csv";
      break;
    case 3:
      logfile = "/Sensor3_log.csv";
      break;
    default:
      break;
  }

  bool send_status = false;
  for (int i = 0; i < 5; i++)
  {
    myFile = SD.open(logfile, FILE_APPEND);
    if (myFile) {
      Serial.print("write to sd..... ");
      myFile.println(write_sd);
      myFile.close();
      Serial.println("done");
      Serial.println(write_sd);
      send_status = true;
      break;
    } else {
      Serial.println("error");
    }
    delay(1000);
  }
  return send_status;
}

bool upload(uint8_t client_id) {
  bool send_status = false;
  OLED.clearDisplay();
  OLED.setCursor(0, 0);
  OLED.print("Saving Sensor "); OLED.println(client_id);
  OLED.print("sheet.. ");
  OLED.display();
  if (googlesheet(client_id, batt_sensor[client_id] , soil_sensor[client_id] , ds_sensor[client_id] , bme_t_sensor[client_id] , bme_h_sensor[client_id] , light_sensor[client_id])) {
    OLED.println("Ok");
    send_status = true;
  } else {
    OLED.println("Failed");
  }
  OLED.display();
//------------------------------------------------------------------------------------------
  OLED.print("netpie.. ");
  OLED.display();
  if (netpie(client_id, batt_sensor[client_id] , soil_sensor[client_id] , ds_sensor[client_id] , bme_t_sensor[client_id] , bme_h_sensor[client_id] , light_sensor[client_id])) {
    OLED.println("Ok");
    send_status = true;
  } else {
    OLED.println("Failed");
  }
  OLED.display();
//------------------------------------------------------------------------------------------
  OLED.print("sd card.. ");
  OLED.display();
  if (save_offline(client_id, batt_sensor[client_id] , soil_sensor[client_id] , ds_sensor[client_id] , bme_t_sensor[client_id] , bme_h_sensor[client_id] , light_sensor[client_id])) {
    OLED.println("Ok");
    send_status = true;
  } else {
    OLED.println("Failed");
  }
  OLED.display();
  delay(5000);

  return send_status;
}

void display_led() {
  OLED.clearDisplay();
  OLED.setCursor(0, 0);
  OLED.println("Recieved Data");
  OLED.print("Sensor1.. ");
  if (sensor[1]) {
    OLED.println("yes");
  } else {
    OLED.println("no");
  }
  OLED.print("Sensor2.. ");
  if (sensor[2]) {
    OLED.println("yes");
  } else {
    OLED.println("no");
  }
  OLED.print("Sensor3.. ");
  if (sensor[3]) {
    OLED.println("yes");
  } else {
    OLED.println("no");
  }
  DateTime now = rtc.now();
  OLED.print("Last update "); OLED.print(now.hour()); OLED.print(":");  OLED.print(now.minute()); OLED.print(":");  OLED.println(now.second()); 
  OLED.display();
}

void valve_control(uint8_t client_id) {
  
  if (soil_sensor[client_id] <= valve_on[client_id]) {
    digitalWrite(valve[client_id], HIGH);
  }
  if (soil_sensor[client_id] >= valve_off[client_id]) {
    digitalWrite(valve[client_id], LOW);
  }

}