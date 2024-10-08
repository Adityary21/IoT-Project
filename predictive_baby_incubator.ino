#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_MLX90614.h>
#include <math.h>
#include <RTClib.h>

// ZMPT101B sensor pin
#define ZMPT101B_PIN A0 

// WiFi credentials
const char* ssid = "wifi";
const char* password = "kucin123";

// Google Apps Script endpoint
const char* host = "script.google.com";
const int httpsPort = 443;
const char* googleScriptId = "AKfycbzaPuB7GuMhKg0i9idfXV6S8e03FhJfuZVM50COGOgIfPMcbL3-455NkpV6sA9-tCou";

// SHA1 fingerprint of the certificate
const char* fingerprint = "5A:DA:6A:A7:18:DA:E0:89:56:E6:D0:10:8F:43:AA:03:9F:70:8F:BF";

// Create instances
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize for 16x2 LCD
WiFiClientSecure client;
RTC_DS1307 rtc;

// Variables for RMS calculation
int decimalPrecision = 2; 
float voltageSampleRead  = 0;
float voltageLastSample  = 0;
float voltageSampleSum   = 0;
float voltageSampleCount = 0;
float voltageMean;
float RMSVoltageMean;
float FinalRMSVoltage; //

// AC Voltage Offset
float voltageOffset1 = 0.00;
float voltageOffset2 = 0.00;

// Variables for display control
unsigned long previousMillis = 0;
const long interval = 2000; 
int displayState = 0;

void setup() {
    Serial.begin(9600);

    // Initialize the LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Monitoring");
    lcd.setCursor(0, 1);
    lcd.print("Suhu & Voltage");
    
    // Connect to WiFi
    Serial.println();
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print("IP:");
    lcd.print(WiFi.localIP());
    delay(2000);
    lcd.clear();

    // Initialize MLX90614 sensor
    if (!mlx.begin()) {
        Serial.println("Error connecting to MLX90614 sensor. Check wiring.");
        while (1);
    }

    // Initialize RTC DS1307
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (!rtc.isrunning()) {
        Serial.println("RTC is NOT running!");
    }

    // Start timer by setting RTC to 0
    rtc.adjust(DateTime(2000, 1, 1, 0, 0, 0));
}

void loop() {
    DateTime now = rtc.now();
    TimeSpan elapsed = now - DateTime(2000, 1, 1, 0, 0, 0); 
    String elapsedTime = String(elapsed.hours()) + ":" + String(elapsed.minutes()) + ":" + String(elapsed.seconds());

    // Read temperature from MLX90614
    float ambientTemp = mlx.readAmbientTempC();
    float objectTemp = mlx.readObjectTempC();

    int Temperature = (int)objectTemp;

    // Read voltage from ZMPT101B
    readVoltage();

    // Determine status based on temperature and voltage
    int tempStatus = getStatus(Temperature, 33, 35);
    int voltStatus = getStatus(FinalRMSVoltage, 200, 230);

    if (isnan(objectTemp)) {
        Serial.println("Failed to read from MLX90614 sensor!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error");
    } else {
        Serial.print("Elapsed Time: ");
        Serial.print(elapsedTime);
        Serial.print("\tTemperature: ");
        Serial.print(Temperature);
        Serial.print(" °C\tVoltage: ");
        Serial.print(FinalRMSVoltage);
        Serial.print(" V\tTemp Status: ");
        Serial.print(tempStatus);
        Serial.print("\tVolt Status: ");
        Serial.println(voltStatus);

        sendData(elapsedTime, Temperature, FinalRMSVoltage, tempStatus, voltStatus);
        
        // Update display based on the interval
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            displayState = (displayState + 1) % 2; 

            lcd.clear();
            if (displayState == 0) {
                lcd.setCursor(0, 0);
                lcd.print("Elapsed Time:");
                lcd.setCursor(0, 1);
                lcd.print(elapsedTime);
            } else {
                lcd.setCursor(0, 0);
                lcd.print("T:");
                lcd.print(Temperature);
                lcd.print("C V:");
                lcd.print(FinalRMSVoltage, decimalPrecision);
                lcd.setCursor(0, 1);
                lcd.print("TS:");
                lcd.print(tempStatus);
                lcd.print(" VS:");
                lcd.print(voltStatus);
            }
        }
    }

    delay(500); // Shorter delay to allow for more frequent display updates
}

void readVoltage() {
    for (int i = 0; i < 1000; i++) {
        voltageSampleRead = (analogRead(ZMPT101B_PIN) - 512) + voltageOffset1; 
        voltageSampleSum += sq(voltageSampleRead); 
        delayMicroseconds(1000); 
    }

    voltageMean = voltageSampleSum / 1000;
    RMSVoltageMean = (sqrt(voltageMean)) * 1.5;
    FinalRMSVoltage = RMSVoltageMean + voltageOffset2;

    if (RMSVoltageMean <= 75.00) { 
        FinalRMSVoltage = 0.00;
    }

    voltageSampleSum = 0; 
}

int getStatus(float value, float min, float max) {
    if (value < min) {
        return 1; // Low
    } else if (value > max) {
        return 3; // High
    } else {
        return 2; // Normal
    }
}

void sendData(String elapsedTime, int Temperature, float Voltage, int tempStatus, int voltStatus) {
    Serial.print("Connecting to ");
    Serial.println(host);

    client.setFingerprint(fingerprint); 

    if (!client.connect(host, httpsPort)) {
        Serial.println("Connection failed");
        return;
    }

    String string_Temperature = String(Temperature, DEC); 
    String string_Voltage = String(Voltage, decimalPrecision);
    String string_tempStatus = String(tempStatus, DEC);
    String string_voltStatus = String(voltStatus, DEC);
    String url = "/macros/s/" + String(googleScriptId) + "/exec?Timestamp=" + elapsedTime + "&Temperature=" + string_Temperature + "&Voltage=" + string_Voltage + "&TempStatus=" + string_tempStatus + "&VoltStatus=" + string_voltStatus;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Request sent");

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("Headers received");
            break;
        }
    }

    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
        Serial.println("ESP8266/Arduino CI successful!");
    } else {
        Serial.println("ESP8266/Arduino CI has failed");
    }
    Serial.println("Reply was:");
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("Closing connection");
}
