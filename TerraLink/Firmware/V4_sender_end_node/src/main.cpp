#include <Arduino.h>
#include "esp_sleep.h"
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

static void initializeSystem() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n================================");
    Serial.println("       TERRALINK SENDER");
    Serial.println("================================");

    loadPersistentData();
    initializeInputs();
    initializeBuzzer();
    initializeDisplay();
    initializeGPS();
    initializeLoRa();

    const esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
    if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke from touch.");
        if (pendingSOS) { activeSequence = persistentSOSSequence; Serial.println("Pending SOS exists after touch wake."); Serial.println("Waiting for release before retry."); }
        goToState(STATE_WAIT_RELEASE);
        buzzerWake();
    } else if (wakeReason == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("Woke from timer.");
        if (pendingSOS) {
            activeSequence = persistentSOSSequence;
            Serial.println("\n================================");
            Serial.println("AUTOMATIC SOS RETRY WAKE");
            Serial.print("Persistent sequence: "); Serial.println(persistentSOSSequence);
            Serial.println("No user action required.");
            Serial.println("================================");
            goToState(STATE_SENDING);
            sosTransmitRequest = true;
        } else goToState(STATE_AWAKENING);
        buzzerWake();
    } else {
        if (pendingSOS) {
            activeSequence = persistentSOSSequence;
            Serial.println("\nPOWER-ON WITH PENDING SOS");
            goToState(STATE_SENDING);
            sosTransmitRequest = true;
        } else goToState(STATE_AWAKENING);
        buzzerWake();
    }

    xTaskCreatePinnedToCore(LoRaTask, "LoRaTask", 8192, nullptr, 2, nullptr, 0);
    Serial.println("LoRa task assigned to Core 0.");
    Serial.println("Application running on Core 1.");
    Serial.println("Local alarm button = GPIO33.");
    Serial.println("Buzzer control = GPIO25.");
    Serial.println("LED button = GPIO13.");
    Serial.println("LED / Torch = GPIO4.");
    Serial.println("Persistent SOS storage = ESP32 NVS.");
    Serial.println("Automatic SOS retry = ENABLED.");
    Serial.println("Rescue message reception = ENABLED.");
    Serial.println("OLED rescue message display = ENABLED.");
    Serial.println("Rescue message sound indication = ENABLED.");
    Serial.println("Optional SOURCE field = ENABLED.");
    Serial.println("Optional PRIORITY field = ENABLED.");
    Serial.println("Location request response = ENABLED.");
    Serial.println("Mesh routing packet support = ENABLED.");
    Serial.println("Sender routing role = END NODE; relays select next hop.");
    Serial.println("================================");
}

void setup() { initializeSystem(); }

void loop() {
    handleLocalAlarm();

    if (popNextRescueMessage()) {
        rescueMessageDisplayStart = millis();
        goToState(STATE_RESCUE_MESSAGE);
        if (!localAlarmPressed) buzzerPriorityMessage(receivedMessagePriority);
    }

    handleLEDButton();
    updateGPS();

    switch (currentState) {
        case STATE_AWAKENING:
            if (millis()-stateStartTime >= AWAKENING_TIME_MS) goToState(STATE_READY);
            break;

        case STATE_WAIT_RELEASE:
            if (digitalRead(TOUCH_PIN) == LOW) {
                if (pendingSOS) startPersistentSOSRetry(); else goToState(STATE_READY);
            }
            break;

        case STATE_READY:
        case STATE_HOLDING_SOS:
            handleTouch();
            break;

        case STATE_SOS_ACTIVATED:
            if (gpsFixAvailable) {
                if (!pendingSOS) {
                    persistentSOSLatitude = currentLatitude;
                    persistentSOSLongitude = currentLongitude;
                    persistentSOSGPSFix = true;
                }
                goToState(STATE_SENDING);
                requestSOS();
            } else if (millis()-gpsWarningStart >= GPS_MESSAGE_TIME_MS) {
                if (!pendingSOS) {
                    persistentSOSLatitude = 0.0;
                    persistentSOSLongitude = 0.0;
                    persistentSOSGPSFix = false;
                }
                goToState(STATE_SENDING);
                requestSOS();
            }
            break;

        case STATE_SENDING:
            if (millis()-stateStartTime >= SENDING_DISPLAY_MS) {
                goToState(STATE_WAITING_ACK);
                Serial.println("Waiting for final rescue ACK...");
                if (!localAlarmPressed) buzzerWaiting();
            }
            break;

        case STATE_WAITING_ACK: {
            bool ackReceivedNow = false;
            portENTER_CRITICAL(&timerMux);
            if (finalAckReceived) { finalAckReceived = false; ackReceivedNow = true; }
            portEXIT_CRITICAL(&timerMux);
            if (ackReceivedNow) {
                Serial.println("\nFINAL RESCUE ACK RECEIVED.");
                clearPendingSOS();
                goToState(STATE_SOS_SUCCESS);
                if (!localAlarmPressed) buzzerSuccess();
            } else if (millis()-stateStartTime >= ACK_WAIT_TIMEOUT_MS) {
                Serial.println("\nRESCUE ACK TIMEOUT.");
                Serial.println("SOS REMAINS STORED IN FLASH.");
                Serial.println("Automatic retry will occur after sleep.");
                goToState(STATE_SOS_FAILURE);
                if (!localAlarmPressed) buzzerFailure();
            }
            break;
        }

        case STATE_SOS_SUCCESS:
            if (millis()-stateStartTime >= SUCCESS_DISPLAY_MS) goToState(STATE_POST_SOS);
            break;

        case STATE_SOS_FAILURE:
            if (millis()-stateStartTime >= FAILURE_DISPLAY_MS) goToState(STATE_POST_SOS);
            break;

        case STATE_POST_SOS:
            handleTouch();
            if (millis()-stateStartTime >= POST_SOS_AWAKE_TIME_MS) enterDeepSleep();
            break;

        case STATE_HOLDING_RESEND:
            handleTouch();
            break;

        case STATE_RESCUE_MESSAGE:
            if (millis()-rescueMessageDisplayStart >= RESCUE_MESSAGE_DISPLAY_MS) goToState(STATE_POST_SOS);
            break;
    }

    updateOLED();
    delay(5);
}
