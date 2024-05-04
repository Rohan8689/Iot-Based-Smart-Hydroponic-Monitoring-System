#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <SoftwareSerial.h>
#define ph_sensor A0
#define DHTPIN 7
#define TdsSensorPin A1
#define DHTTYPE DHT11
LiquidCrystal_I2C lcd1(0x26, 16, 2);  // I2C address 0x27 (from DIYables LCD), 16 column and 2 rows
LiquidCrystal_I2C lcd2(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
#include <L298N.h>
#define PH_UP_MOTOR_IN1 2
#define PH_UP_MOTOR_IN2 3
#define motor1EnablePin 9 // PWM Pin
#define PH_DOWN_MOTOR_IN1 5
#define PH_DOWN_MOTOR_IN2 6
#define motor2EnablePin 10 // PWM Pin
#define MOISTURE_PIN A3 
#define THRESHOLD 530   // => CHANGE YOUR THRESHOLD HERE
const int relayPin1 = 22;
const int relayPin2 = 24;
const int relayPin3 = 26;
const int relayPin4 = 28;
const int relayPin5 = 36;

SoftwareSerial espSerial(18, 19); // TX, RX

L298N pHUpMotor(PH_UP_MOTOR_IN1, PH_UP_MOTOR_IN2); // Create instances of L298N for pH up pump
L298N pHDownMotor(PH_DOWN_MOTOR_IN1, PH_DOWN_MOTOR_IN2); // Create instances of L298N for pH down pump
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;
float ph_value,sensor_value=0;

const float PH_LOWER_LIMIT = 5.5;
const float PH_UPPER_LIMIT = 6.5;
const float PH_TOLERANCE = 0.5; // pH tolerance range

void setup()
{
  dht.begin();  
  Serial.begin(115200);
  espSerial.begin(115200);
  pinMode(TdsSensorPin,INPUT);
  lcd1.init(); // initialize the lcd1
  lcd1.backlight();
  lcd2.init(); // initialize the lcd2
  lcd2.backlight();

  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  pinMode(relayPin5, OUTPUT);

  pinMode(PH_UP_MOTOR_IN1, OUTPUT);
  pinMode(PH_UP_MOTOR_IN2, OUTPUT);
  pinMode(motor1EnablePin, OUTPUT);
  pinMode(PH_DOWN_MOTOR_IN1, OUTPUT);
  pinMode(PH_DOWN_MOTOR_IN2, OUTPUT);
  pinMode(motor2EnablePin, OUTPUT);
}

void loop()
{

  {
    delay(15000); // wait a few seconds between measurements

    float humi  = dht.readHumidity();    // read humidity
    float tempC = dht.readTemperature(); // read temperature

    lcd1.clear();
    if (isnan(humi) || isnan(tempC)) {
      lcd1.setCursor(0, 0);
      lcd1.print("Failed");
      return;
    }
    lcd1.setCursor(0, 0);  // start to print at the first row
    lcd1.print("Temp: ");
    lcd1.print(tempC);     // print the temperature
    lcd1.print((char)223); // print ° character
    lcd1.print("C");

    lcd1.setCursor(0, 1);  // start to print at the second row
    lcd1.print("Humi: ");
    lcd1.print(humi);      // print the humidity
    lcd1.print("%");
    espSerial.print(tempC); // Send temperature to ESP8266
    espSerial.print(",");
    espSerial.print(humi); // Send temperature to ESP8266
    espSerial.println();
    delay(5000); // Send data every 10 seconds (adjust as needed)
    if (tempC > 25) {
      digitalWrite(relayPin1, LOW); // Turn on the relay
      Serial.println("Fan turned ON");
    } else {
      digitalWrite(relayPin1, HIGH); // Turn off the relay
      Serial.println("Fan turned OFF");
    }

    if (humi < 60) {
      digitalWrite(relayPin2, LOW); // Turn on the relay
      Serial.println("Humidifier turned ON");
    } else {
      digitalWrite(relayPin2, HIGH); // Turn off the relay
      Serial.println("Humidifier turned OFF");
    }
  }

  {
  //Turn on pump
  digitalWrite(relayPin4, LOW);
  delay(30000);
  digitalWrite(relayPin4, HIGH);
  delay(10000);
  //Turn on light
  digitalWrite(relayPin3, LOW);
  delay(50000);
  digitalWrite(relayPin3, HIGH);
  delay(20000);
  }

  {
    int value = analogRead(MOISTURE_PIN); // read the analog value from sensor
    if (value > THRESHOLD) {
      digitalWrite(relayPin5, HIGH);
    } else {
      digitalWrite(relayPin5, LOW);
    }
    delay(7000);
  }

  {
    ph_value=analogRead(ph_sensor);
    sensor_value=(ph_value-200)/35;
    lcd2.setCursor(0, 0);
    lcd2.print("PH Value =");
    lcd2.setCursor(11, 0);
    lcd2.print(sensor_value);
    delay(5000);
    
    if (sensor_value < (PH_LOWER_LIMIT - PH_TOLERANCE)) 
    {
      digitalWrite(PH_UP_MOTOR_IN1, HIGH);
      digitalWrite(PH_UP_MOTOR_IN2, LOW);
      analogWrite(motor1EnablePin, 150);
      delay(500); // Adjust delay according to required dosage time
      pHUpMotor.stop(); // Stop motor
    } 
    else if (sensor_value > (PH_UPPER_LIMIT + PH_TOLERANCE))
    {
      digitalWrite(PH_DOWN_MOTOR_IN1, HIGH);
      digitalWrite(PH_DOWN_MOTOR_IN2, LOW);
      analogWrite(motor2EnablePin, 150);
      delay(500); // Adjust delay according to required dosage time
      pHDownMotor.stop(); // Stop motor
    }
    delay(10000); // Delay between pH checks

  }
}