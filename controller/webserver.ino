

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