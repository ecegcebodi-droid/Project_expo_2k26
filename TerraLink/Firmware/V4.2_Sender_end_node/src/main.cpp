#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

void setup()
{
  Serial.begin(
    115200
  );


  delay(500);


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "       TERRALINK SENDER"
  );


  Serial.println(
    "================================"
  );


  // ==========================================================
  // LOAD PERSISTENT SOS FIRST
  // ==========================================================

  loadPersistentData();


  // ==========================================================
  // GPIO
  // ==========================================================

  pinMode(
    TOUCH_PIN,
    INPUT
  );


  pinMode(
    LOCAL_ALARM_PIN,
    INPUT_PULLUP
  );


  pinMode(
    LED_BUTTON_PIN,
    INPUT_PULLUP
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  pinMode(
    TORCH_PIN,
    OUTPUT
  );


  buzzerOff();


  digitalWrite(
    TORCH_PIN,
    LOW
  );


  // ==========================================================
  // OLED
  // ==========================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );


  oled.begin();


  oled.setPowerSave(0);


  oled.clearBuffer();


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    31,
    30,
    "TERRALINK"
  );


  oled.drawStr(
    35,
    46,
    "STARTING"
  );


  oled.sendBuffer();


  Serial.println(
    "OLED initialized."
  );


  // ==========================================================
  // GPS
  // ==========================================================

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.println(
    "GPS initialized."
  );


  // ==========================================================
  // SPI
  // ==========================================================

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );


  // ==========================================================
  // LoRa
  // ==========================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


  Serial.println(
    "Initializing LoRa..."
  );


  if (
    LoRa.begin(
      LORA_FREQUENCY
    )
  )
  {
    loraInitialized =
      true;


    Serial.println(
      "LoRa initialized successfully."
    );


    LoRa.setSyncWord(
      LORA_SYNC_WORD
    );


    LoRa.setSpreadingFactor(
      LORA_SPREADING_FACTOR
    );


    LoRa.setSignalBandwidth(
      LORA_BANDWIDTH
    );


    LoRa.setCodingRate4(
      LORA_CODING_RATE
    );


    LoRa.enableCrc();


    LoRa.setTxPower(
      LORA_TX_POWER
    );


    LoRa.receive();


    Serial.println(
      "LoRa configuration complete."
    );
  }
  else
  {
    loraInitialized =
      false;


    Serial.println(
      "ERROR: LoRa initialization FAILED."
    );
  }


  // ==========================================================
  // WAKE REASON
  // ==========================================================

  esp_sleep_wakeup_cause_t wakeReason =
    esp_sleep_get_wakeup_cause();


  if (
    wakeReason ==
    ESP_SLEEP_WAKEUP_EXT0
  )
  {
    Serial.println(
      "Woke from touch."
    );


    /*
       If an SOS is pending, do not create a new SOS.

       The existing persistent SOS is retained.
    */

    if (pendingSOS)
    {
      activeSequence =
        persistentSOSSequence;


      Serial.println(
        "Pending SOS exists after touch wake."
      );


      Serial.println(
        "Waiting for release before retry."
      );
    }


    goToState(
      STATE_WAIT_RELEASE
    );


    buzzerWake();
  }
  else if (
    wakeReason ==
    ESP_SLEEP_WAKEUP_TIMER
  )
  {
    Serial.println(
      "Woke from timer."
    );


    if (pendingSOS)
    {
      /*
         Automatic retransmission.

         No button press required.
      */

      activeSequence =
        persistentSOSSequence;


      Serial.println();
      Serial.println(
        "================================"
      );


      Serial.println(
        "AUTOMATIC SOS RETRY WAKE"
      );


      Serial.print(
        "Persistent sequence: "
      );


      Serial.println(
        persistentSOSSequence
      );


      Serial.println(
        "No user action required."
      );


      Serial.println(
        "================================"
      );


      goToState(
        STATE_SENDING
      );


      sosTransmitRequest =
        true;
    }
    else
    {
      goToState(
        STATE_AWAKENING
      );
    }


    buzzerWake();
  }
  else
  {
    /*
       Normal power-on.

       If flash contains an SOS, automatically retry it.
    */

    if (pendingSOS)
    {
      activeSequence =
        persistentSOSSequence;


      Serial.println();
      Serial.println(
        "POWER-ON WITH PENDING SOS"
      );


      goToState(
        STATE_SENDING
      );


      sosTransmitRequest =
        true;
    }
    else
    {
      goToState(
        STATE_AWAKENING
      );
    }


    buzzerWake();
  }


  // ==========================================================
  // CORE 0 LoRa TASK
  // ==========================================================

  xTaskCreatePinnedToCore(
    LoRaTask,
    "LoRaTask",
    8192,
    NULL,
    2,
    NULL,
    0
  );


  Serial.println(
    "LoRa task assigned to Core 0."
  );


  Serial.println(
    "Application running on Core 1."
  );


  Serial.println(
    "Local alarm button = GPIO33."
  );


  Serial.println(
    "Buzzer control = GPIO25."
  );


  Serial.println(
    "LED button = GPIO13."
  );


  Serial.println(
    "LED / Torch = GPIO4."
  );


  Serial.println(
    "Persistent SOS storage = ESP32 NVS."
  );


  Serial.println(
    "Automatic SOS retry = ENABLED."
  );


  Serial.println(
    "Rescue message reception ENABLED."
  );


  Serial.println(
    "Location request response ENABLED."
  );


  Serial.println(
    "Mesh routing packet support ENABLED."
  );


  Serial.println(
    "Sender routing role = END NODE; relays select next hop."
  );


  Serial.println(
    "================================"
  );
}

void loop()
{
  // ==========================================================
  // LOCAL ALARM
  // ==========================================================

  handleLocalAlarm();


  // ==========================================================
  // RESCUE MESSAGE EVENT
  // ==========================================================

  bool newRescueMessage =
    false;


  portENTER_CRITICAL(
    &timerMux
  );


  if (
    rescueMessageReceived
  )
  {
    rescueMessageReceived =
      false;


    newRescueMessage =
      true;
  }


  portEXIT_CRITICAL(
    &timerMux
  );


  if (
    newRescueMessage
  )
  {
    rescueMessageDisplayStart =
      millis();


    goToState(
      STATE_RESCUE_MESSAGE
    );


    if (
      !localAlarmPressed
    )
    {
      buzzerRescueMessage();
    }
  }


  // ==========================================================
  // LED BUTTON
  // ==========================================================

  handleLEDButton();


  // ==========================================================
  // GPS
  // ==========================================================

  updateGPS();


  // ==========================================================
  // STATE MACHINE
  // ==========================================================

  switch (
    currentState
  )
  {

    // ========================================================
    // AWAKENING
    // ========================================================

    case STATE_AWAKENING:

      if (
        millis() -
        stateStartTime >=
        AWAKENING_TIME_MS
      )
      {
        goToState(
          STATE_READY
        );
      }

      break;


    // ========================================================
    // WAIT RELEASE
    // ========================================================

    case STATE_WAIT_RELEASE:

      if (
        digitalRead(
          TOUCH_PIN
        ) == LOW
      )
      {
        /*
           If an SOS is already pending, immediately retry
           after button release.

           Otherwise return to normal READY.
        */

        if (pendingSOS)
        {
          startPersistentSOSRetry();
        }
        else
        {
          goToState(
            STATE_READY
          );
        }
      }

      break;


    // ========================================================
    // READY
    // ========================================================

    case STATE_READY:

      handleTouch();

      break;


    // ========================================================
    // HOLDING SOS
    // ========================================================

    case STATE_HOLDING_SOS:

      handleTouch();

      break;


    // ========================================================
    // SOS ACTIVATED
    // ========================================================

    case STATE_SOS_ACTIVATED:

      if (
        gpsFixAvailable
      )
      {
        /*
           For a NEW SOS, GPS is captured here.

           For a pending SOS, the original GPS remains
           persisted and is not replaced unnecessarily.
        */

        if (!pendingSOS)
        {
          persistentSOSLatitude =
            currentLatitude;


          persistentSOSLongitude =
            currentLongitude;


          persistentSOSGPSFix =
            true;
        }


        goToState(
          STATE_SENDING
        );


        requestSOS();
      }
      else if (
        millis() -
        gpsWarningStart >=
        GPS_MESSAGE_TIME_MS
      )
      {
        /*
           No GPS fix.

           SOS is STILL created and stored.
        */

        if (!pendingSOS)
        {
          persistentSOSLatitude =
            0.0;


          persistentSOSLongitude =
            0.0;


          persistentSOSGPSFix =
            false;
        }


        goToState(
          STATE_SENDING
        );


        requestSOS();
      }

      break;


    // ========================================================
    // SENDING
    // ========================================================

    case STATE_SENDING:

      if (
        millis() -
        stateStartTime >=
        SENDING_DISPLAY_MS
      )
      {
        goToState(
          STATE_WAITING_ACK
        );


        Serial.println(
          "Waiting for final rescue ACK..."
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerWaiting();
        }
      }

      break;


    // ========================================================
    // WAITING FOR ACK
    // ========================================================

    case STATE_WAITING_ACK:
    {
      bool ackReceivedNow =
        false;


      portENTER_CRITICAL(
        &timerMux
      );


      if (
        finalAckReceived
      )
      {
        finalAckReceived =
          false;


        ackReceivedNow =
          true;
      }


      portEXIT_CRITICAL(
        &timerMux
      );


      if (
        ackReceivedNow
      )
      {
        Serial.println();
        Serial.println(
          "FINAL RESCUE ACK RECEIVED."
        );


        /*
           CRITICAL:

           ACK means rescue station has received the SOS.

           Therefore remove the persistent SOS from flash.
        */

        clearPendingSOS();


        goToState(
          STATE_SOS_SUCCESS
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerSuccess();
        }
      }
      else if (
        millis() -
        stateStartTime >=
        ACK_WAIT_TIMEOUT_MS
      )
      {
        Serial.println();
        Serial.println(
          "RESCUE ACK TIMEOUT."
        );


        /*
           DO NOT clear flash.

           SOS remains pending.
        */

        Serial.println(
          "SOS REMAINS STORED IN FLASH."
        );


        Serial.println(
          "Automatic retry will occur after sleep."
        );


        goToState(
          STATE_SOS_FAILURE
        );


        if (
          !localAlarmPressed
        )
        {
          buzzerFailure();
        }
      }

      break;
    }


    // ========================================================
    // SUCCESS
    // ========================================================

    case STATE_SOS_SUCCESS:

      if (
        millis() -
        stateStartTime >=
        SUCCESS_DISPLAY_MS
      )
      {
        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // FAILURE
    // ========================================================

    case STATE_SOS_FAILURE:

      if (
        millis() -
        stateStartTime >=
        FAILURE_DISPLAY_MS
      )
      {
        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // POST SOS
    // ========================================================

    case STATE_POST_SOS:

      /*
         Manual resend is still supported.

         If the SOS is pending, the user can hold the
         touch button for 2 seconds.
      */

      handleTouch();


      /*
         IMPORTANT REFINED BEHAVIOR:

         Previously the ESP32 only slept when the touch
         condition was LOW.

         Now:

           pending SOS
                ↓
           60 sec awake period
                ↓
           deep sleep
                ↓
           timer wake
                ↓
           retransmit automatically

         The victim does not need to touch the device.
      */

      if (
        millis() -
        stateStartTime >=
        POST_SOS_AWAKE_TIME_MS
      )
      {
        enterDeepSleep();
      }

      break;


    // ========================================================
    // HOLDING RESEND
    // ========================================================

    case STATE_HOLDING_RESEND:

      handleTouch();

      break;


    // ========================================================
    // RESCUE MESSAGE
    // ========================================================

    case STATE_RESCUE_MESSAGE:

      if (
        millis() -
        rescueMessageDisplayStart >=
        RESCUE_MESSAGE_DISPLAY_MS
      )
      {
        goToState(
          STATE_POST_SOS
        );
      }

      break;


    // ========================================================
    // DEFAULT
    // ========================================================

    default:

      goToState(
        STATE_READY
      );

      break;
  }


  // ==========================================================
  // OLED
  // ==========================================================

  updateOLED();


  // ==========================================================
  // SMALL DELAY
  // ==========================================================

  delay(5);
}
