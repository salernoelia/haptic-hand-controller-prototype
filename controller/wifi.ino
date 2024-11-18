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
}

// Function to reconnect WiFi (not used in this version, kept for reference)
void reconnectWiFi() {
    
}