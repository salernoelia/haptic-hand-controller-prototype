// controller.ino

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

#include "M5StickCPlus2.h"
#include "M5Unified.h"
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <MPU6886.h> // Added MPU6886 library

// Initialize UDP and Web Server
WiFiUDP udp;
AsyncWebServer server(80);

// Default Configuration
struct Config {
    String ssid = "Snibs";
    String password = "Sunnyvase086";
    IPAddress oscAddress = IPAddress(192, 168, 1, 92);
    int oscPort = 50002;
    int localPort = 50001;

    // Gyroscope Offsets
    float gyroOffsetX = 0.0;
    float gyroOffsetY = 0.0;
    float gyroOffsetZ = 0.0;
} config;

// Function Declarations
bool loadConfig();
bool saveConfig();
void setupWebServer();
void handleRoot(AsyncWebServerRequest *request);
void handleConfig(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
bool connectToWiFi();
void startAP();
void reconnectWiFi();
void checkForOSC();
void setVibration(bool vibrationState);
void calibrateGyro();

// AP Credentials
const char* apSSID = "M5Stick_Config";
const char* apPassword = "config123"; // Change as needed

const int vibrationPin = 26; // Vibration motor pin

// Calibration Parameters
const int calibrationSampleCount = 500; // Number of samples for calibration
const int calibrationDelay = 10; // Delay between samples in milliseconds

// Gyro Offsets (initialized from config)
float gyroOffsetX = 0.0;
float gyroOffsetY = 0.0;
float gyroOffsetZ = 0.0;

// Orientation variables (theta and phi from example)
float theta = 0.0;
float phi = 0.0;

// MPU6886 Initialization
MPU6886 imu;

// Variables for calibration
double sumX = 0.0;
double sumY = 0.0;
double sumZ = 0.0;

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) delay(10);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    // Load Configuration
    if (!loadConfig()) {
        Serial.println("Using default configuration");
        saveConfig(); // Save default config if loading fails
    }

    // Assign gyro offsets from config
    gyroOffsetX = config.gyroOffsetX;
    gyroOffsetY = config.gyroOffsetY;
    gyroOffsetZ = config.gyroOffsetZ;

    // Initialize Wire1
    Wire1.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("Wire1 initialized.");

    // Attempt to connect to WiFi
    if (!connectToWiFi()) {
        Serial.println("Failed to connect to WiFi. Starting Access Point...");
        startAP();
    }

    // Initialize UDP for OSC
    udp.begin(config.localPort);
    Serial.println("UDP started.");

    // Setup Web Server
    setupWebServer();

    // Initialize M5StickCPlus2
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Orbitron_Light_24);
    M5.Display.setTextSize(0.5);

    // Initialize Vibration Motor
    pinMode(vibrationPin, OUTPUT);
    digitalWrite(vibrationPin, LOW);

    // Display Mode
    M5.Display.setCursor(10, 10);
    if (WiFi.getMode() & WIFI_AP) {
        M5.Display.println("AP Mode");
        M5.Display.println("SSID: " + String(apSSID));
    } else if (WiFi.getMode() & WIFI_STA) {
        M5.Display.println("Station Mode");
        M5.Display.println("IP: " + WiFi.localIP().toString());
    }

    // Display current gyro offsets
    M5.Display.println("Gyro Offsets:");
    M5.Display.printf("X: %.4f\nY: %.4f\nZ: %.4f", gyroOffsetX, gyroOffsetY, gyroOffsetZ);
    delay(3000); // Show initial offsets for 3 seconds

    // Initialize MPU6886
    Serial.println("Initializing MPU6886...");
    if (imu.Init() != 0) { // Check if initialization was successful
        Serial.println("Failed to initialize MPU6886");
        M5.Display.clear();
        M5.Display.setCursor(10, 10);
        M5.Display.println("MPU6886 Init Failed");
        while (1) { // Halt execution
            delay(1000);
        }
    } else {
        Serial.println("MPU6886 Initialized Successfully");
        M5.Display.clear();
        M5.Display.setCursor(10, 10);
        M5.Display.println("MPU6886 Initialized");
        delay(2000);
    }
}

void loop() {
    M5.update();

    // Clear Display (optional: adjust as needed)
    M5.Display.clear();

    // Read IMU Data
    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;

    // Retrieve accelerometer and gyroscope data
    imu.getAccelData(&accX, &accY, &accZ);
    imu.getGyroData(&gyroX, &gyroY, &gyroZ);

    // Debugging: Check if data is valid
    if (accX == 0.0 && accY == 0.0 && accZ == 0.0 &&
        gyroX == 0.0 && gyroY == 0.0 && gyroZ == 0.0) {
        Serial.println("IMU Data is all zeros. Check connections.");
    }

    // Apply gyro calibration offsets
    float calibratedGyroX = gyroX - gyroOffsetX;
    float calibratedGyroY = gyroY - gyroOffsetY;
    float calibratedGyroZ = gyroZ - gyroOffsetZ;

    // Calculate theta and phi as in the example
    if ((accX < 1) && (accX > -1)) {
        theta = asin(-accX) * 57.295; // Convert to degrees
    }
    if (accZ != 0) {
        phi = atan(accY / accZ) * 57.295; // Convert to degrees
    }

    // Apply simple low-pass filter (alpha = 0.2)
    float alpha = 0.2;
    static float last_theta = 0.0;
    static float last_phi = 0.0;
    theta = alpha * theta + (1 - alpha) * last_theta;
    phi = alpha * phi + (1 - alpha) * last_phi;
    last_theta = theta;
    last_phi = phi;

    // Read battery voltage
    int vol = StickCP2.Power.getBatteryVoltage();

    // Display Data
    M5.Display.setCursor(10, 10);
    if (WiFi.getMode() & WIFI_AP) {
        M5.Display.printf("AP Mode\nSSID: %s\n", apSSID);
        M5.Display.println("PWD: " + String(apPassword) + "\n");
        M5.Display.printf("IP: 192.168.4.1");
    } else {
        M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    }
    M5.Display.printf("BAT: %dmV\n", vol);
    M5.Display.printf("Accel: %.2f %.2f %.2f\n", accX, accY, accZ);
    M5.Display.printf("Gyro: %.2f %.2f %.2f\n", calibratedGyroX, calibratedGyroY, calibratedGyroZ);
    M5.Display.printf("Orientation:\nTheta: %.2f\nPhi: %.2f", theta, phi);

    // Send accelerometer data via OSC
    OSCMessage msgAccel("/accel");
    msgAccel.add(accX).add(accY).add(accZ);

    // Begin the UDP packet to the specified address and port
    udp.beginPacket(config.oscAddress, config.oscPort);
    // Send the OSC message through the UDP packet
    msgAccel.send(udp); 
    // End the UDP packet and send it
    udp.endPacket();
    msgAccel.empty();

    // Send gyroscope data via OSC
    OSCMessage msgGyro("/gyro");
    msgGyro.add(calibratedGyroX).add(calibratedGyroY).add(calibratedGyroZ);

    // Begin the UDP packet to the specified address and port
    udp.beginPacket(config.oscAddress, config.oscPort);
    // Send the OSC message through the UDP packet
    msgGyro.send(udp); 
    // End the UDP packet and send it
    udp.endPacket();
    msgGyro.empty();

    // Send orientation data via OSC
    OSCMessage msgOrient("/orientation");
    msgOrient.add(theta).add(phi); // Removed yaw as it's not calculated

    udp.beginPacket(config.oscAddress, config.oscPort);
    msgOrient.send(udp);
    udp.endPacket();
    msgOrient.empty();

    // Print data to Serial for debugging
    Serial.printf("Accel: %.2f, %.2f, %.2f\n", accX, accY, accZ);
    Serial.printf("Gyro: %.2f, %.2f, %.2f\n", calibratedGyroX, calibratedGyroY, calibratedGyroZ);
    Serial.printf("Orientation: Theta=%.2f, Phi=%.2f\n", theta, phi);

    // Check for OSC Messages
    checkForOSC();

    // Button A for Vibration Control
    if (M5.BtnA.wasPressed()) {
        Serial.println("A Btn Pressed");
        setVibration(true);
    }
    if (M5.BtnA.wasReleased()) {
        Serial.println("A Btn Released");
        setVibration(false);
    }

    // Button B for Gyro Calibration
    if (M5.BtnB.wasPressed()) {
        Serial.println("B Btn Pressed - Starting Gyro Calibration");
        calibrateGyro();
    }

    // Button C to Shutdown
    if (M5.BtnC.wasPressed()) {
        Serial.println("C Btn Pressed - Shutting Down");
        M5.Power.powerOff();
    }

    delay(100);
}

// Function to control the vibration motor
void setVibration(bool vibrationState) {
    digitalWrite(vibrationPin, vibrationState ? HIGH : LOW);
}