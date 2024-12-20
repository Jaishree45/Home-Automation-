#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <ArduinoOTA.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
 
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
 
#define WIFI_SSID "Your_Wifi_Name"
#define WIFI_PASSWORD "Password"

// Insert Firebase project API Key
#define API_KEY " Fill Api From Firebase"

// Insert RTDB URL
#define DATABASE_URL "https://Project.firebaseio.com/"

// Define the user Email and password that already registered or added in your project
#define USER_EMAIL "User in Firebase"
#define USER_PASSWORD "Password"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;

// DHT sensor settings
#define DHTPIN 15     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
int LED1_PIN = 2;
int LED2_PIN = 4 ;
int LED3_PIN = 16 ;
int LED4_PIN = 17 ;
Servo servo;
#define SERVO_PIN 18

void setup()
{
  Serial.begin(115200);
  servo.attach(SERVO_PIN);
  servo.write(90); // Set initial servo position to 0 degrees

 
  // Initialize DHT sensor
  dht.begin();
  pinMode(LED1_PIN,OUTPUT);
  pinMode(LED2_PIN,OUTPUT);
  pinMode(LED3_PIN,OUTPUT);
  pinMode(LED4_PIN,OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Initialize OTA
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the API key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign-in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Reconnect WiFi automatically
  Firebase.reconnectNetwork(true);

  // Set the SSL buffer size
  fbdo.setBSSLBufferSize(4096, 1024); // Set Rx and Tx buffer sizes

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);
}

void loop()
{
  // Handle OTA updates
  ArduinoOTA.handle();

  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // Read temperature and humidity from DHT11 sensor
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); // Read temperature in Celsius

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C ");
    Serial.print("Humidity: ");
    Serial.println(humidity);

    // Create a JSON object to hold the data
    FirebaseJson json;

    json.set("temperature", temperature);
    json.set("humidity", humidity);

    // Send data to Firebase
    if (Firebase.setJSON(fbdo, "/sensor/temp", json)) {
      Serial.println("Data sent to Firebase successfully");
    } else {
      Serial.print("Error sending data: ");
      Serial.println(fbdo.errorReason());
    }

    // Read data from Firebase (for example, read some control data)
    if (Firebase.getJSON(fbdo, "/sensor")) {
      FirebaseJson &jsonData = fbdo.to<FirebaseJson>();
      Serial.println("Received data from Firebase:");
      Serial.println(jsonData.raw());
      processButtonStates(jsonData.raw());
        Serial.println("hello");
      
    } else {
      Serial.print("Error getting data: ");
      Serial.println(fbdo.errorReason());
    }

    Serial.println();

    count++;
  }
}      


// Function to process button states and control LEDs
void processButtonStates(String jsonData) {
  // Create a JSON document for parsing (adjust size as needed)
  StaticJsonDocument<512> doc;

  // Parse the JSON string
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print("JSON deserialization failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract button states
  const char* buttonState1 = doc["ButtenState1"];
  const char* buttonState2 = doc["ButtenState2"];
  const char* buttonState3 = doc["ButtenState3"];
  const char* buttonState4 = doc["ButtenState4"];
   int servoPosition = doc["servo"];
  // Control LEDs based on button states
  digitalWrite(LED1_PIN, (String(buttonState1) == "\"1\"") ? HIGH : LOW);
  digitalWrite(LED2_PIN, (String(buttonState2) == "\"1\"") ? HIGH : LOW);
  digitalWrite(LED3_PIN, (String(buttonState3) == "\"1\"") ? HIGH : LOW);
  digitalWrite(LED4_PIN, (String(buttonState4) == "\"1\"") ? HIGH : LOW);
  servo.write(servoPosition);
  Serial.print("Servo set to position: ");
    Serial.println(servoPosition);
  // Log the results to the Serial Monitor
  Serial.println("Button States:");
  Serial.print("LED1: "); Serial.println(String(buttonState1) == "\"1\"" ? "ON" : "OFF");
  Serial.print("LED2: "); Serial.println(String(buttonState2) == "\"1\"" ? "ON" : "OFF");
  Serial.print("LED3: "); Serial.println(String(buttonState3) == "\"1\"" ? "ON" : "OFF");
  Serial.print("LED4: "); Serial.println(String(buttonState4) == "\"1\"" ? "ON" : "OFF");
}
