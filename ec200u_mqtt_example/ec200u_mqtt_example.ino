#include <Arduino.h>

#define EC200U_SERIAL   Serial2
#define EC200U_BAUD     115200

// APN Configuration
const char* apn = "airtelgprs.com";

// MQTT Broker Configuration
const char* MQTT_SERVER = "test.mosquitto.org"; // your server adress
const int MQTT_PORT = 1883; // your server port ususally 1883 of 8883
const char* CLIENT_ID = "Ameba_Client"; //client id 
const char* PUBLISH_TOPIC = "test/responses"; //publish topic name 
const char* SUBSCRIBE_TOPIC = "test/commands";  //subscibe topic name
const char* clientUser = "user";   // user id required for ssl
const char* clientPass = "pass";  // password

// Global Variables
bool subscribed = false;

/* ========================== Utility Functions ========================== */
void sendATCommand(const char* cmd, unsigned long timeout = 2000, bool debug = true) {
  if (debug) {
    Serial.print(">>> ");
    Serial.println(cmd);
  }
  
  EC200U_SERIAL.println(cmd);
  
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (EC200U_SERIAL.available()) {
      char c = EC200U_SERIAL.read();
      if (debug) Serial.write(c);
    }
  }
  
  if (debug) Serial.println();
}

/* ========================== Initialization ========================== */
void setup4GModule() {
  Serial.println("Initializing 4G module...");
  
  // Reset module
  sendATCommand("AT+CFUN=0", 5000);
  sendATCommand("AT+CFUN=1", 5000);
  delay(3000);
  
  // Configure APN and activate PDP context
  String apnCmd = "AT+QICSGP=1,1,\"" + String(apn) + "\",\"\",\"\",1";
  sendATCommand(apnCmd.c_str(), 5000);
  sendATCommand("AT+QIACT=1", 10000);
  
  Serial.println("4G module initialized");
}

void setupMQTT() {
  Serial.println("Configuring MQTT connection...");
  
  // Configure MQTT parameters
  sendATCommand("AT+QMTCFG=\"recv/mode\",0,0,1", 1000);  // Buffer URCs
  sendATCommand("AT+QMTCFG=\"ssl\",0,1,0", 1000);        // Enable SSL
  
  // Open MQTT connection
  String openCmd = "AT+QMTOPEN=0,\"" + String(MQTT_SERVER) + "\"," + String(MQTT_PORT);
  sendATCommand(openCmd.c_str(), 5000);

  // Connect to broker
  String connectCmd = "AT+QMTCONN=0,\"" + String(CLIENT_ID) + "\",\"" + 
                      String(clientUser) + "\",\"" + String(clientPass) + "\"";
  sendATCommand(connectCmd.c_str(), 5000);

  // Subscribe to topic
  String subscribeCmd = "AT+QMTSUB=0,1,\"" + String(SUBSCRIBE_TOPIC) + "\",0";
  sendATCommand(subscribeCmd.c_str(), 2000);
  subscribed = true;

  Serial.println("MQTT setup completed");
}

/* ========================== Message Handling ========================== */
void handleIncomingMessages() {
  while (EC200U_SERIAL.available()) {
    String response = EC200U_SERIAL.readStringUntil('\n');
    response.trim();
    
    if (response.startsWith("+QMTRECV:")) {
      // Parse MQTT message
      int topicStart = response.indexOf('"') + 1;
      int topicEnd = response.indexOf('"', topicStart);
      int msgStart = response.indexOf('"', topicEnd + 1) + 1;
      int msgEnd = response.indexOf('"', msgStart);
      
      String topic = response.substring(topicStart, topicEnd);
      String message = response.substring(msgStart, msgEnd);
      
      Serial.print("\n[Received on ");
      Serial.print(topic);
      Serial.print("] ");
      Serial.println(message);
      
      // Handle specific messages
      String reply = "";
      message.toLowerCase();  // Case-insensitive matching
      
      if (message == "hi" || message == "hello") {
        reply = "hii";
      }
      // Add more command handlers here as needed
      
      // Send response if applicable
      if (reply != "") {
        Serial.print("Sending reply: ");
        Serial.println(reply);
        
        String publishCmd = "AT+QMTPUB=0,0,0,0,\"" + 
                            String(PUBLISH_TOPIC) + "\",\"" + 
                            reply + "\"";
        sendATCommand(publishCmd.c_str(), 5000);
      }
    }
  }
}

/* ========================== Arduino Lifecycle ========================== */
void setup() {
  // Initialize serial ports
  Serial.begin(115200);
  EC200U_SERIAL.begin(EC200U_BAUD);
  delay(3000);  // Wait for module initialization

  setup4GModule();
  setupMQTT();
  Serial.println("System ready. Listening for messages...");
}

void loop() {
  handleIncomingMessages();
  
  // Reconnect if subscription lost
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    lastCheck = millis();
    if (!subscribed) {
      Serial.println("Reconnecting MQTT...");
      setupMQTT();
    }
  }
}