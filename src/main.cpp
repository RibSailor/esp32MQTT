
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include <Fonts/FreeMonoBold18pt7b.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define NUMPIXELS  1
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
// Change the credentials below, so your ESP8266 connects to your router
const char* ssid0 = "ORBI20";
const char* password0 = "smoothwater684";
const char* ssid1 = "FRITZ!Box 6490 Cable";
const char* password1 = "99656955838332221415";


// MQTT broker credentials (set to NULL if not required)
const char* MQTT_username = NULL; 
const char* MQTT_password = NULL; 

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
// const char* mqtt_server = "192.168.1.150";
const char* mqtt_server = "breandown.local";


// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Boiler - Relay - GPIO5 = D5 on ESP3266 Feather board
const int boiler = 5;

String boilerState;
String currentTemp; 
String setPoint;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(1000);


  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  //tft.setFont(&FreeMonoBold18pt7b);
  tft.fillScreen(ST77XX_WHITE);
  //tft.drawRect(8, 8, tft.width()-16, tft.height()-16, ST77XX_RED);
  for (int16_t x = 0; x < 5; x++) {
    tft.drawRect(x, x, tft.width() - (x * 2), tft.height() - (x * 2), ST77XX_RED);
  }               
  tft.setTextWrap(false);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid0);

  // WiFi.begin(ssid0, password0);
  // scan....
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i)== ssid0 ) {
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ssid0);
      WiFi.begin(ssid0,password0); //trying to connect the modem
      break;
    }
    if (WiFi.SSID(i)== ssid1) {
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ssid1);
      WiFi.begin(ssid1,password1); //trying to connect the modem
      break;
    }
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(125);
}

void PrintToTFT(String boilerState, String currentTemp, String setPoint) {
  Serial.println("Printing to TFT Screen");
    tft.fillScreen(ST77XX_WHITE);
    for (int16_t x = 0; x < 5; x++) {
      tft.drawRect(x, x, tft.width() - (x * 2), tft.height() - (x * 2), ST77XX_RED);
    } 
    tft.setCursor(98, 10);
    if (boilerState == "On") {
      tft.setTextColor(ST77XX_RED);
    } else {
      tft.setTextColor(ST77XX_GREEN);
    }
    tft.setTextSize(4);
    tft.print(boilerState);
    tft.setTextColor(ST77XX_BLUE);
    tft.setCursor(10, 45);
    tft.setTextSize(6);
    tft.print(currentTemp);
    tft.print((char)9);
    tft.print("C");
    tft.setCursor(10, 92);
    tft.setTextSize(4);
    tft.print(setPoint);
    tft.print((char)9);
    tft.print("C");
}

void printBMEValues() {
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}

// This function is executed when some device publishes a message to a topic that your ESP32 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  printBMEValues();
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageState;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageState += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic caravan/switches/boiler, you check if the message is either on or off. Turns the boiler GPIO according to the message
  if(topic=="caravan/switches/boiler"){
      Serial.print("Changing boiler to ");
      if(messageState == "on"){
        digitalWrite(boiler, HIGH);
        // set color to red
        pixels.fill(0xFF0000);
        pixels.show();
        Serial.println("On");      
        boilerState = "On";
      }
      else if(messageState == "off"){
        digitalWrite(boiler, LOW);
        // set color to red
        pixels.fill(0x00FF00);
        pixels.show();
        Serial.println("Off");
        boilerState = "Off";     
      }
  }
  if(topic=="caravan/thermostat/setpoint"){
      Serial.print("Thermostat setpoint changed to ");
      Serial.println(messageState);
      setPoint = messageState;
  }
  if(topic=="caravan/room/temp"){
      Serial.print("Current caravan temperature ");
      Serial.println(messageState);      
      currentTemp = messageState;
  }
  PrintToTFT(boilerState, currentTemp, setPoint);
  Serial.println();
}

// This functions reconnects your ESP32 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP32Client", MQTT_username, MQTT_password)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("caravan/switches/boiler");
      client.subscribe("caravan/thermostat/setpoint");
      client.subscribe("caravan/room/temp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupBME() {
  unsigned BMEstatus;
  BMEstatus = bme.begin(); 

  if (!BMEstatus) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
    }
}



// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs

void setup() {
  pinMode(boiler, OUTPUT);
  digitalWrite(boiler, HIGH);
  Serial.begin(9600);
  setup_wifi();
  setupBME();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  printBMEValues();
}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {
  // printBMEValues();
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP32Client", MQTT_username, MQTT_password);

  now = millis();
  // Publishes bme values every 30 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    // Temperature in Celsius
   

    // Publishes Temperature and Humidity values
    client.publish("caravan/bme/temp", String(bme.readTemperature()).c_str());
    client.publish("caravan/bme/press", String(bme.readPressure() / 100.0F).c_str());
    client.publish("caravan/bme/alt", String(bme.readAltitude(SEALEVELPRESSURE_HPA)).c_str());
    client.publish("caravan/bme/hum", String(bme.readHumidity()).c_str());


    Serial.print("BME Temperature: ");
    Serial.print(bme.readTemperature());
    Serial.println(" ºC"); 
    
  }
} 