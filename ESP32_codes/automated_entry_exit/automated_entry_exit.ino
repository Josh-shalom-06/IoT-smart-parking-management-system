#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi credentials
const char* ssid = "La";
const char* password = "Loyolai@#2050";

// Firebase configuration
#define API_KEY "AIzaSyDtfVW_Tl6Z-rzYcfZr4uKz8Q7hO41sYHU"
#define DATABASE_URL "https://iot-smart-park-sys-josh-default-rtdb.firebaseio.com/"
#define USER_EMAIL "gjoshalom@gmail.com"
#define USER_PASSWORD "josh@firebase"

// Pin definitions
#define RST_PIN         2
#define SS_PIN          5
#define ENTRY_SERVO_PIN 12
#define EXIT_SERVO_PIN  14
#define IR_SENSOR_PIN   34

// LCD configuration (I2C address - common addresses are 0x27 or 0x3F)
#define LCD_ADDRESS     0x27
#define LCD_COLUMNS     16
#define LCD_ROWS        2

// Servo positions
#define GATE_CLOSED     0
#define GATE_OPEN       90
#define GATE_DELAY      3000  // 3 seconds

// Initialize components
MFRC522 rfid(SS_PIN, RST_PIN);
Servo entryServo;
Servo exitServo;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Global variables
bool entryGateOpen = false;
bool exitGateOpen = false;
unsigned long entryGateTimer = 0;
unsigned long exitGateTimer = 0;
unsigned long lcdMessageTimer = 0;
String lastScannedUID = "";
bool showingMessage = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  displayMessage("Smart Parking", "System Starting...");
  
  // Initialize pins
  pinMode(IR_SENSOR_PIN, INPUT);
  
  // Initialize servos
  entryServo.attach(ENTRY_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);
  entryServo.write(GATE_CLOSED);
  exitServo.write(GATE_CLOSED);
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  displayMessage("Connecting to", "WiFi...");
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  displayMessage("WiFi Connected!", "Initializing...");
  
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Sign in with email and password
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Configure token status callback (use built-in callback)
  config.token_status_callback = tokenStatusCallback; // This uses the built-in function
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  displayMessage("Connecting to", "Firebase...");
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("");
  Serial.print("User UID: ");
  Serial.println(auth.token.uid.c_str());
  
  displayMessage("System Ready!", "Scan RFID Card");
  Serial.println("Smart Parking System Ready!");
}

void loop() {
  // Handle RFID scanning for entry
  handleRFIDEntry();
  
  // Handle IR sensor for exit
  handleIRExit();
  
  // Manage gate timers
  manageGateTimers();
  
  // Manage LCD message timer
  manageLCDMessages();
  
  delay(100);
}

void displayMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  
  // Set timer for temporary messages
  lcdMessageTimer = millis();
  showingMessage = true;
}

void manageLCDMessages() {
  // Return to default message after 5 seconds if showing a temporary message
  if (showingMessage && (millis() - lcdMessageTimer >= 5000)) {
    if (!entryGateOpen && !exitGateOpen) {
      displayMessage("System Ready!", "Scan RFID Card");
      showingMessage = false;
    }
  }
}

void handleRFIDEntry() {
  // Check if new card is present
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Read UID with proper formatting to ensure 8 digits
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0"; // Add leading zero for single digit hex values
    }
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  Serial.println("RFID Scanned: " + uid);
  displayMessage("RFID Scanned:", uid);
  
  // Prevent duplicate scans
  if (uid == lastScannedUID && (millis() - entryGateTimer < 5000)) {
    rfid.PICC_HaltA();
    return;
  }
  
  lastScannedUID = uid;
  
  // Check if user exists and has reservation
  int accessResult = checkUserAndReservation(uid);
  if (accessResult == 1) {
    displayMessage("Access Granted!", "Gate Opening...");
    openEntryGate();
  } else if (accessResult == 0) {
    displayMessage("Access Denied!", "No Reservation");
    Serial.println("Access denied: No valid reservation found");
    // Optional: Add buzzer or LED feedback for denied access
  } else {
    displayMessage("Access Denied!", "User Not Found");
    Serial.println("Access denied: User not registered");
    // Optional: Add buzzer or LED feedback for denied access
  }
  
  rfid.PICC_HaltA();
}

int checkUserAndReservation(String uid) {
  displayMessage("Checking User...", "Please Wait");
  
  // Check if user exists in rfid_users by trying to get data
  String userPath = "/rfid_users/" + uid;
  
  if (!Firebase.RTDB.get(&fbdo, userPath)) {
    Serial.println("User not found in database");
    return -1; // User not registered
  }
  
  // Check if the path actually has data
  if (fbdo.dataType() == "null") {
    Serial.println("User not found in database");
    return -1; // User not registered
  }
  
  Serial.println("User found in database");
  
  // Check if user has any slot reserved
  if (Firebase.RTDB.getJSON(&fbdo, "/slots")) {
    FirebaseJson json;
    json.setJsonData(fbdo.to<String>().c_str());
    FirebaseJsonData jsonData;
    
    // Check each slot
    for (int i = 1; i <= 6; i++) {
      String slotPath = "slot" + String(i) + "/reserved_by";
      
      if (json.get(jsonData, slotPath)) {
        String reserved_by = jsonData.to<String>();
        reserved_by.replace("\"", ""); // Remove quotes
        
        if (reserved_by == uid) {
          Serial.println("Valid reservation found for slot" + String(i));
          return 1; // Access granted - user found and has reservation
        }
      }
    }
  } else {
    Serial.printf("Failed to get slots data: %s\n", fbdo.errorReason().c_str());
  }
  
  Serial.println("No reservation found for this user");
  return 0; // User registered but no reservation
}

void handleIRExit() {
  int irValue = digitalRead(IR_SENSOR_PIN);
  
  // IR sensor typically reads LOW when object is detected
  if (irValue == LOW && !exitGateOpen) {
    Serial.println("Vehicle detected at exit");
    displayMessage("Vehicle Detected", "Exit Gate Open");
    openExitGate();
  }
}

void openEntryGate() {
  if (!entryGateOpen) {
    Serial.println("Opening entry gate");
    entryServo.write(GATE_OPEN);
    entryGateOpen = true;
    entryGateTimer = millis();
    
    // Log entry event to Firebase
    logEvent("entry", lastScannedUID);
  }
}

void openExitGate() {
  if (!exitGateOpen) {
    Serial.println("Opening exit gate");
    exitServo.write(GATE_OPEN);
    exitGateOpen = true;
    exitGateTimer = millis();
    
    // Log exit event to Firebase
    logEvent("exit", "IR_TRIGGERED");
  }
}

void manageGateTimers() {
  // Close entry gate after delay
  if (entryGateOpen && (millis() - entryGateTimer >= GATE_DELAY)) {
    Serial.println("Closing entry gate");
    entryServo.write(GATE_CLOSED);
    entryGateOpen = false;
    lastScannedUID = ""; // Reset last scanned UID
    displayMessage("Entry Gate", "Closed");
  }
  
  // Close exit gate after delay
  if (exitGateOpen && (millis() - exitGateTimer >= GATE_DELAY)) {
    Serial.println("Closing exit gate");
    exitServo.write(GATE_CLOSED);
    exitGateOpen = false;
    displayMessage("Exit Gate", "Closed");
  }
}

void logEvent(String eventType, String userId) {
  // Create event log
  FirebaseJson eventJson;
  eventJson.set("type", eventType);
  eventJson.set("userId", userId);
  eventJson.set("timestamp", (int)millis());
  
  // Generate unique event ID
  String eventId = eventType + "_" + String(millis());
  String eventPath = "/events/" + eventId;
  
  if (Firebase.RTDB.setJSON(&fbdo, eventPath, &eventJson)) {
    Serial.println("Event logged: " + eventType);
  } else {
    Serial.printf("Failed to log event: %s\n", fbdo.errorReason().c_str());
  }
}

// Optional: Function to handle network reconnection
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    displayMessage("WiFi Lost!", "Reconnecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected!");
    displayMessage("WiFi Connected!", "System Ready");
  }
}