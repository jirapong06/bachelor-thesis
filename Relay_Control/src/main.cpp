#include <Arduino.h>

int time1, time2, time3;

#define in1 5
#define in2 6
#define in3 7

#define valve1 A0
#define valve2 A1
#define valve3 A2
#define pump A3

void setup() {

  pinMode(valve1, OUTPUT);  digitalWrite(valve1, HIGH);
  pinMode(valve2, OUTPUT);  digitalWrite(valve2, HIGH);
  pinMode(valve3, OUTPUT);  digitalWrite(valve3, HIGH);
  pinMode(pump, OUTPUT);    digitalWrite(pump, HIGH);

  pinMode(in1, INPUT);
  pinMode(in2, INPUT);
  pinMode(in3, INPUT);

  delay(5000);
}

void loop() {

  if (digitalRead(in1) == HIGH) {
    if (time1 < 600) {
      time1++;
    } else
    {
      time1 = 0;
    }

    if (time1 < 60) {
      digitalWrite(valve1, LOW);
    } else {
      digitalWrite(valve1, HIGH);
    }
  } else {
    time1 = 0;
    digitalWrite(valve1, HIGH);
  }
  //--------------------------------------------------------------------------  
  if (digitalRead(in2) == HIGH) {
    if (time2 < 600) {
      time2++;
    } else
    {
      time2 = 0;
    }

    if (time2 < 60) {
      digitalWrite(valve2, LOW);
    } else {
      digitalWrite(valve2, HIGH);
    }
  } else {
    time2 = 0;
    digitalWrite(valve2, HIGH);
  }
  //--------------------------------------------------------------------------  
  if (digitalRead(in3) == HIGH) {
    if (time3 < 600) {
      time3++;
    } else
    {
      time3 = 0;
    }

    if (time3 < 60) {
      digitalWrite(valve3, LOW);
    } else {
      digitalWrite(valve3, HIGH);
    }
  } else {
    time3 = 0;
    digitalWrite(valve3, HIGH);
  }
  //--------------------------------------------------------------------------
  if ((!digitalRead(valve1)) || (!digitalRead(valve2)) || (!digitalRead(valve3))) {
    digitalWrite(pump, LOW);
  } else
  {
    digitalWrite(pump, HIGH);
  }
  //--------------------------------------------------------------------------
  delay(1000);

}