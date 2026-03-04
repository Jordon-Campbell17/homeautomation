//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##################################################################################################################

// IMPORT ALL REQUIRED LIBRARIES
#include <rom/rtc.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>

// IMPORT IMAGES
#include "lockclose.h"
#include "lockopen.h"

#ifndef _WIFI_H 
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif 

// IMPORT FONTS FOR TFT DISPLAY
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h> 

// TFT DISPLAY PIN DEFINITIONS
#define TFT_CS    5
#define TFT_RST   16 
#define TFT_DC    17
#define TFT_MOSI  23
#define TFT_SCK   18
#define TFT_MISO  19

// BUTTON PIN DEFINITIONS
#define BUTTON1_PIN  25
#define BUTTON2_PIN  26
#define BUTTON3_PIN  27

// POTENTIOMETER PIN
#define POT_PIN  34

// VARIABLES
uint8_t currentDigit = 1;  // Keeps track of the current digit being modified
bool lockState = false;     // Keeps track of Open and Close state of lock

uint8_t digit1Val = 0;
uint8_t digit2Val = 0;
uint8_t digit3Val = 0;
uint8_t digit4Val = 0;

// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620162191";
static const char* subtopic[]    = {"620162191_sub", "/elet2415"};
static const char* mqtt_server   = "10.22.13.197";
static uint16_t mqtt_port        = 1883;

// WIFI CREDENTIALS
const char* ssid       = "MonaConnect";
const char* password   = "";

// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL; 

// TFT OBJECT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);

// FUNCTION DECLARATIONS   
void checkHEAP(const char* Name);
void initMQTT(void);
unsigned long getTimeStamp(void);
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload);
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 
void digit1(uint8_t number);
void digit2(uint8_t number);
void digit3(uint8_t number);
void digit4(uint8_t number);
void checkPasscode(void);
void showLockState(void);

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif

void setup() {
    Serial.begin(115200);

    // INIT TFT DISPLAY
    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(ILI9341_BLACK);

    // CONFIGURE BUTTON PINS
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    pinMode(BUTTON3_PIN, INPUT_PULLUP);

    // CONFIGURE POT PIN
    pinMode(POT_PIN, INPUT);

    // DISPLAY STARTUP SCREEN - show 0 in all boxes
    tft.setFont(&FreeSansBold9pt7b);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(60, 120);
    tft.printf("Home Automation");
    tft.setCursor(60, 150);
    tft.printf("Remote Lock");

    digit1(0);
    digit2(0);
    digit3(0);
    digit4(0);

    initialize();
    vButtonCheckFunction();
}

void loop() {
    // READ POTENTIOMETER AND MAP TO 0-9
    int potValue = analogRead(POT_PIN);
    int mappedValue = map(potValue, 0, 4095, 0, 9);

    // ASSIGN MAPPED VALUE TO CURRENT SELECTED DIGIT
    switch(currentDigit) {
        case 1: digit1Val = mappedValue; digit1(digit1Val); break;
        case 2: digit2Val = mappedValue; digit2(digit2Val); break;
        case 3: digit3Val = mappedValue; digit3(digit3Val); break;
        case 4: digit4Val = mappedValue; digit4(digit4Val); break;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);  
}

//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck( void * pvParameters ) {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );     
      
    for( ;; ) {
        // BUTTON 1 - Select next digit
        if(digitalRead(BUTTON1_PIN) == LOW) {
            vTaskDelay(200 / portTICK_PERIOD_MS); // debounce
            currentDigit++;
            if(currentDigit > 4) currentDigit = 1;
        }

        // BUTTON 2 - Check passcode
        if(digitalRead(BUTTON2_PIN) == LOW) {
            vTaskDelay(200 / portTICK_PERIOD_MS); // debounce
            checkPasscode();
        }

        // BUTTON 3 - Lock (set lockState to false)
        if(digitalRead(BUTTON3_PIN) == LOW) {
            vTaskDelay(200 / portTICK_PERIOD_MS); // debounce
            lockState = false;
            showLockState();
        }
       
        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }
}

void vUpdate( void * pvParameters ) {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
 
    for( ;; ) {
        // PUBLISH timestamp to topic every second
        char payload[100];
        unsigned long ts = getTimeStamp();
        snprintf(payload, sizeof(payload),
            "{\"id\":\"620162191\",\"timestamp\":%lu}", ts);
        publish(pubtopic, payload);
        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}

unsigned long getTimeStamp(void) {
    time_t now;         
    time(&now);
    return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("\nMessage received : ( topic: %s ) \n", topic); 
    char *received = new char[length + 1] {0}; 
  
    for (int i = 0; i < length; i++) { 
        received[i] = (char)payload[i];    
    }

    Serial.printf("Payload : %s \n", received);

    // CONVERT MESSAGE TO JSON
    StaticJsonDocument<200> doc;
    deserializeJson(doc, received);

    // PROCESS MESSAGE - if passcode update received
    if(doc.containsKey("passcode")) {
        Serial.printf("New passcode received: %s\n", (const char*)doc["passcode"]);
    }

    delete[] received;
}

bool publish(const char *topic, const char *payload){   
    bool res = false;
    try{
        res = mqtt.publish(topic, payload);
        if(!res){
            res = false;
            throw false;
        }
    }
    catch(...){
        Serial.printf("\nError (%d) >> Unable to publish message\n", res);
    }
    return res;
}

//####################################################################
//#                        DIGIT FUNCTIONS                           #
//####################################################################
void digit1(uint8_t number){
    tft.setFont(&FreeSansBold18pt7b);
    tft.fillRoundRect(20, 260, 50, 50, 8, ILI9341_GREEN);
    tft.setCursor(33, 295);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.printf("%d", number);
}

void digit2(uint8_t number){
    tft.setFont(&FreeSansBold18pt7b);
    tft.fillRoundRect(80, 260, 50, 50, 8, ILI9341_GREEN);
    tft.setCursor(93, 295);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.printf("%d", number);
}

void digit3(uint8_t number){
    tft.setFont(&FreeSansBold18pt7b);
    tft.fillRoundRect(140, 260, 50, 50, 8, ILI9341_GREEN);
    tft.setCursor(153, 295);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.printf("%d", number);
}

void digit4(uint8_t number){
    tft.setFont(&FreeSansBold18pt7b);
    tft.fillRoundRect(200, 260, 50, 50, 8, ILI9341_GREEN);
    tft.setCursor(213, 295);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.printf("%d", number);
}

void checkPasscode(void){
    WiFiClient client;
    HTTPClient http;

    if(WiFi.status() == WL_CONNECTED){ 
        http.begin(client, "http://10.22.13.197:8080/api/check/combination");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
        char message[20];
        snprintf(message, sizeof(message), "passcode=%d%d%d%d",
            digit1Val, digit2Val, digit3Val, digit4Val);
                      
        int httpResponseCode = http.POST(message);

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String received = http.getString();

            // CONVERT TO JSON
            StaticJsonDocument<200> doc;
            deserializeJson(doc, received);

            // PROCESS RESPONSE
            const char* status = doc["status"];
            if(strcmp(status, "complete") == 0) {
                lockState = true;
                showLockState();
            } else {
                lockState = false;
                showLockState();
            }
        }     
        http.end();
    }
}

void showLockState(void){
    tft.setFont(&FreeSansBold9pt7b);  
    tft.setTextSize(1);
    
    if(lockState == true){
        tft.drawRGBBitmap(68, 10, lockopen, 104, 97); 
        tft.setCursor(50, 200);  
        tft.setTextColor(ILI9341_WHITE); 
        tft.printf("Access Denied"); 
        tft.setCursor(50, 200);  
        tft.setTextColor(ILI9341_GREEN); 
        tft.printf("Access Granted");
    }
    else {
        tft.drawRGBBitmap(68, 10, lockclose, 104, 103); 
        tft.setCursor(50, 200);  
        tft.setTextColor(ILI9341_WHITE); 
        tft.printf("Access Granted"); 
        tft.setCursor(50, 200);  
        tft.setTextColor(ILI9341_RED); 
        tft.printf("Access Denied"); 
    }
}