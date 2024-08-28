#include <HX711_ADC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Konfigurasi pin untuk HX711
const int HX711_dout = D7; // MCU > HX711 dout pin
const int HX711_sck = D6; // MCU > HX711 sck pin

// Konfigurasi pin untuk sensor ultrasonik
#define TRIGGER_PIN  D4
#define ECHO_PIN     D5
#define MAX_DISTANCE 200

// Konfigurasi pin untuk push button
const int buttonPin = D8; // Pin untuk push button
int gender = -1; // Variabel untuk menyimpan jenis kelamin (0 untuk perempuan, 1 untuk laki-laki)
bool buttonPressed = false; // Flag untuk menandai apakah tombol sudah ditekan
bool genderSelected = false; // Flag untuk menandai apakah gender sudah dipilih
bool measurementCompleted = false; // Flag untuk menandai apakah pengukuran selesai
unsigned long buttonPressTime = 0; // Waktu saat tombol terakhir kali ditekan
const unsigned long debounceDelay = 200; // Debounce delay
const unsigned long confirmDelay = 2000; // Delay untuk konfirmasi pemilihan gender

// WiFi configuration
const char* ssid = "wifi";
const char* password = "kucin123";

// Google Script Deployment URL
const char* serverName = "https://script.google.com/macros/s/AKfycbyskWKwaIc9q7xwFHFFgNbhlvMHrMtzzfgGD9Wrbx3on5j-Lq148tRmaHgAe3Jz9N0/exec";

// SHA-1 fingerprint of the server certificate
const char* fingerprint = "5E:16:23:DF:7D:42:8E:61:6E:AA:4A:CC:FB:08:1A:B9:8F:FA:E0:A2";

// Inisialisasi HX711 dan LCD
HX711_ADC Loadcell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address (0x27) and LCD size (16x2)
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

WiFiClientSecure wifiClient;

float lastWeight = 0; // Variabel untuk menyimpan nilai berat terakhir
float lastDistance = 0; // Variabel untuk menyimpan nilai jarak terakhir
float threshold = 0.1; // Ambang batas perubahan berat yang signifikan (dalam kg)
const long interval = 100; // Interval waktu antara pengukuran (ms)
const int stableReadingsRequired = 5; // Jumlah pembacaan stabil yang dibutuhkan
int stableCount = 0; // Counter untuk jumlah pembacaan stabil
bool dataSent = false; // Flag untuk menandai apakah data sudah dikirim

unsigned long lastTime = 0;
bool initialReadingsDone = false;

void setup() {
  Serial.begin(9600);

  // Inisialisasi HX711
  Loadcell.begin(); // start connection to HX711
  Loadcell.start(2000); // load cell gets 2000ms of time to stabilize
  Loadcell.setCalFactor(22.3813); // calibration factor for load cell

  // Inisialisasi LCD
  lcd.init(); // initialize the LCD
  lcd.backlight(); // turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("Select Gender:");
  lcd.setCursor(0, 1);
  lcd.print("M:1 F:0");

  // Inisialisasi WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // Set the fingerprint for the WiFiClientSecure
  wifiClient.setFingerprint(fingerprint);
  
  // Inisialisasi pin push button
  pinMode(buttonPin, INPUT_PULLUP); // Set pin push button sebagai input dengan pullup
}

void loop() {
  unsigned long currentTime = millis();
  
  // Membaca input push button untuk memilih gender
  if (!genderSelected) {
    handleGenderSelection(currentTime);
  } else if (!measurementCompleted) {
    handleMeasurement(currentTime);
  } else {
    displayDataSentMessage();
  }
}

void handleGenderSelection(unsigned long currentTime) {
  if (digitalRead(buttonPin) == LOW && !buttonPressed && (currentTime - buttonPressTime > debounceDelay)) {
    // Push button ditekan
    gender = (gender == 0) ? 1 : 0; // Toggle antara 0 (perempuan) dan 1 (laki-laki)
    buttonPressed = true;
    buttonPressTime = currentTime; // Simpan waktu saat tombol ditekan
    
    // Perbarui tampilan gender di LCD
    lcd.setCursor(0, 1);
    lcd.print("Gender: ");
    lcd.print(gender == 1 ? "M" : "F");
  } else if (digitalRead(buttonPin) == HIGH) {
    // Reset flag ketika tombol dilepas
    buttonPressed = false;
  }

  if (digitalRead(buttonPin) == LOW && (currentTime - buttonPressTime >= confirmDelay)) {
    // Konfirmasi pemilihan gender
    genderSelected = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gender sudah dipilih");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Standby untuk");
    lcd.setCursor(0, 1);
    lcd.print("pengukuran");
    delay(2000);
    lcd.clear();
    delay(2000); // Delay tambahan sebelum pengukuran
  }
}

void handleMeasurement(unsigned long currentTime) {
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;

    // Membaca berat dari load cell
    Loadcell.update(); // retrieves data from the load cell
    float weightInKg = abs(Loadcell.getData() / 1000.0); // convert to kilograms and apply abs
    int roundedWeightInKg = round(weightInKg); // Membulatkan ke bilangan bulat

    // Membaca jarak dari sensor ultrasonik
    unsigned int uS = sonar.ping();
    float distance = uS / US_ROUNDTRIP_CM;

    // Menampilkan berat dan jarak di Serial Monitor
    Serial.print("weight(kg): "); // print to serial monitor
    Serial.println(roundedWeightInKg);
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Menampilkan berat dan jarak di LCD
    lcd.setCursor(0, 0); // set the cursor to the first row
    lcd.print("Weight: "); // print weight to the LCD
    lcd.print(roundedWeightInKg);
    lcd.print(" kg   "); // adding extra spaces to clear any remaining characters

    lcd.setCursor(0, 1); // set the cursor to the second row
    lcd.print("Distance: "); // print distance to the LCD
    lcd.print(distance);
    lcd.print(" cm   "); // adding extra spaces to clear any remaining characters

    // Logika untuk memeriksa stabilitas berat dan tinggi
    if (initialReadingsDone) {
      if (abs(roundedWeightInKg - lastWeight) < threshold && abs(distance - lastDistance) < threshold) {
        stableCount++;
        if (stableCount >= stableReadingsRequired && !dataSent) {
          // Berat dan tinggi stabil, kirim data ke Google Sheets
          if (roundedWeightInKg != 0) {
            sendData(roundedWeightInKg, distance);
          }
        }
      } else {
        stableCount = 0; // Reset stable count jika ada perubahan signifikan
      }
    } else {
      // Pastikan kita memiliki pembacaan awal yang stabil sebelum mulai memeriksa stabilitas
      stableCount++;
      if (stableCount >= stableReadingsRequired) {
        initialReadingsDone = true;
        stableCount = 0; // Reset stable count untuk memulai pemeriksaan stabilitas sesungguhnya
      }
    }
    
    // Perbarui nilai berat dan jarak terakhir
    lastWeight = roundedWeightInKg;
    lastDistance = distance;
  }
}

void sendData(float weight, float distance) {
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    HTTPClient http;
    http.begin(wifiClient, serverName); // Gunakan WiFiClientSecure dengan URL

    // Mengirim data dalam format URL
    String httpRequestData = "weight=" + String(weight) + "&distance=" + String(distance) + "&gender=" + String(gender);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();

    // Set flag data sudah dikirim
    dataSent = true;
    measurementCompleted = true;

    // Menampilkan pesan bahwa data terkirim
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Data terkirim!");
    lcd.setCursor(0, 1);
    lcd.print("Tekan tombol");
    delay(2000); // Delay untuk menunjukkan pesan
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("untuk ulang");
    lcd.setCursor(0, 1);
    lcd.print("Select Gender");
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void displayDataSentMessage() {
  if (digitalRead(buttonPin) == LOW && !buttonPressed && (millis() - buttonPressTime > debounceDelay)) {
    buttonPressed = true;
    buttonPressTime = millis();
    resetSystem(); // Reset sistem untuk kembali ke pemilihan gender
  } else if (digitalRead(buttonPin) == HIGH) {
    buttonPressed = false;
  }
}

void resetSystem() {
  // Reset semua variabel ke kondisi awal
  gender = -1;
  genderSelected = false;
  stableCount = 0;
  dataSent = false;
  measurementCompleted = false;
  lastWeight = 0;
  lastDistance = 0;
  initialReadingsDone = false; // Reset flag pembacaan awal
  
  // Reset tampilan LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Gender:");
  lcd.setCursor(0, 1);
  lcd.print("M:1 F:0");
}
