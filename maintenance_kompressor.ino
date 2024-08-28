#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_MLX90614.h>
#include <math.h>

// Pressure sensor pin
#define PRESSURE_SENSOR_PIN A0 // Connect Pressure Sensor OUT pin to A0 (Analog Input) on WeMos D1 R2

// WiFi credentials
const char* ssid = "Fauzi Surya Fazri"; 
const char* password = "123456789";  

// Google Apps Script endpoint
const char* host = "script.google.com";
const int httpsPort = 443;
const char* googleScriptId = "AKfycbzkb6DMZmEyOoM5edeVdXy31ixg6HB5ZUobKO2H7-dggW87dcoPQH6lQmED7bxNtm58iw"; 

// SHA1 fingerprint of the certificate
const char* fingerprint = "A9:52:08:E0:FC:37:B4:6B:5F:CF:C5:AB:C4:10:C7:D6:00:4D:DC:69";

// I2C pins for WeMos D1 R2
#define I2C_SDA_PIN D2
#define I2C_SCL_PIN D1

// Create instances
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
LiquidCrystal_I2C lcd(0x27, 16, 2); 
WiFiClientSecure client;

// Variables for display control
unsigned long previousMillis = 0;
const long interval = 2000; // Interval for switching display (2 seconds)
int displayState = 0;

// Pressure sensor constants
const int pressureZero = 102.4; 
const int pressureMax = 921.6; 
const int pressuretransducermaxPSI = 100; 

void setup() {
    Serial.begin(9600);

    // Initialize the LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Monitoring");
    lcd.setCursor(0, 1);
    lcd.print("Suhu & Tekanan");

    // Initialize I2C with specified pins
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

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
}

void loop() {
    // Read temperature from MLX90614
    float ambientTemp = mlx.readAmbientTempC();
    float objectTemp = mlx.readObjectTempC();

    int Temperature = (int)objectTemp;

    // Read pressure from Pressure Sensor
    float pressureValue = analogRead(PRESSURE_SENSOR_PIN);
    float pressure = ((pressureValue - pressureZero) * pressuretransducermaxPSI) / (pressureMax - pressureZero);
    
    int tempStatus = getStatus(Temperature, 33, 35);
    int pressureStatus = getStatus(pressure, 50, 100); 

    if (isnan(objectTemp)) {
        Serial.println("Failed to read from MLX90614 sensor!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error");
    } else {
        Serial.print("Temperature: ");
        Serial.print(Temperature);
        Serial.print(" °C\tPressure: ");
        Serial.print(pressure);
        Serial.print(" psi\tTemp Status: ");
        Serial.print(tempStatus);
        Serial.print("\tPressure Status: ");
        Serial.println(pressureStatus);

        sendData(Temperature, pressure, tempStatus, pressureStatus);
        
        // Update display based on the interval
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            displayState = (displayState + 1) % 2;

            lcd.clear();
            if (displayState == 0) {
                lcd.setCursor(0, 0);
                lcd.print("Temp:");
                lcd.print(Temperature);
                lcd.print("C");
                lcd.setCursor(0, 1);
                lcd.print("Pressure:");
                lcd.print(pressure, 1);
                lcd.print(" psi");
            } else {
                lcd.setCursor(0, 0);
                lcd.print("TS:");
                lcd.print(tempStatus);
                lcd.setCursor(0, 1);
                lcd.print("PS:");
                lcd.print(pressureStatus);
            }
        }
    }

    delay(500); 
}

int getStatus(float value, float min, float max) {
    if (value < min) {
        return 1; // tidak stabil
    } else if (value > max) {
        return 3; // tidak layak pakai
    } else {
        return 2; // stabil
    }
}

void sendData(int Temperature, float Pressure, int tempStatus, int pressureStatus) {
    Serial.print("Connecting to ");
    Serial.println(host);

    client.setFingerprint(fingerprint); 

    if (!client.connect(host, httpsPort)) {
        Serial.println("Connection failed");
        return;
    }

    String string_Temperature = String(Temperature, DEC); 
    String string_Pressure = String(Pressure, 1);
    String string_tempStatus = String(tempStatus, DEC);
    String string_pressureStatus = String(pressureStatus, DEC);
    String url = "/macros/s/" + String(googleScriptId) + "/exec?Temperature=" + string_Temperature + "&Pressure=" + string_Pressure + "&TempStatus=" + string_tempStatus + "&PressureStatus=" + string_pressureStatus;

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
    Serial.println("Reply was ");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("Closing connection");

}
