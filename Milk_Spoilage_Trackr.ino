#include <WiFi.h>       
#include <DHT.h>
#include <DallasTemperature.h>  
#include <OneWire.h>  
#include <PubSubClient.h>
#include <cmath>

const char* ssid = "ssid";
const char* password = "password";  

const char* mqtt_server = "io.adafruit.com";
const char* mqtt_username = "adafruit_username";
const char* mqtt_key = "aio_key";
const char* topic_status = "adafruit_username/feeds/milk-spoilage.status";
const char* topic_alert = "adafruit_username/feeds/milk-spoilage.alert";

#define DHTPIN 4   
#define DHTTYPE DHT22
#define MQ135PIN 34 
#define PHPIN 35    
#define RELAY_PIN 4  
#define ONE_WIRE_BUS 2  // GPIO2 for DS18B20 data pin

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);

// Sensor Values
float temperature, humidity, gas_level, ph_value;
float spoilage_risk = 0;

// Function to Connect to WiFi
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

// Function to Reconnect MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_key)) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying...");
      delay(5000);
    }
  }
}

float coldTemp(float temp) {
  if (temp <= 5) return 1;
  else if (temp > 5 && temp < 10) return (10 - temp) / 5.0;
  else return 0;
}

float warmTemp(float temp) {
  if (temp >= 10) return 1;
  else if (temp > 5 && temp < 10) return (temp - 5) / 5.0;
  else return 0;
}

float lowHumidity(float hum) {
  if (hum <= 50) return 1;
  else if (hum > 50 && hum < 80) return (80 - hum) / 30.0;
  else return 0;
}

float highHumidity(float hum) {
  if (hum >= 80) return 1;
  else if (hum > 50 && hum < 80) return (hum - 50) / 30.0;
  else return 0;
}

float acidicPH(float ph) {
  if (ph < 6.5) return 1;
  else if (ph >= 6.5 && ph < 7) return (7 - ph) / 0.5;
  else return 0;
}

float goodPH(float ph) {
  if (ph >= 7) return 1;
  else if (ph >= 6.5 && ph < 7) return (ph - 6.5) / 0.5;
  else return 0;
}

float lowGas(float gas) {
  if (gas < 400) return 1;
  else if (gas >= 400 && gas < 800) return (800 - gas) / 400.0;
  else return 0;
}

float highGas(float gas) {
  if (gas >= 800) return 1;
  else if (gas >= 400 && gas < 800) return (gas - 400) / 400.0;
  else return 0;
}

float calculateSpoilageRisk(float temp, float humidity, float ph, float gas) {
  float cold = coldTemp(temp);
  float warm = warmTemp(temp);
  float lowHum = lowHumidity(hum);
  float highHum = highHumidity(hum);
  float acidic = acidicPH(ph);
  float good = goodPH(ph);
  float lowG = lowGas(gas);
  float highG = highGas(gas);

  float r1 = fmin(warm, highHum); 
  float r2 = fmin(cold, lowHum);      
  float r3 = fmin(highG, acidic);       
  float r4 = fmin(lowG, good);     
  float r5 = fmin(cold, acidic);        

  float veryHigh = 90;
  float high = 75;
  float medium = 50;
  float low = 20;

  float numerator = r1 * veryHigh + r2 * low + r3 * veryHigh + r4 * low + r5 * medium;
  float denominator = r1 + r2 + r3 + r4 + r5;

  if (denominator == 0) return 0;
  return numerator / denominator;
}

// UV Sterilization Function
void activateUVSterilization() {
  Serial.println("Activating UV Sterilization...");
  client.publish(topic_alert, "Risk Level... Activating UV Sterilization!!");
  digitalWrite(RELAY_PIN, HIGH);
  delay(10000); // Activate for 10 seconds
  digitalWrite(RELAY_PIN, LOW);

  client.publish(topic_alert, "Sterilization done... Consume within 3 days!");
  client.publish(topic_status, "Sterilized!!");
}

void sendMQTTData() {
  if (isnan(humidity)) {
    Serial.println("Humidity is NaN, not publishing to MQTT");
    client.publish("adafruit_username/feeds/milk-spoilage.humidity", "NaN");
  } else {
    client.publish("adafruit_username/feeds/milk-spoilage.humidity", String(humidity).c_str());
  }
  client.publish("adafruit_username/feeds/milk-spoilage.temperature", String(temperature).c_str());
  client.publish("adafruit_username/feeds/milk-spoilage.ph", String(ph_value).c_str());
  client.publish("adafruit_username/feeds/milk-spoilage.gas", String(gas_level).c_str());
  client.publish("adafruit_username/feeds/milk-spoilage.risk", String(spoilage_risk).c_str());
}

// Setup Function
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  dht.begin();
  ds18b20.begin();  
  pinMode(MQ135PIN, INPUT);
  pinMode(PHPIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

// Main Loop
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Read Humidity from DHT22
  humidity = dht.readHumidity();

  // Read Temperature from DS18B20
  ds18b20.requestTemperatures();  
  temperature = ds18b20.getTempCByIndex(0);

  // Read Gas and pH Sensor Data
  gas_level = (analogRead(MQ135PIN) * 5.0 / 1023.0) * 1000;  // Convert analog to ppm
  ph_value = 7.0 - ((analogRead(PHPIN) - 512.0) * (3.5 / 512.0));

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from sensors!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  // Calculate Spoilage Risk
  spoilage_risk = calculateSpoilageRisk(temperature, humidity, ph_value, gas_level);
  Serial.printf("Temp: %.2f°C | Humidity: %.2f%% | pH: %.2f | Gas: %.2f ppm | Risk: %.2f%%\n", temperature, humidity, ph_value, gas_level, spoilage_risk);
  sendMQTTData();

  // UV Sterilization if Risk is High
  if (spoilage_risk >= 70) {
    client.publish(topic_status, "At Risk... Sterilization Activate!!");
    activateUVSterilization();
  } else {
    client.publish(topic_status, "Stable State");
  }

  delay(5000);  
}
