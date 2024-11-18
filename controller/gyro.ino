// gyro.ino

// Function to perform gyro calibration
void calibrateGyro() {
    M5.Display.clear();
    M5.Display.setCursor(10, 10);
    M5.Display.println("Calibrating");
    M5.Display.println("Please keep still...");

    // Reset sums
    sumX = 0.0;
    sumY = 0.0;
    sumZ = 0.0;

    // Collect samples
    for (int i = 0; i < calibrationSampleCount; i++) {
        float accX, accY, accZ;
        float gyroX, gyroY, gyroZ;
        imu.getAccelData(&accX, &accY, &accZ);
        imu.getGyroData(&gyroX, &gyroY, &gyroZ);

        sumX += gyroX;
        sumY += gyroY;
        sumZ += gyroZ;

        delay(calibrationDelay);
        // Optional: Update progress on display
        if (i % 100 == 0) {
            M5.Display.printf("Calibrating... %d/%d\n", i, calibrationSampleCount);
        }
    }

    // Calculate average offsets
    gyroOffsetX = sumX / calibrationSampleCount;
    gyroOffsetY = sumY / calibrationSampleCount;
    gyroOffsetZ = sumZ / calibrationSampleCount;

    // Update configuration
    config.gyroOffsetX = gyroOffsetX;
    config.gyroOffsetY = gyroOffsetY;
    config.gyroOffsetZ = gyroOffsetZ;

    // Save configuration
    if (saveConfig()) {
        M5.Display.clear();
        M5.Display.setCursor(10, 10);
        M5.Display.println("Calibration Successful");
        M5.Display.printf("Offsets:\nX: %.4f\nY: %.4f\nZ: %.4f", gyroOffsetX, gyroOffsetY, gyroOffsetZ);
    } else {
        M5.Display.clear();
        M5.Display.setCursor(10, 10);
        M5.Display.println("Calibration Failed");
    }

    delay(3000); // Display the result for 3 seconds
}