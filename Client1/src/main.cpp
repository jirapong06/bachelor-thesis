#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ONE_WIRE_PIN1 33
#define ONE_WIRE_PIN2 25
#define en_circuit 32
#define SEALEVELPRESSURE_HPA (1013.25)
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    433E6
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define v_batt_pin 35
Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

byte device_id = 0x03;
byte server_id = 0x40;
Adafruit_ADS1115 ads(0x48);
OneWire oneWire1(ONE_WIRE_PIN1);
OneWire oneWire2(ONE_WIRE_PIN2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
Adafruit_BME280 bme;
BH1750 lightMeter(0x23);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  int v_batt_read = analogRead(v_batt_pin);
  float v_batt = ((v_batt_read * 3.3) / 4095.0) * 2;
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(en_circuit, OUTPUT);
  digitalWrite(en_circuit, HIGH);
  delay(5000);
  
  Wire.begin();
  OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  OLED.clearDisplay();
  OLED.setTextColor(WHITE, BLACK);
  OLED.setTextSize(1);
  OLED.setCursor(0, 0);
  OLED.display();

  Wire.beginTransmission (0x48);
  if (Wire.endTransmission () == 0) {
    ads.begin();
  } else {
    Serial.println("ADS... Failed");
    OLED.println("ADS... Failed");
    OLED.display();
    delay(2000);
  }

  sensors1.begin();
  sensors2.begin();

  Wire.beginTransmission (0x76);
  if (Wire.endTransmission () == 0) {
    bme.begin(0x76);
  } else {
    Serial.println("BME... Failed");
    OLED.println("BME... Failed");
    OLED.display();
    delay(2000);
  }

  Wire.beginTransmission (0x23);
  if (Wire.endTransmission () == 0) {
    lightMeter.begin();
  } else {
    Serial.println("BH1750... Failed");
    OLED.println("BH1750... Failed");
    OLED.display();
    delay(2000);
  }

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa... Failed");
    OLED.println("LoRa... Failed");
    OLED.display();
    delay(2000);
  }
  
  esp_sleep_enable_timer_wakeup(600*1000*1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(en_circuit, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
  OLED.clearDisplay();
  OLED.setCursor(0, 0);
  OLED.display();
  //-----------------------------------------------------------------
  float v_batt = ((analogRead(v_batt_pin) * 3.3) / 4095.0);
  uint16_t v_batt_int = v_batt * 100; 
  uint8_t v_batt_h = v_batt_int >> 8;
  uint8_t v_batt_l = v_batt_int & 0xff;
  //Serial.print("V_Batt "); Serial.println(v_batt);
  //OLED.print("V_Batt "); OLED.println(v_batt);
  //OLED.display();
  delay(100);
  //-----------------------------------------------------------------
  uint32_t adc0, adc1, adc2, adc3, adc;
  float vcc, v1, v2, v3;

  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);

  for (int i = 0; i < 5; i++)
  {
    adc0 += ads.readADC_SingleEnded(0);
    adc1 += ads.readADC_SingleEnded(1);
    adc2 += ads.readADC_SingleEnded(2);
    delay(500);
  }
  adc0 = adc0 / 5;
  adc1 = adc1 / 5;
  adc2 = adc2 / 5;

  vcc = (adc0 * 0.1875) / 1000.0;
  v1 = (adc1 * 0.1875) / 1000.0;
  v2 = (adc2 * 0.1875) / 1000.0;
  v3 = (adc3 * 0.1875) / 1000.0;

  uint8_t soil1 = map(adc1, 0, adc0, 100, 0);
  uint8_t soil2 = map(adc2, 0, adc0, 100, 0);

  uint8_t soil_average = soil1;
  Serial.print("Vcc : "); Serial.println(vcc);
  Serial.print("Moisture1 : "); Serial.println(soil1);
  Serial.print("Moisture2 : "); Serial.println(soil2);
  OLED.print("Soil "); OLED.println(soil1); //OLED.print(", Soil2 "); OLED.println(soil2); 
  OLED.display();
  delay(100);
 //-----------------------------------------------------------------
  
  float temper, ds18b20_temp = 0;

  for (int i = 0; i < 5; i++)
  {
    sensors1.requestTemperatures();
    temper = sensors1.getTempCByIndex(0);
    if (temper < 0) {
      temper = 0;
    }  
    if (temper > 100) {
      temper = 100;
    }

    ds18b20_temp += temper;
    delay(200);
  }
  ds18b20_temp = ds18b20_temp / 5;

  uint16_t ds18b20_temp_int = ds18b20_temp * 100; 
  uint8_t ds18b20_temp_h = ds18b20_temp_int >> 8;
  uint8_t ds18b20_temp_l = ds18b20_temp_int & 0xff;
  Serial.print("18B20 T : "); Serial.println(ds18b20_temp);
  OLED.print("ds18b20 "); OLED.println(ds18b20_temp);
  OLED.display();
  delay(100);
  //-----------------------------------------------------------------
  float temperbme, bme_temp = 0;

  for (int i = 0; i < 5; i++)
  {
    temperbme = bme.readTemperature();
    if (temperbme < 0) {
      temperbme = 0;
    }  
    if (temperbme > 100) {
      temperbme = 100;
    }

    bme_temp += temperbme;
    delay(100);
  }
  bme_temp = bme_temp / 5;
  
  uint16_t bme_temp_int = bme_temp * 100;
  uint8_t bme_temp_h = bme_temp_int >> 8;
  uint8_t bme_temp_l = bme_temp_int & 0xff;

  uint8_t humerbme, bme_hum = 0;
  for (int i = 0; i < 5; i++)
  {
    humerbme = bme.readHumidity();
    if (humerbme < 0) {
      humerbme = 0;
    }  
    if (humerbme > 100) {
      humerbme = 100;
    }

    bme_hum += humerbme;
    delay(100);
  }
  bme_hum = bme_hum / 5;

  Serial.print("BME T : "); Serial.println(bme_temp);
  Serial.print("BME H : "); Serial.println(bme_hum);
  OLED.print("BME T "); OLED.print(bme_temp); OLED.print(", H "); OLED.println(bme_hum);
  OLED.display();
  delay(100);
  //-----------------------------------------------------------------
  float luxer, lux = 0;
  for (int i = 0; i < 5; i++)
  {
    luxer = lightMeter.readLightLevel();
    delay(200);
  }
  for (int i = 0; i < 5; i++)
  {
    luxer = lightMeter.readLightLevel();
    lux += luxer;
    delay(200);
  }
  lux = lux / 5;
  
  uint16_t lux_int = lux;
  uint8_t lux_h = lux_int >> 8;
  uint8_t lux_l = lux_int & 0xff;
  Serial.print("Light : "); Serial.println(lux);
  OLED.print("Light "); OLED.println(lux);
  OLED.display();
  delay(100);
  //-----------------------------------------------------------------
  uint16_t chksum_raw = (device_id + v_batt_h + v_batt_l + soil_average + ds18b20_temp_h + ds18b20_temp_l + bme_temp_h + bme_temp_l + bme_hum + lux_h + lux_l);
  uint8_t chksum = chksum_raw & 0xff;
  //-----------------------------------------------------------------
  OLED.print("Sending");  OLED.display();
  bool send_ok = false;
  for (int i = 0; i < 5; i++)
  {
    /* code */
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);

    bool lora_available = false;
    while (lora_available == false) {
      for (int i = 0; i < 5; i++)
      {
        int gateway_busy = LoRa.parsePacket();
        if(gateway_busy) {
          while (LoRa.available()) {
            Serial.print((char)LoRa.read());
          }
          lora_available = false;
        } else {
          lora_available = true;
        }
      }
    }

    OLED.print(".");  OLED.display();
    LoRa.beginPacket();
    LoRa.write(server_id);
    LoRa.write(device_id);
    LoRa.write(v_batt_h);
    LoRa.write(v_batt_l);
    LoRa.write(soil_average);
    LoRa.write(ds18b20_temp_h);
    LoRa.write(ds18b20_temp_l);
    LoRa.write(bme_temp_h);
    LoRa.write(bme_temp_l);
    LoRa.write(bme_hum);
    LoRa.write(lux_h);
    LoRa.write(lux_l);
    LoRa.write(chksum);
    LoRa.endPacket();

    for (uint16_t j = 0; j < 65534; j++)
    {
      /* code */
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        uint16_t buf1 = LoRa.read();
        uint16_t buf2 = LoRa.read();

        if (buf1 == device_id) {
          if (buf2 == server_id) {
            send_ok = true;
            break;
          } else
          {
            send_ok = false;
          }
        } else
        {
          send_ok = false;
        }
      }
    }

    if (send_ok == true) {
      OLED.println("Ok");  OLED.display();
      break;
    }
    delay(500);
  }

  if (send_ok == false) {
    OLED.println("Fail");  OLED.display();
    delay(1000);
    esp_sleep_enable_timer_wakeup(300*1000*1000);
    delay(100);
    esp_deep_sleep_start();
  }
  
  
  //-----------------------------------------------------------------
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  //-----------------------------------------------------------------
/*
  LoRa.beginPacket();
  LoRa.print("Vcc : ");
  LoRa.print(vcc);
  LoRa.print(", Moisture : ");
  LoRa.print(soil);
  LoRa.print(", 18B20_T : ");
  LoRa.print(sensors.getTempCByIndex(0));
  LoRa.print(", BME_T : ");
  LoRa.print(bme.readTemperature());
  LoRa.print(", BME_H : ");
  LoRa.print(bme.readHumidity());
  LoRa.print(", Light : ");https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues/6

  LoRa.print(lux);
  LoRa.endPacket();
*/
  delay(4000);
  OLED.clearDisplay();
  OLED.display();
  LoRa.sleep();
  pinMode(RST, INPUT);
  digitalWrite(en_circuit, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  esp_deep_sleep_start();


}