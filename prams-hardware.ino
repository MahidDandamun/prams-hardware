#include <Adafruit_ADS1X15.h>
#include<Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
//#include "addons/RTDBHelper.h"
#include<WiFi.h>
#include "env.h"

#define WIFI_SSID "2G"
#define WIFI_PASSWORD "257IllangIllangst."

#define USER_EMAIL "dandamunmahid@gmail.com"
#define USER_PASSWORD "Ap10022002"

#define API_KEY "AIzaSyBSY5KGG1KfkYHBzNfDOvgUnUuo3ACDcGQ"
#define DATABASE_URL "https://prams-esp32-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "prams-esp32"
// Constants
const int SAMPLES = 5;        // Number of samples for averaging
const int TURB_SAMPLES = 600; // Sampling number for turbidity
const int DELAY_MS = 100;     // Delay between readings in milliseconds

const int PH_PIN = 0;    // ADS1115 A0
const int TEMP_PIN = 1;    // ADS1115 A1
const int TURB_PIN = 2;    // ADS1115 A2

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Adafruit_ADS1115 ads;

// Sensor Readings
int16_t ph_readings[SAMPLES];
int16_t temp_readings[SAMPLES];
int16_t turb_readings[TURB_SAMPLES];

// Sensor Values
float ph_value = 0.0;
float temp_value = 0.0;
float turb_value = 0.0;

// Calibration Values
const float PH_SLOPE = -5.7;
const float PH_INTERCEPT = 21.34 + 0.5;
const float VOLTAGE_CONVERSION = 0.1875 / 1000.0;  // ADS1115 voltage conversion factor


const int RX_BUFFER_SIZE = 4096;
const int TX_BUFFER_SIZE = 1024;

// Payload response size to be collected 
const int FBDO_RESPONSE_SIZE = 2048;

unsigned long dataMillis = 0;

void setup() {
  Serial.begin(115200);
  initWiFi();
  initFirebase();
 // initAds();
}
void loop(){
  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))
  {
    FirebaseJson json;

// Select the fields you want to retrieve
  json.set("fields/ph/doubleValue", "ph");
  json.set("fields/temperature/doubleValue", "temperature");
  json.set("fields/turbidity/doubleValue", "turbidity");

    // Specify the collection to query
  //query.set("from/collectionId", "data");

// Optional: Add ordering (e.g., order by temperature descending)
//query.set("orderBy/field/fieldPath", "temperature");
//query.set("orderBy/direction", "DESCENDING");

// Optional: Limit the number of results
//query.set("limit", 10);

// Execute the query
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", "sensor-data/data", &json)) {
      Serial.println("Query results:");
      Serial.println(fbdo.payload().c_str());
  } else {
      Serial.println("Query failed:");
      Serial.println(fbdo.errorReason());
  }
  }
}

// readings = array of values, sampling_size = number of readings, analog_pin = sensor pin
void collect_samples(int16_t readings[], int analog_pin) {
  for (int i = 0; i < sizeof(readings); i++) {
    readings[i] = ads.readADC_SingleEnded(analog_pin);
    delay(10); 
  }
}

// sat
float calculate_average(int16_t readings[]) {
    long sum = 0;
    int sampling_size = sizeof(readings);
    for (int i = 0; i < sampling_size; i++) {
        sum += readings[i];
    }
    return (float)sum / sampling_size;
}
// pH = 0, temperature = 1, turbidity = 2 
float convert_value(int16_t readings[], int parameter){
  float average_value = calculate_average(readings);
  float converted_voltage = average_value * VOLTAGE_CONVERSION;
  switch(parameter){
    case 0:   
      ph_value = PH_SLOPE * converted_voltage + PH_INTERCEPT;
      break;
    case 1:          
      temp_value = converted_voltage * 8;
      break;
    case 2:
      turb_value = converted_voltage * 1000.0;   
      break;   
  }
}
// pH = 0, temperature = 1, turbidity = 2 
void calculate_parameter(int pin) {
  switch(pin){
    case 0:
      collect_samples(ph_readings, pin);
      convert_value(ph_readings, pin);
      break;
    case 1:
      collect_samples(temp_readings, pin);
      convert_value(temp_readings, pin);
      break;
    case 2: 
      collect_samples(turb_readings, pin);
      convert_value(turb_readings, pin);
      break;
  }
}
void initFirebase()
{
  config.api_key = API_KEY;
  //config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(RX_BUFFER_SIZE /* 512 - 16384*/, TX_BUFFER_SIZE);

  //payload size
  fbdo.setResponseSize(FBDO_RESPONSE_SIZE);
  Firebase.begin(&config, &auth);
  Serial.println("Connected to Firebase successfully");
}
void initWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void initAds(){
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);  
  }
  ads.setGain(GAIN_TWOTHIRDS);  
  Serial.println("Ads mounted successfully");
}

void print_values() {
    Serial.print("pH: ");
    Serial.print(ph_value, 2);
    Serial.print(" | Temp: ");
    Serial.print(temp_value, 1);
    Serial.print("°C | Turbidity: ");
    Serial.print(turb_value, 1);
    Serial.println(" NTU");
}