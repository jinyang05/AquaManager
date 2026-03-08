#include <OneWire.h>
#include <Stepper.h>
#include <LiquidCrystal.h>

// lcd and joystick
LiquidCrystal lcd(13, 12, 6, 5, 3, A2);
bool started = false;

int joyX = A5;
int joyY = A4;
int joyButton = 2;

int tempThreshold = 30;
int cleanThreshold = 50;


/* water pump motors */
#define motor1A A3 // blue wire
#define motor2A 7 // white wire

/* water sensor level */
#define signal_pin A0


int tds_meter = A1;
int DS18S20_Pin = 4; 
int pin1 = 8, pin2 = 10, pin3 = 9, pin4 = 11; /* insert pin */
bool waterIsClean = false;
bool waterIsCold = false;
int water_sensor = 0; /* water sensor value */

int steps_per_revolution = 2048;
int steps_degree = 682;
Stepper motor(steps_per_revolution, pin1, pin2, pin3, pin4);



// Temperature chip i/o
OneWire ds(DS18S20_Pin);


void setup(){
  //joystick and LCD
  pinMode(joyButton, INPUT_PULLUP);
  lcd.begin(16,2);

  motor.setSpeed(10); // motor speed
  // pinMode(power_pin, OUTPUT); // water sensor
  // digitalWrite(power_pin, LOW); // water sensor
  pinMode(motor1A, OUTPUT); // pump
  pinMode(motor2A, OUTPUT); // pump
  Serial.begin(9600);

  while (!started) {

    int x = analogRead(joyX);
    int y = analogRead(joyY);

    if (x < 300) {
      cleanThreshold--;
      delay(200);
    }

    if (x > 700) {
      cleanThreshold++;
      delay(200);
    }

    if (y < 300) {
      tempThreshold--;
      delay(200);
    }

    if (y > 700) {
      tempThreshold++;
      delay(200);
    }

    lcd.setCursor(0,0);
    lcd.print("Temp:");
    lcd.print(tempThreshold);
    lcd.print("   ");

    lcd.setCursor(0,1);
    lcd.print("Clean:");
    lcd.print(cleanThreshold);
    lcd.print("   ");

    if (digitalRead(joyButton) == LOW) {
      started = true;
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("START");
      delay(2000);
    }

  }
    lcd.setCursor(0,0);
    lcd.print("Trial Running");



  delay(2000);

  // check if water is dirty
  waterIsClean = isClean(cleanThreshold);
  // check water temp (30 and under is cold)
  waterIsCold = isCold(tempThreshold);

  
  // get motor ready
  if (waterIsClean) {
    motor.step(steps_degree);
    // water is clean and can be reused to clean
  } else if(waterIsCold) {
    motor.step(steps_degree * 2);
    // water is cold and can be used to cool
  }else {
    // water is dirty and too hot 
    // do nothing?
  }

}

void loop(){

  water_sensor = analogRead(signal_pin);
  Serial.println(water_sensor);
  delay(1000);
  while (water_sensor > 200) {
    water_sensor = analogRead(signal_pin);
    digitalWrite(motor1A, HIGH);
    digitalWrite(motor2A, LOW);
  }
  digitalWrite(motor1A, LOW);
  digitalWrite(motor2A, LOW);

  delay(5000);

  // reset motor back
  if (waterIsClean) {
    motor.step(-steps_degree);
    // water is clean and can be reused to clean
  } else if(waterIsCold) {
    motor.step(-1* steps_degree * 2);
    // water is cold and can be used to cool
  }else {
    // water is dirty and too hot 
    // do nothing?
  }
  
  Serial.println("Exiting");
  delay(1000);
  exit(0);

}


bool isCold(int tempThreshold){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (TemperatureSum > tempThreshold) {
    Serial.println("it is hot");
    return false;
  } else {
    Serial.println("it is cold");
    return true;
  }

}


float isClean(int cleanThreshold) {
  int raw = analogRead(tds_meter);

  // convert voltage
  float voltage = raw * 5.0 / 1024.0;

  // simple TDS approximation
  float tds = voltage * 500;

  if (tds < cleanThreshold) {
    Serial.println("it is clean!");
    return true;
  } else {
    Serial.println("it's dirty");
    return false;
  }

}


bool isEmpty() {
  delay(10);
  water_sensor = analogRead(signal_pin);
  while (water_sensor < 290 && water_sensor > 130) {
    water_sensor = analogRead(signal_pin);
    Serial.println(water_sensor);
    delay(1000);
  }
  Serial.println(water_sensor);

  if (water_sensor < 130) return true; // it's empty
  else return false; // it's full

}

