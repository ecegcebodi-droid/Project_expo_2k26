#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void loadPersistentData()
{
  preferences.begin(
    "terralink",
    false
  );


  sequenceNumber =
    preferences.getUInt(
      "sequence",
      0
    );


  pendingSOS =
    preferences.getBool(
      "pending",
      false
    );


  if (pendingSOS)
  {
    persistentSOSSequence =
      preferences.getUInt(
        "sos_seq",
        0
      );


    persistentSOSLatitude =
      preferences.getDouble(
        "lat",
        0.0
      );


    persistentSOSLongitude =
      preferences.getDouble(
        "lon",
        0.0
      );


    persistentSOSGPSFix =
      preferences.getBool(
        "gpsfix",
        false
      );


    activeSequence =
      persistentSOSSequence;


    Serial.println();
    Serial.println(
      "================================"
    );
    Serial.println(
      "PERSISTENT SOS FOUND IN FLASH"
    );
    Serial.print(
      "Sequence: "
    );
    Serial.println(
      persistentSOSSequence
    );


    if (persistentSOSGPSFix)
    {
      Serial.print(
        "Stored LAT: "
      );
      Serial.println(
        persistentSOSLatitude,
        6
      );


      Serial.print(
        "Stored LON: "
      );
      Serial.println(
        persistentSOSLongitude,
        6
      );
    }
    else
    {
      Serial.println(
        "Stored GPS: NO FIX"
      );
    }


    Serial.println(
      "SOS WILL BE RETRANSMITTED"
    );


    Serial.println(
      "================================"
    );
  }
  else
  {
    Serial.println(
      "No pending SOS in flash."
    );
  }
}
void saveSequenceNumber()
{
  preferences.putUInt(
    "sequence",
    sequenceNumber
  );
}
void savePendingSOS()
{
  preferences.putBool(
    "pending",
    true
  );


  preferences.putUInt(
    "sos_seq",
    persistentSOSSequence
  );


  preferences.putDouble(
    "lat",
    persistentSOSLatitude
  );


  preferences.putDouble(
    "lon",
    persistentSOSLongitude
  );


  preferences.putBool(
    "gpsfix",
    persistentSOSGPSFix
  );


  pendingSOS =
    true;


  Serial.println();
  Serial.println(
    "SOS SAVED TO ESP32 FLASH"
  );


  Serial.print(
    "Persistent sequence = "
  );


  Serial.println(
    persistentSOSSequence
  );
}
void clearPendingSOS()
{
  preferences.putBool(
    "pending",
    false
  );


  preferences.remove(
    "sos_seq"
  );


  preferences.remove(
    "lat"
  );


  preferences.remove(
    "lon"
  );


  preferences.remove(
    "gpsfix"
  );


  pendingSOS =
    false;


  persistentSOSSequence =
    0;


  Serial.println();
  Serial.println(
    "================================"
  );
  Serial.println(
    "PERSISTENT SOS CLEARED"
  );
  Serial.println(
    "FINAL ACK CONFIRMED"
  );
  Serial.println(
    "================================"
  );
}
