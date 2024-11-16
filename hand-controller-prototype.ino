#include "M5StickCPlus2.h"
#include "M5Unified.h"
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiUdp.h>


const char* ssid = "Snibs";        // Replace with your network SSID
const char* password = "Sunnyvase086"; // Replace with your network password

WiFiUDP udp;
const IPAddress oscAddress(192, 168, 1, 92);
const int oscPort = 50002;
const int localPort = 50001; // Local port to receive OSC messages

void setup() {
    auto cfg = M5.config();

    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextColor(GREEN);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Orbitron_Light_24);
    M5.Display.setTextSize(1);

    pinMode(26, OUTPUT);     // Set pin 26 as output for vibration motor
    digitalWrite(26, LOW);   // Start with motor off

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // Start UDP for OSC reception
    udp.begin(localPort);
}

void loop() {
    M5.update();

    // Clear the display
    M5.Display.clear();

    // Read IMU data
    if (M5.Imu.update()) {
        auto data = M5.Imu.getImuData();

        // Display accelerometer and gyroscope data on screen
        M5.Display.setCursor(10, 10);
        M5.Display.printf("Accel:\nX: %0.2f\nY: %0.2f\nZ: %0.2f\n", data.accel.x, data.accel.y, data.accel.z);
        M5.Display.printf("Gyro:\nX: %0.2f\nY: %0.2f\nZ: %0.2f\n", data.gyro.x, data.gyro.y, data.gyro.z);

        // Send accelerometer and gyroscope data via OSC
        OSCMessage msgAccel("/accel");
        msgAccel.add(data.accel.x).add(data.accel.y).add(data.accel.z);
        udp.beginPacket(oscAddress, oscPort);
        msgAccel.send(udp);
        udp.endPacket();
        msgAccel.empty();

        OSCMessage msgGyro("/gyro");
        msgGyro.add(data.gyro.x).add(data.gyro.y).add(data.gyro.z);
        udp.beginPacket(oscAddress, oscPort);
        msgGyro.send(udp);
        udp.endPacket();
        msgGyro.empty();
    }

    // Check for incoming OSC messages to control vibration
    checkForOSC();

    // Button A controls vibration motor
    if (M5.BtnA.wasPressed()) {
        Serial.println("A Btn Pressed");
        setVibration(true); // Turn on vibration
    }
    if (M5.BtnA.wasReleased()) {
        Serial.println("A Btn Released");
        setVibration(false); // Turn off vibration
    }

     // Button B shuts down the device
    if (M5.BtnC.wasPressed()) {
        Serial.println("C Btn Pressed - Shutting Down");
        M5.Power.powerOff(); // Correct function call

    }

    

    delay(100); // Delay to avoid flooding OSC messages
}

// Function to control the vibration motor
void setVibration(bool vibrationState) {
    digitalWrite(26, vibrationState ? HIGH : LOW);
}

// Function to check for incoming OSC messages
void checkForOSC() {
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        OSCMessage msg;
        while (packetSize--) {
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
}
