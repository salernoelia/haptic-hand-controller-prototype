# Haptic Hand Controller

Authors: Ege Seçgin, Elia Salerno

This is a project done for the M5StickC Plus 2 ESP32 based microcontroller, that sets-up a gyro+accelerometer based hand controller and transmits the controller data via OSC.

---

# Documentation

This documentation provides an overview of the OSC Controller functionality, endpoints, and general code usage for the **M5StickC Plus 2** device.

---

## **Overview**

The code enables the M5StickC Plus 2 to:
1. Connect to Wi-Fi and send sensor data via OSC over UDP.
2. Provide a web-based interface for configuration.
3. Calibrate the onboard gyroscope for accurate sensor readings.
4. Use buttons for vibration control, calibration, and shutdown.

---

## **Hardware Requirements**

- **M5StickC Plus 2** with MPU6886 IMU
- Vibration motor connected to pin **26**
- Access to Wi-Fi network

---

## **Endpoints**

### **OSC Endpoints**

| Endpoint         | Data Sent                            | Description                     |
|------------------|--------------------------------------|---------------------------------|
| `/accel`         | `float accX, float accY, float accZ` | Accelerometer readings (X, Y, Z). |
| `/gyro`          | `float gyroX, float gyroY, float gyroZ` | Gyroscope readings with offsets applied. |
| `/orientation`   | `float theta, float phi`             | Orientation angles. |

### **Web Server Endpoints**

| HTTP Method | URL         | Description                                                   |
|-------------|-------------|---------------------------------------------------------------|
| `GET`       | `/`         | Displays a configuration form for Wi-Fi and OSC settings.     |
| `POST`      | `/configure`| Updates and saves the configuration, then reboots the device. |
| `GET`       | `/status`   | Provides the current configuration in JSON format.            |

---

## **Wi-Fi Configuration**

The device supports two modes:
1. **Station Mode (STA):** Connects to a specified Wi-Fi network.
2. **Access Point Mode (AP):** Creates its own network (`M5Stick_Config`) for configuration.

### Default AP Credentials
- **SSID:** `M5Stick_Config`
- **Password:** `config123`

---

## **Code Functionalities**

### **Initialization**
- Initializes I2C for IMU communication.
- Loads configuration from SPIFFS (saved on the device).
- Attempts Wi-Fi connection; falls back to AP mode if STA fails.
- Initializes the MPU6886 IMU.

### **OSC Communication**
- Sends accelerometer, gyroscope, and orientation data to the configured OSC server via UDP.

### **Buttons**
- **Button A:** Toggles the vibration motor.
- **Button B:** Starts gyro calibration.
- **Button C:** Powers down the device.

---

## **Gyro Calibration**

The calibration process:
1. Collects **500** samples from the gyroscope.
2. Computes the average offsets for X, Y, and Z axes.
3. Updates and saves the configuration file with the new offsets.

---

## **Configuration File**

The configuration is stored in SPIFFS as `config.json`. It contains:
- Wi-Fi credentials (`ssid`, `password`)
- OSC settings (`oscAddress`, `oscPort`, `localPort`)
- Gyro offsets (`gyroOffsetX`, `gyroOffsetY`, `gyroOffsetZ`)

---

## **How to Use**

1. **First-Time Setup:**
   - Power on the device. If it fails to connect to Wi-Fi, it enters AP mode.
   - Connect to the `M5Stick_Config` network.
   - Access the configuration page at `http://192.168.4.1/`.

2. **Regular Operation:**
   - Once configured, the device automatically connects to the saved Wi-Fi network.
   - Sensor data is sent to the OSC server specified in the configuration.

3. **Gyro Calibration:**
   - Press **Button B** to start calibration. Keep the device still during this process.

4. **Shutdown:**
   - Press **Button C** to power down the device.

---

## **OSC Integration Example**

To receive the OSC messages, use any OSC-compatible software or library (e.g., Max/MSP, Pure Data, Python OSC library). Ensure the listening port matches the `localPort` in the configuration.

---

## **Error Handling**

- **IMU Initialization Failure:** The display shows "MPU6886 Init Failed" and halts execution.
- **Wi-Fi Connection Failure:** The device starts in AP mode for configuration.
- **SPIFFS Error:** Logs an error and uses default settings.

---

## **Customizable Parameters**

| Parameter             | Location       | Default Value                  |
|-----------------------|----------------|--------------------------------|
| `I2C_SDA_PIN`         | `controller.ino` | `21`                           |
| `I2C_SCL_PIN`         | `controller.ino` | `22`                           |
| `calibrationSampleCount` | `controller.ino` | `500`                           |
| `oscPort`             | `config.json`  | `50002`                        |
| `localPort`           | `config.json`  | `50001`                        |

--- 

This documentation provides a quick reference to the code's main features and usage, enabling easy deployment and integration with OSC systems.
