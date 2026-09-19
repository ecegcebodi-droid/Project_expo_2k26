#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

void initializeGPS() {
    GPSserial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GPS initialized.");
}

void updateGPS() {
    while (GPSserial.available()) gps.encode(GPSserial.read());
    if (gps.location.isValid()) {
        currentLatitude = gps.location.lat();
        currentLongitude = gps.location.lng();
        gpsFixAvailable = true;
    } else {
        gpsFixAvailable = false;
    }
}
