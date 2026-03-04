#include <SoftwareSerial.h>
#include <math.h>

//**********ENTER IP ADDRESS OF SERVER******************//
#define HOST_IP     "172.16.193.110"    // IP ADDRESS OF COMPUTER THE BACKEND IS RUNNING ON
#define HOST_PORT   "8080"            // BACKEND FLASK API PORT
#define route       "api/update"      // LEAVE UNCHANGED 
#define idNumber    "620162191"       // YOUR ID NUMBER 

// WIFI CREDENTIALS
#define SSID        "MonaConnect"    // YOUR WIFI SSID   
#define password    ""                

#define stay        100

//**********PIN DEFINITIONS******************//
#define TRIG_PIN      6
#define ECHO_PIN      7
#define espRX         10
#define espTX         11
#define espTimeout_ms 300

// TANK DIMENSIONS
#define SENSOR_HEIGHT    94.5     // Height of sensor from base (inches)
#define MAX_WATER_HEIGHT 77.763   // Maximum water height for 1000 Gal (inches)
#define TANK_DIAMETER    61.5     // Tank diameter (inches)

// FUNCTION DECLARATIONS
float getRadar(void);
float getWaterHeight(float radar);
float getReserve(float radar);
float getPercentage(float waterheight);

SoftwareSerial esp(espRX, espTX); 

void setup(){
  Serial.begin(115200); 
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  espInit();  
}

void loop(){ 
  float radar       = getRadar();
  float waterheight = getWaterHeight(radar);
  float reserve     = getReserve(radar);
  float percentage  = getPercentage(waterheight);

  char message[200];
  char r[10], wh[10], res[10], pct[10];
  dtostrf(radar, 1, 2, r);
  dtostrf(waterheight, 1, 2, wh);
  dtostrf(reserve, 1, 2, res);
  dtostrf(percentage, 1, 2, pct);
  snprintf(message, sizeof(message),
    "{\"id\":\"%s\",\"type\":\"ultrasonic\",\"radar\":%s,\"waterheight\":%s,\"reserve\":%s,\"percentage\":%s}",
    idNumber, r, wh, res, pct);

  Serial.println(message);
  espUpdate(message);

  delay(1000);  
}

//***** UTIL FUNCTIONS ******

float getRadar(void){
  // Measure distance using HC-SR04
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0133 / 2; // Convert to inches
  return distance;
}

float getWaterHeight(float radar){
  // Water height = sensor height - radar measurement
  float waterheight = SENSOR_HEIGHT - radar;
  if(waterheight < 0) waterheight = 0;
  return waterheight;
}

float getReserve(float radar){
  // Volume of cylinder = pi * r^2 * h
  float radius = TANK_DIAMETER / 2.0;
  float waterheight = getWaterHeight(radar);
  float volumeCubicInches = M_PI * radius * radius * waterheight;
  float volumeGallons = volumeCubicInches / 231.0; // 1 US Gallon = 231 cubic inches
  if(volumeGallons < 0) volumeGallons = 0;
  return volumeGallons;
}

float getPercentage(float waterheight){
  float percentage = (waterheight / MAX_WATER_HEIGHT) * 100.0;
  if(percentage < 0) percentage = 0;
  return percentage;
}

void espSend(char command[]){   
    esp.print(command);
    while(esp.available()){ Serial.println(esp.readString());}    
}

void espUpdate(char mssg[]){ 
    char espCommandString[50] = {0};
    char post[290]            = {0};

    snprintf(espCommandString, sizeof(espCommandString),"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",HOST_IP,HOST_PORT); 
    espSend(espCommandString);
    delay(stay);

    snprintf(post,sizeof(post),"POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",route,HOST_IP,strlen(mssg),mssg);
  
    snprintf(espCommandString, sizeof(espCommandString),"AT+CIPSEND=%d\r\n", strlen(post));
    espSend(espCommandString);
    delay(stay);
    Serial.println(post);
    espSend(post);
    delay(stay);
    espSend("AT+CIPCLOSE\r\n");
}

void espInit(){
    char connection[100] = {0};
    esp.begin(115200); 
    Serial.println("Initiallizing");
    esp.println("AT"); 
    delay(1000);
    esp.println("AT+CWMODE=1");
    delay(1000);
    while(esp.available()){ Serial.println(esp.readString());} 

    snprintf(connection, sizeof(connection),"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,password);
    esp.print(connection);

    delay(3000);

    if(esp.available()){ Serial.print(esp.readString());}
    
    Serial.println("\nFinish Initializing");    
}