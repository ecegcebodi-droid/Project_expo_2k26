#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void handleLocalAlarm()
{
  localAlarmPressed =
    (digitalRead(
      LOCAL_ALARM_PIN
    ) == LOW);


  if (localAlarmPressed)
  {
    buzzerOn();
  }
  else
  {
    buzzerOff();
  }
}
void handleLEDButton()
{
  ledButtonPressed =
    (digitalRead(
      LED_BUTTON_PIN
    ) == LOW);


  if (ledButtonPressed)
  {
    digitalWrite(
      TORCH_PIN,
      HIGH
    );
  }
  else
  {
    digitalWrite(
      TORCH_PIN,
      LOW
    );
  }
}
void handleTouch()
{
  bool pressed =
    digitalRead(
      TOUCH_PIN
    ) == HIGH;


  // ==========================================================
  // READY -> HOLDING SOS
  // ==========================================================

  if (
    currentState ==
    STATE_READY
  )
  {
    if (
      pressed &&
      !touchWasPressed
    )
    {
      touchStartTime =
        millis();


      sosHoldTriggered =
        false;


      goToState(
        STATE_HOLDING_SOS
      );


      Serial.println(
        "SOS hold started."
      );
    }
  }


  // ==========================================================
  // HOLDING SOS
  // ==========================================================

  else if (
    currentState ==
    STATE_HOLDING_SOS
  )
  {
    if (!pressed)
    {
      Serial.println(
        "SOS hold cancelled."
      );


      goToState(
        STATE_READY
      );
    }
    else
    {
      if (
        !sosHoldTriggered &&
        (
          millis() -
          touchStartTime >=
          SOS_LONG_PRESS_MS
        )
      )
      {
        sosHoldTriggered =
          true;


        goToState(
          STATE_SOS_ACTIVATED
        );


        buzzerSOSActivated();


        gpsWarningStart =
          millis();


        Serial.println(
          "SOS 2-second hold completed."
        );
      }
    }
  }


  // ==========================================================
  // POST SOS -> HOLDING RESEND
  // ==========================================================

  else if (
    currentState ==
    STATE_POST_SOS
  )
  {
    if (
      pressed &&
      !touchWasPressed
    )
    {
      touchStartTime =
        millis();


      sosHoldTriggered =
        false;


      goToState(
        STATE_HOLDING_RESEND
      );


      Serial.println(
        "Resend hold started."
      );
    }
  }


  // ==========================================================
  // HOLDING RESEND
  // ==========================================================

  else if (
    currentState ==
    STATE_HOLDING_RESEND
  )
  {
    if (!pressed)
    {
      Serial.println(
        "Resend cancelled."
      );


      goToState(
        STATE_POST_SOS
      );
    }
    else
    {
      if (
        !sosHoldTriggered &&
        (
          millis() -
          touchStartTime >=
          SOS_LONG_PRESS_MS
        )
      )
      {
        sosHoldTriggered =
          true;


        Serial.println(
          "Resend 2-second hold completed."
        );


        buzzerSOSActivated();


        /*
           Manual resend of the same active SOS.

           The persistent sequence is preserved.
        */

        goToState(
          STATE_SOS_ACTIVATED
        );


        gpsWarningStart =
          millis();
      }
    }
  }


  else if (
    currentState ==
    STATE_RESCUE_MESSAGE
  )
  {
    // No touch action.
  }


  touchWasPressed =
    pressed;
}
