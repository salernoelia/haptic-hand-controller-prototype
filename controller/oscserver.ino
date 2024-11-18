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