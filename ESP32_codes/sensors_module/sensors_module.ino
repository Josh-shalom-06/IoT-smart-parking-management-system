#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Wi-Fi credentials
#define WIFI_SSID "La"
#define WIFI_PASSWORD "Loyolai@#2050"

// Firebase credentials
#define API_KEY "AIzaSyDtfVW_Tl6Z-rzYcfZr4uKz8Q7hO41sYHU"
#define DATABASE_URL "https://iot-smart-park-sys-josh-default-rtdb.firebaseio.com/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define NUM_SLOTS 6
#define THRESHOLD_CM 10
#define MAX_DISTANCE_CM 400
#define MIN_DISTANCE_CM 2
#define STABLE_READINGS 3  // Number of consecutive readings needed to confirm state change

// Pin assignments
int trigPins[NUM_SLOTS] = {2, 5, 19, 22, 27, 26};
int echoPins[NUM_SLOTS] = {4, 18, 21, 23, 14, 25};

// State tracking for each slot
String previousStatus[NUM_SLOTS];
int stableCount[NUM_SLOTS] = {0}; // Counter for stable readings
bool carDetected[NUM_SLOTS] = {false}; // Current car detection state
bool prevCarDetected[NUM_SLOTS] = {false}; // Previous car detection state

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting Smart Parking System with State Management...");

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Setup pins
  for (int i = 0; i < NUM_SLOTS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    digitalWrite(trigPins[i], LOW);
    previousStatus[i] = "Unknown"; // Initialize previous status
  }

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  auth.user.email = "gjoshalom@gmail.com";
  auth.user.password = "josh@firebase";

  Serial.println("Connecting to Firebase...");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Initialize slot states from Firebase
  initializeSlotStates();
  
  Serial.println("Setup completed!\n");
}

void initializeSlotStates() {
  Serial.println("Initializing slot states from Firebase...");
  for (int i = 0; i < NUM_SLOTS; i++) {
    String statusPath = "/slots/slot" + String(i + 1) + "/status";
    if (Firebase.RTDB.getString(&fbdo, statusPath)) {
      previousStatus[i] = fbdo.stringData();
      Serial.printf("Slot %d initialized with status: %s\n", i + 1, previousStatus[i].c_str());
    } else {
      previousStatus[i] = "Available";
      Serial.printf("Slot %d defaulted to: Available\n", i + 1);
    }
  }
}

float readUltrasonicDistance(int sensorIndex) {
  digitalWrite(trigPins[sensorIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[sensorIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[sensorIndex], LOW);
  
  long duration = pulseIn(echoPins[sensorIndex], HIGH, 30000);
  
  if (duration == 0) {
    return -1;
  }
  
  float distance = duration * 0.034 / 2;
  
  if (distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM) {
    return -1;
  }
  
  return distance;
}

String determineSlotStatus(int slotIndex, float distance, bool isReserved) {
  String currentStatus = previousStatus[slotIndex];
  bool carPresent = (distance > 0 && distance < THRESHOLD_CM);
  
  // Update car detection state
  prevCarDetected[slotIndex] = carDetected[slotIndex];
  carDetected[slotIndex] = carPresent;
  
  // Check for stable readings
  if (carDetected[slotIndex] == prevCarDetected[slotIndex]) {
    stableCount[slotIndex]++;
  } else {
    stableCount[slotIndex] = 0; // Reset counter on state change
  }
  
  // Only proceed with state changes if we have stable readings
  if (stableCount[slotIndex] < STABLE_READINGS) {
    return currentStatus; // Keep current status until stable
  }
  
  // State machine logic
  if (currentStatus == "Available") {
    if (isReserved) {
      return "Reserved";
    } else if (carPresent) {
      return "Occupied"; // Direct parking without reservation
    }
    return "Available";
  }
  else if (currentStatus == "Reserved") {
    if (carPresent) {
      return "Occupied"; // Car parked in reserved slot
    } else if (!isReserved) {
      return "Available"; // Reservation cancelled
    }
    return "Reserved"; // Stay reserved until car arrives
  }
  else if (currentStatus == "Occupied") {
    if (!carPresent) {
      // Clear the reservation when car leaves
      clearReservation(slotIndex);
      return "Available"; // Car left
    }
    return "Occupied"; // Car still there
  }
  
  return currentStatus; // Default case
}

void updateSlotInFirebase(int slotIndex, String newStatus, float distance) {
  String statusPath = "/slots/slot" + String(slotIndex + 1) + "/status";
  String distancePath = "/slots/slot" + String(slotIndex + 1) + "/distance";
  String timestampPath = "/slots/slot" + String(slotIndex + 1) + "/lastUpdated";
  
  // Update status
  if (Firebase.RTDB.setString(&fbdo, statusPath, newStatus)) {
    Serial.printf("✓ Slot %d status updated: %s\n", slotIndex + 1, newStatus.c_str());
  } else {
    Serial.printf("✗ Failed to update slot %d status: %s\n", slotIndex + 1, fbdo.errorReason().c_str());
  }
  
  // Update distance
  if (distance > 0) {
    Firebase.RTDB.setFloat(&fbdo, distancePath, distance);
  }
  
  // Update timestamp
  Firebase.RTDB.setTimestamp(&fbdo, timestampPath);
  
  // Log state transition
  if (newStatus != previousStatus[slotIndex]) {
    Serial.printf("🔄 Slot %d: %s → %s (Distance: %.2f cm)\n", 
                  slotIndex + 1, previousStatus[slotIndex].c_str(), newStatus.c_str(), distance);
    
    // Update parking statistics
    updateParkingStats(slotIndex + 1, previousStatus[slotIndex], newStatus);
  }
}

void clearReservation(int slotIndex) {
  String basePath = "/slots/slot" + String(slotIndex + 1);
  String reservedPath = basePath + "/reserved";
  String reservedByPath = basePath + "/reserved_by";

  // Clear reserved flag
  if (Firebase.RTDB.setBool(&fbdo, reservedPath, false)) {
    Serial.printf("🔓 Reservation cleared for slot %d\n", slotIndex + 1);
  } else {
    Serial.printf("❌ Failed to clear reservation for slot %d: %s\n", slotIndex + 1, fbdo.errorReason().c_str());
  }

  // Delete reserved_by field
  if (Firebase.RTDB.deleteNode(&fbdo, reservedByPath)) {
    Serial.printf("🧹 Cleared reserved_by for slot %d\n", slotIndex + 1);
  } else {
    Serial.printf("❌ Failed to delete reserved_by for slot %d: %s\n", slotIndex + 1, fbdo.errorReason().c_str());
  }
}


void updateParkingStats(int slotNumber, String fromStatus, String toStatus) {
  // Log parking events for analytics
  if (fromStatus == "Reserved" && toStatus == "Occupied") {
    Serial.printf("📊 Parking Event: Slot %d - Reserved slot occupied\n", slotNumber);
    // You can add more detailed logging here
  } else if (fromStatus == "Available" && toStatus == "Occupied") {
    Serial.printf("📊 Parking Event: Slot %d - Direct parking (no reservation)\n", slotNumber);
  } else if (fromStatus == "Occupied" && toStatus == "Available") {
    Serial.printf("📊 Parking Event: Slot %d - Car departed\n", slotNumber);
  }
}

void loop() {
  Serial.println("=== Parking System Status Check ===");
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    // Get distance reading
    float distance = readUltrasonicDistance(i);
    
    // Get reservation status from Firebase
    String reservedPath = "/slots/slot" + String(i + 1) + "/reserved";
    bool isReserved = false;

    if (Firebase.RTDB.getBool(&fbdo, reservedPath)) {
      isReserved = fbdo.boolData();
    }

    // Determine new status based on state machine
    String newStatus = determineSlotStatus(i, distance, isReserved);
    
    // Print current state
    Serial.printf("Slot %d: Distance=%.2f cm, Reserved=%s, Status=%s", 
                  i + 1, distance, isReserved ? "Yes" : "No", newStatus.c_str());
    
    if (carDetected[i]) {
      Serial.print(" [CAR DETECTED]");
    }
    
    if (stableCount[i] < STABLE_READINGS) {
      Serial.printf(" [STABILIZING %d/%d]", stableCount[i], STABLE_READINGS);
    }
    
    Serial.println();
    
    // Update Firebase if status changed or if this is a new stable reading
    if (newStatus != previousStatus[i] || stableCount[i] == STABLE_READINGS) {
      updateSlotInFirebase(i, newStatus, distance);
      previousStatus[i] = newStatus;
    }

    delay(100); // Small delay between sensors
  }
  
  Serial.println("=== End Status Check ===\n");
  delay(2000); // Check every 2 seconds
}
