#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    433E6

#define wifi_ssid "RIMKLONGNEW2_2.4G"
#define wifi_password "9988776655"

HTTPClient http;

int packetSize;
String url = "https://script.google.com/macros/s/AKfycbyiorp5gHw9fOnk--H9UTZLW39PMuEBes6XqRjQ5WHBnOtjaoRg/exec";
int httpCode;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver");

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

void loop() {
  // try to parse packet
  packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((byte)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());

    http.begin(url);
    httpCode = http.GET();
    digitalWrite(LED_BUILTIN, LOW);
    delay(20000);
  }
  
}
