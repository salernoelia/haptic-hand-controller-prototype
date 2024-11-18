// config.ino

// File path for configuration
const char* configPath = "/config.json";

/// Function to load configuration from SPIFFS
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
    if (size > 2048) { // Increased size to accommodate new fields
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
    DynamicJsonDocument doc(2048); // Increased size
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

    // Load Gyro Offsets
    if (doc.containsKey("gyroOffsetX") && doc.containsKey("gyroOffsetY") && doc.containsKey("gyroOffsetZ")) {
        config.gyroOffsetX = doc["gyroOffsetX"].as<float>();
        config.gyroOffsetY = doc["gyroOffsetY"].as<float>();
        config.gyroOffsetZ = doc["gyroOffsetZ"].as<float>();
        Serial.printf("Gyro Offsets Loaded - X: %.4f, Y: %.4f, Z: %.4f\n", config.gyroOffsetX, config.gyroOffsetY, config.gyroOffsetZ);
    } else {
        Serial.println("Gyro offsets not found in config. Using default (0.0, 0.0, 0.0)");
    }

    Serial.println("Configuration loaded:");
    Serial.println(doc.as<String>());

    return true;
}

// Function to save configuration to SPIFFS
bool saveConfig() {
    DynamicJsonDocument doc(2048); // Increased size
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["oscAddress"] = config.oscAddress.toString();
    doc["oscPort"] = config.oscPort;
    doc["localPort"] = config.localPort;

    // Save Gyro Offsets
    doc["gyroOffsetX"] = config.gyroOffsetX;
    doc["gyroOffsetY"] = config.gyroOffsetY;
    doc["gyroOffsetZ"] = config.gyroOffsetZ;

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
