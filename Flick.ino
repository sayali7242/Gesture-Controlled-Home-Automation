/*
  AWS IoT WiFi

  This sketch securely connects to an AWS IoT using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a public
  certificate for SSL/TLS authetication.

  It publishes a message every 5 seconds to arduino/outgoing
  topic and subscribes to messages on the arduino/incoming
  topic.

  The circuit:
  - Arduino Nano 33 IoT board 
  - Push button on pin 2, normal off
*/

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> 
#include <Arduino_LSM6DS3.h>

#include <ArduinoJson.h>

#include "arduino_secrets.h"

#include <math.h>
#include <string.h>
#include "nano_numtostr.h"
//Button control
const int buttonPin = 2;
int buttonState = 0; 

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

void setup() {

  //Using pin 2 for detecting button press
  pinMode(buttonPin, INPUT);
  
  //using the builtIn led to signal wifi and aws connection;
  pinMode(LED_BUILTIN, OUTPUT);

  
  Serial.begin(115200);
//  while (!Serial);
  
  // Custom IMU check
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (true); // halt program
  } 
  Serial.println("IMU initialized!");

  //Custom end

  
  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

//filtering variables for acceleration
float filteredaX = 0;
float filteredaY = 0;
float filteredaZ = 0;
//filtering variables for rotation
float filteredgX = 0;
float filteredgY = 0;
float filteredgZ = 0;

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();


  //custom gyro and accel block
  float aX, aY, aZ;
  float gX, gY, gZ;
  const char * spacer = ", ";
  //while pressed, do:
  int flag = 0;
  int flicks[] = {0,0,0};
  int count = 0;
  while(digitalRead(buttonPin)==HIGH){
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {      
      flag = 1; //button was pressed
      float f_window = 0.1;  //weight of current value. 1= 100%
      float f_window_g = 1;
      //fil;tering accel and gyro values
      IMU.readAcceleration(aX, aY, aZ);
      filteredaX = (1-f_window) * filteredaX + (f_window) * aX;
      filteredaY = (1-f_window) * filteredaY + (f_window) * aY;
      filteredaZ = (1-f_window) * filteredaZ + (f_window) * aZ;
      IMU.readGyroscope(gX, gY, gZ);
      filteredgX = (1-f_window_g) * filteredgX + (f_window_g) * gX;
      filteredgY = (1-f_window_g) * filteredgY + (f_window_g) * gY;
      filteredgZ = (1-f_window_g) * filteredgZ + (f_window_g) * gZ;
      float accelSense = 1.3;
      float gyroSense = 500;
      
      // Storing flick information as a triplet of tribits (XYZ), where each bit can take on values 0,1 or 2. gX is Least Value tribit, gZ is Most Value tribit. 
      int gscore= 13;
      int gXbit = 1;
      int gYbit = 1;
      int gZbit = 1;
      if(filteredgX>gyroSense) gXbit = 2;
      else if(filteredgX<-gyroSense) gXbit = 0;
      if(filteredgY>gyroSense) gYbit = 2;
      else if(filteredgY<-gyroSense) gYbit = 0;
      if(filteredgZ>gyroSense) gZbit = 2;
      else if(filteredgZ<-gyroSense) gZbit = 0;
  
      gscore = 1*gXbit + 3*gYbit + 3*3*gZbit;
      if(gscore!=13 && count<2) //detect only 3-long flick chains, as that is the limit for webhooks
      {
        flicks[count]=gscore;
        count++;
        //short delay before detecting next flick. Can be tuned to avoid extra triggers depending on user flick speed
        delay(100);
      }
      float tiltAngle = atan(filteredaZ/filteredaX) * 57.2957795131;
      if(filteredaX <0){
        tiltAngle = tiltAngle+180;
      }
    }
  }

  //see if button was pressed
  if(flag==1){
    //buffer to store message
    char msg[100]="";
    //see if any flicks were detected
    if(count>0){
      strcat(msg,"flicks_");
      for(int i=0; i<count;i++){
        char str[20];
        strcat(msg,"_");
        ftoa(flicks[i], str, 0);
        strcat(msg,str);
      }
    }
    else{
      //if no flicks, check angle
      strcat(msg,"percent_");
      float tiltAngle = atan(filteredaZ/filteredaX) * 57.2957795131;
      if(filteredaX <0){
        tiltAngle = tiltAngle+180;
      }
      //Converting to percent
      float percent=(tiltAngle)/180*100;
      if(percent<1){
        percent = 1;
      }
      else if(percent>100){
        percent=100;
      }
      //convert percentage to string and append to message buffer
      char str[20];
      ftoa(percent, str, 0);
      strcat(msg,str);
    }
    Serial.println(msg);
    publishMessageGyroStr(msg); 
  }

  
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  digitalWrite(LED_BUILTIN, LOW);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");
  
  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    //delay(5000);
    
    //flash light while attempting to connect to mqtt client
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on by making the voltage HIGH
    delay(200);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(200);                       // wait for a second
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(200);                       
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);                       
    }
  digitalWrite(LED_BUILTIN, HIGH);    by making the voltage HIGH
  Serial.println();

  
  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  mqttClient.print("hello ");
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void publishMessageGyro(int gscore) {
  //primitive message passing. Use only to send singular data values for gyro
  Serial.println("Publishing message to gyroscope topic");

  //Json parsing
  char JSONmessageBuffer[100];
  
  DynamicJsonDocument doc(1024);
  doc["device"] = "Nano33IoT";
  doc["sensor"] = "gyro";
  doc["data"] = gscore;
  serializeJson(doc, JSONmessageBuffer);
  Serial.println(JSONmessageBuffer);

  
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing-gyroscope");
  mqttClient.println(JSONmessageBuffer);
  //  mqttClient.print("GScore ");
  //  mqttClient.print(gscore);
  mqttClient.endMessage();
}

void publishMessageGyroStr(char *str) {
  //To publish information on button release. 
  Serial.println("Publishing message to gyroscope topic");

  //Json parsing
  char JSONmessageBuffer[100];
  
  DynamicJsonDocument doc(1024);
  doc["device"] = "Nano33IoT";
  doc["sensor"] = "All";
  doc["data"] = str;
  serializeJson(doc, JSONmessageBuffer);
  Serial.println(JSONmessageBuffer);

  
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing-gyroscope");
  mqttClient.println(JSONmessageBuffer);
//  mqttClient.print("GScore ");
//  mqttClient.print(gscore);
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}
