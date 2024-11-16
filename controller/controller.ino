#include "M5StickCPlus2.h"
#include "M5Unified.h"
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// Default Configuration
struct Config {
    String ssid = "Snibs";
    String password = "Sunnyvase086";
    IPAddress oscAddress = IPAddress(192, 168, 1, 92);
    int oscPort = 50002;
    int localPort = 50001;
} config;

// File path for configuration
const char* configPath = "/config.json";

// Initialize UDP and Web Server
WiFiUDP udp;
AsyncWebServer server(80);

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

// AP Credentials
const char* apSSID = "M5Stick_Config";
const char* apPassword = "config123"; // Change as needed

const int vibrationPin = 26; // Vibration motor pin

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

    // Attempt to connect to WiFi
    if (!connectToWiFi()) {
        Serial.println("Failed to connect to WiFi. Starting Access Point...");
        startAP();
    }

    // Initialize UDP for OSC
    udp.begin(config.localPort);

    // Setup Web Server
    setupWebServer();

    // Initialize M5StickCPlus2
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Orbitron_Light_24);
    M5.Display.setTextSize(1);

    // Initialize Vibration Motor
    pinMode(vibrationPin, OUTPUT);
    digitalWrite(vibrationPin, LOW);

    // Display Mode
    if (WiFi.getMode() & WIFI_AP) {
        M5.Display.setCursor(10, 10);
        M5.Display.println("AP Mode");
        M5.Display.println("SSID: " + String(apSSID));
    } else if (WiFi.getMode() & WIFI_STA) {
        M5.Display.setCursor(10, 10);
        M5.Display.println("Station Mode");
        M5.Display.println("IP: " + WiFi.localIP().toString());
    }
}

void loop() {
    M5.update();

    // Clear Display (optional: adjust as needed)
    M5.Display.clear();

    // Read IMU Data
    if (M5.Imu.update()) {
        auto data = M5.Imu.getImuData();
        int vol = StickCP2.Power.getBatteryVoltage();

        // Display Data
        M5.Display.setCursor(10, 10);
        if (WiFi.getMode() & WIFI_AP) {
            M5.Display.printf("AP Mode\nSSID: %s\n", apSSID);
        } else {
            M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        }
        M5.Display.printf("BAT: %dmV\n", vol);
        M5.Display.printf("Accel: %.2f %.2f %.2f\n", data.accel.x, data.accel.y, data.accel.z);
        M5.Display.printf("Gyro: %.2f %.2f %.2f", data.gyro.x, data.gyro.y, data.gyro.z);

        // Send accelerometer data via OSC
        OSCMessage msgAccel("/accel");
        msgAccel.add(data.accel.x).add(data.accel.y).add(data.accel.z);

        // Begin the UDP packet to the specified address and port
        udp.beginPacket(config.oscAddress, config.oscPort);
        // Send the OSC message through the UDP packet
        msgAccel.send(udp); 
        // End the UDP packet and send it
        udp.endPacket();
        msgAccel.empty();

        // Send gyroscope data via OSC
        OSCMessage msgGyro("/gyro");
        msgGyro.add(data.gyro.x).add(data.gyro.y).add(data.gyro.z);

        // Begin the UDP packet to the specified address and port
        udp.beginPacket(config.oscAddress, config.oscPort);
        // Send the OSC message through the UDP packet
        msgGyro.send(udp); 
        // End the UDP packet and send it
        udp.endPacket();
        msgGyro.empty();
    }

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

    // Button C to Shutdown
    if (M5.BtnC.wasPressed()) {
        Serial.println("C Btn Pressed - Shutting Down");
        M5.Power.powerOff();
    }

    delay(50);
}

// Function to load configuration from SPIFFS
bool loadConfig() {
    if (!SPIFFS.exists(configPath)) {
        Serial.println("Config file does not exist");
        return false;
    }

    File file = SPIFFS.open(configPath, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }

    size_t size = file.size();
    if (size > 1024) {
        Serial.println("Config file size is too large");
        file.close();
        return false;
    }

    // Allocate buffer
    std::unique_ptr<char[]> buf(new char[size + 1]);
    file.readBytes(buf.get(), size);
    buf[size] = '\0';
    file.close();

    // Parse JSON
    DynamicJsonDocument doc(1024);
    auto error = deserializeJson(doc, buf.get());
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }

    // Assign to config
    config.ssid = doc["ssid"].as<String>();
    config.password = doc["password"].as<String>();
    String oscIP = doc["oscAddress"].as<String>();
    config.oscAddress.fromString(oscIP);
    config.oscPort = doc["oscPort"].as<int>();
    config.localPort = doc["localPort"].as<int>();

    Serial.println("Configuration loaded:");
    Serial.println(doc.as<String>());

    return true;
}

// Function to save configuration to SPIFFS
bool saveConfig() {
    DynamicJsonDocument doc(1024);
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["oscAddress"] = config.oscAddress.toString();
    doc["oscPort"] = config.oscPort;
    doc["localPort"] = config.localPort;

    File file = SPIFFS.open(configPath, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to config file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Configuration saved");
    return true;
}

// Function to setup the web server
void setupWebServer() {
    // Serve Configuration Form
    server.on("/", HTTP_GET, handleRoot);

    // Handle Form Submission
    server.on("/configure", HTTP_POST, handleConfig);

    // Serve Status Page
    server.on("/status", HTTP_GET, handleStatus);

    // Start Server
    server.begin();
    Serial.println("Web server started");
}

// Handler for root (configuration form)
void handleRoot(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><title>Configuration</title></head><body>";
    html += "<h2>Configure Device</h2>";
    html += "<form action='/configure' method='post'>";
    html += "WiFi SSID:<br><input type='text' name='ssid' value='" + config.ssid + "' required><br>";
    html += "WiFi Password:<br><input type='password' name='password' value='" + config.password + "' required><br>";
    html += "OSC IP Address:<br><input type='text' name='oscAddress' value='" + config.oscAddress.toString() + "' required><br>";
    html += "OSC Port:<br><input type='number' name='oscPort' value='" + String(config.oscPort) + "' required><br>";
    html += "Listening Port:<br><input type='number' name='localPort' value='" + String(config.localPort) + "' required><br><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";

    request->send(200, "text/html", html);
}

// Handler for configuration form submission
void handleConfig(AsyncWebServerRequest *request) {
    // Check if all required parameters are present
    if (request->hasParam("ssid", true) && request->hasParam("password", true) &&
        request->hasParam("oscAddress", true) && request->hasParam("oscPort", true) &&
        request->hasParam("localPort", true)) {

        config.ssid = request->getParam("ssid", true)->value();
        config.password = request->getParam("password", true)->value();
        String oscIP = request->getParam("oscAddress", true)->value();
        config.oscAddress.fromString(oscIP);
        config.oscPort = request->getParam("oscPort", true)->value().toInt();
        config.localPort = request->getParam("localPort", true)->value().toInt();

        // Save configuration
        if (saveConfig()) {
            // Send response before restarting
            request->send(200, "text/html", "<!DOCTYPE html><html><head><title>Success</title></head><body><h2>Configuration Saved. Rebooting...</h2></body></html>");

            // Allow time for response to be sent
            delay(1000);

            // Restart device to apply new settings
            ESP.restart();
            return;
        } else { 
            request->send(500, "text/plain", "Failed to save configuration");
            return;
        }
    }

    // Missing parameters
    request->send(400, "text/plain", "Bad Request: Missing Parameters");
}

// Handler for status page
void handleStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["oscAddress"] = config.oscAddress.toString();
    doc["oscPort"] = config.oscPort;
    doc["localPort"] = config.localPort;

    String json;
    serializeJsonPretty(doc, json);
    request->send(200, "application/json", json);
}

// Function to connect to WiFi in Station Mode
bool connectToWiFi() {
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(config.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    // Attempt to connect for 10 seconds
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        return false;
    }
}

// Function to start Access Point
void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Optionally, you can set up a captive portal or display AP info
}

// Function to reconnect WiFi (not used in this version, kept for reference)
void reconnectWiFi() {
    // This function can be used to attempt reconnection if needed
}

// Function to control the vibration motor
void setVibration(bool vibrationState) {
    digitalWrite(vibrationPin, vibrationState ? HIGH : LOW);
}

// Function to check for incoming OSC messages
void checkForOSC() {
    OSCMessage msg;
    int size = udp.parsePacket();
    while (size--) {
        msg.fill(udp.read());
    }
    if (!msg.hasError()) {
        if (msg.match("/vibrate")) {
            int vibrationCommand = msg.getInt(0);
            if (vibrationCommand == 1) {
                setVibration(true);
                Serial.println("Vibration ON (from OSC)");
            } else if (vibrationCommand == 0) {
                setVibration(false);
                Serial.println("Vibration OFF (from OSC)");
            }
        }
    }
}
