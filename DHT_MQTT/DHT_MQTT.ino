#include "definitions.h"
#include "DHTesp.h"

#ifdef DISPLAYAVAIL
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#endif

//WiFiClient espClient;
//PubSubClient mqttClient(espClient);
DHTesp dht;  //DHT22
long lastMsg = 0;

int dhtPin = 14;

// Initialize the OLED display using Wire library
#ifdef DISPLAYAVAIL
SSD1306  display(0x3C, D2, D1); // (I2C Address, SCL Pin, SDA Pin)
#endif

// Initialising the display.
void setupDisplay()
{
#ifdef DISPLAYAVAIL
  display.init(); pinMode(dhtPin, INPUT);          // set pin to input
  digitalWrite(dhtPin, HIGH);       // turn on pullup resistors
  dht.setup(dhtPin); // data pin 14
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.display();
  drawSplashImage();
  delay(1000);
#endif
}

// Setup Mqtt
void setupMqtt()
{
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
}

// Print Debug Messages
void printStatusMessages(float T, float H)
{
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
}

//Main Setup Routine
void setup()
{
  Serial.begin(115200);
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");  //get the current time
  setup_wifi();
  setupMqtt();
  setupDisplay();


  //ESP.deepSleep(FREQUENCYOFREADING);
}


void loop()
{
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  long now = millis();
  if (now - lastMsg > FREQUENCYOFREADING) {
    // Get the current Time
    time_t currentTime = time(nullptr);
    //  Get the humidity and temp
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature() + CALTEMP;
    displayData(temperature, humidity, 1);

    //Add the Sensor data to be sent by MQQT
    int n = 2;
    String sType[n], sUOM[n];
    float sValue[n];
    for (int i = 0; i < n; i++) {
      if (i == 0) {
        sType[i] = 'T';
        sValue[i] = temperature;
        sUOM[i] = 'D';
      }
      if (i == 1) {
        sType[1] = 'H';
        sValue[1] = humidity;
        sUOM[1] = 'P';
      }
    }
    sendJS(n, sType, sValue, sUOM);
    lastMsg = now;
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// The callback is for receiving messages back from other sources
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

// Reconnect to the network if connection is lost
void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("TT", "Temperature and Humidity");
      // ... and resubscribe
      mqttClient.subscribe("TT");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Send the data to the OLED screen
void displayData(double lT, double lH, int tempType) {
#ifdef DISPLAYAVAIL
  //Format to string
  char tempToDisplay[10];
  char humid[10];
  char TMessage[20];
  char HMessage[20];
  //  Change this to go fahrenheit or Centrigrade
  if (tempType == 1) {
    dtostrf(dht.toFahrenheit(lT), 7, 2, tempToDisplay);
  } else {
    dtostrf(lT, 7, 2, tempToDisplay);
  }

  dtostrf(lH, 3, 0, humid);
  snprintf (TMessage, 20, "%s °F", tempToDisplay);
  snprintf (HMessage, 20, "%s%sH", humid, "%");

  //Display Message on screen
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(66, 0, "Troy Telematics");
  display.drawString(66, 16, "Temp/Humidty");
  display.setFont(ArialMT_Plain_16);
  display.drawString(66, 32, TMessage);
  display.drawString(66, 48, HMessage);
  display.display();
#endif
}


void drawSplashImage() {
#ifdef DISPLAYAVAIL
  //display.drawXbm(32, 14, clouds_width, clouds_height, cloud_bits);
  //display.drawXbm(32, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  //drawFastImage(32, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  //Library updated and images arent working anymore
  display.display();
#endif
}

