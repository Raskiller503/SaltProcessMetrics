#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <SD.h>
// #include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <sequencer3.h>
#include <Ezo_i2c_util.h>
#include <avr/wdt.h>
// RTC setup
RTC_DS1307 rtc;
// DS18B20 setup
#define DS18B20_PIN 7
OneWire oneWire(DS18B20_PIN);
DallasTemperature DS18B20(&oneWire);
// DHT11 setup
#define DHT_PIN 2
DHT dht(DHT_PIN, DHT11);
// Anemometer setup
// const int sensorPin = A0;
// float voltageConversionConstant = .004882814;
// float voltageMin = .4;
// float windSpeedMin = 0;
// float voltageMax = 2.0;
// float windSpeedMax = 32;
// int sensorDelay = 1000;
// SD card setup
#define SD_CS_PIN 4
// EC sensor setup using SoftwareSerial
#define rx 3
#define tx 5
SoftwareSerial myserial(rx, tx);
String inputstring = "";
String sensorstring = "";
boolean input_string_complete = false;
boolean sensor_string_complete = false;
// Timing variables
unsigned long lastReadingTime = 0;
unsigned long readingInterval = 600000;
// unsigned long readingInterval = 2000;
long EC_value = 0;  // Declare and initialize EC_value only once
void setup() {
    wdt_enable(WDTO_8S);  // Enable watchdog timer with 8s timeout
    Serial.begin(9600);
    myserial.begin(9600);
    inputstring.reserve(10);
    sensorstring.reserve(30);
    DS18B20.begin();
    dht.begin();
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized.");
    // Initialize the RTC DS1307
    // if (!rtc.begin()) {
    //     Serial.println("Couldn't find RTC");
    //     while (1);
    // }
    rtc.begin();
    // rtc.adjust(DateTime(2023, 10, 5, 19, 40, 1));
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
void loop() {
    wdt_reset();  // Reset the watchdog timer
    DateTime now = rtc.now();
    //String EC_string = retrieveEC();  // Retrieve EC as a string
   // int EC_value = EC_string.toInt();  // Convert the string to an integer
    if (input_string_complete == true) {
        myserial.print(inputstring);
        myserial.print('\r');
        inputstring = "";
        input_string_complete = false;
    }
    if (myserial.available() > 0) {
        char inchar = (char)myserial.read();
        sensorstring += inchar;
        if (inchar == '\r') {
            sensor_string_complete = true;
        }
    }
    if (sensor_string_complete == true) {
    if (isdigit(sensorstring[0]) == false) {
        Serial.println(sensorstring);
    } else {
        char sensorstring_array_local[30];
        sensorstring.toCharArray(sensorstring_array_local, 30);
        long EC_value_raw = strtol(strtok(sensorstring_array_local, ","), NULL, 10);
        EC_value = EC_value_raw;
        Serial.print("EC_value: ");
        Serial.println(EC_value);
        //Serial.print("EC_value_raw: ");
        //Serial.println(EC_value_raw);
        sensorstring = "";
        sensor_string_complete = false;
    }
}
    unsigned long currentMillis = millis();
    if (currentMillis - lastReadingTime > readingInterval) {
        lastReadingTime = currentMillis;
        // DS18B20 reading
        DS18B20.requestTemperatures();
        float tempC = DS18B20.getTempCByIndex(0);
        // DHT11 readings
        float humidity = dht.readHumidity();
        float tempDHT = dht.readTemperature();
        // Anemometer reading
        Serial.print("DS18B20 Temperature: "); Serial.println(tempC);
        Serial.print("DHT11 Temperature: "); Serial.println(tempDHT);
        Serial.print("DHT11 Humidity: "); Serial.println(humidity);
        // Serial.print("Wind Speed: "); Serial.println(windSpeed);
        Serial.print("Storing EC value to SD: "); Serial.println(EC_value);       // int sensorValue = analogRead(sensorPin);
        // float sensorVoltage = sensorValue * voltageConversionConstant;
        // float windSpeed = (sensorVoltage <= voltageMin) ? 0 : (sensorVoltage - voltageMin) * windSpeedMax / (voltageMax - voltageMin);
        // Store readings to SD card
        File dataFile = SD.open("test.txt", FILE_WRITE);
        if (dataFile) {
            DateTime now = rtc.now(); // Get the current date-time
            dataFile.print("Date-Time: ");
            dataFile.print(now.year(), DEC);
            dataFile.print('/');
            dataFile.print(now.month(), DEC);
            dataFile.print('/');
            dataFile.print(now.day(), DEC);
            dataFile.print(' ');
            dataFile.print(now.hour(), DEC);
            dataFile.print(':');
            dataFile.print(now.minute(), DEC);
            dataFile.print(':');
            dataFile.println(now.second(), DEC);
            // DS18B20, DHT11, and anemometer readings
            dataFile.print("DS18B20 Temp: "); dataFile.print(tempC); dataFile.println(" C");
            dataFile.print("DHT11 Temp: "); dataFile.print(tempDHT); dataFile.println(" C");
            dataFile.print("DHT11 Humidity: "); dataFile.print(humidity); dataFile.println(" %");
            // dataFile.print("Wind Speed: "); dataFile.print(windSpeed); dataFile.println(" m/s");
            dataFile.print("EC: "); dataFile.print(EC_value); dataFile.println(" uS/cm");
            dataFile.close();
            Serial.println("Data saved to SD card.");
        } else {
            Serial.println("Failed to open data.txt for writing.");
        }
    }
}
