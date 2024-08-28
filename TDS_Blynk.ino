#include <GravityTDS.h>

#include <EEPROM.h>

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6Q-vjBCEC"
#define BLYNK_TEMPLATE_NAME "Monitoring dan kontrol"
#define BLYNK_AUTH_TOKEN "Xy7UgbQjZlC5yJGvtYQcCdLE2Cy7hQOK"
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>
#define TdsSensorPin 35

BlynkTimer timer;
GravityTDS gravityTds;



char auth[] = BLYNK_AUTH_TOKEN;
float temperature = 25,tdsValue = 0;
 
// BLYNK_CONNECTED() {}


//char ssid[] = "wifi";  
//char pass[] = "12345678";  

void setup()
{
  Blynk.begin(BLYNK_AUTH_TOKEN, "wifi", "kucin123");
    Serial.begin(9600);
    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(3.3);
    gravityTds.setAdcRange(4096);
    gravityTds.begin();  
    timer.setInterval(2500L, sendSensor);
}
void loop()
{
//    temperature = readTemperature();  //pembacaan sensor temperature
    //gravityTds.setTemperature(temperature);  // 
     Blynk.run();
     timer.run();
   
}

void sendSensor()
{
   gravityTds.update();  
    tdsValue = gravityTds.getTdsValue();
    Serial.print(tdsValue,0);
    Serial.println("ppm");
    delay(1000);

    Blynk.virtualWrite(V5, tdsValue);
   
}
