#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
String createSOSPacket()
{
  String packet;

  packet.reserve(180);


  packet +=
    "SRC:";

  packet +=
    String(NODE_ID);


  packet +=
    ",DST:";

  packet +=
    String(BROADCAST_ID);


  packet +=
    ",SEQ:";

  packet +=
    String(activeSequence);


  packet +=
    ",TTL:";

  packet +=
    String(SOS_INITIAL_TTL);


  packet +=
    ",HOP:0";


  packet +=
    ",LAT:";


  if (pendingSOS)
  {
    packet +=
      String(
        persistentSOSLatitude,
        6
      );
  }
  else if (gpsFixAvailable)
  {
    packet +=
      String(
        currentLatitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",LON:";


  if (pendingSOS)
  {
    packet +=
      String(
        persistentSOSLongitude,
        6
      );
  }
  else if (gpsFixAvailable)
  {
    packet +=
      String(
        currentLongitude,
        6
      );
  }
  else
  {
    packet +=
      "0.000000";
  }


  packet +=
    ",TYPE:SOS";


  return packet;
}
void sendSOS()
{
  if (!loraInitialized)
  {
    Serial.println(
      "ERROR: LoRa not initialized."
    );

    return;
  }


  String packet =
    createSOSPacket();


  Serial.println();
  Serial.println(
    "================================"
  );
  Serial.println(
    "TERRALINK ROUTED SOS TRANSMISSION"
  );


  Serial.print(
    "Packet: "
  );


  Serial.println(
    packet
  );


  Serial.print(
    "Sequence: "
  );


  Serial.println(
    activeSequence
  );


  Serial.println(
    "Destination: BROADCAST / MESH"
  );


  Serial.print(
    "Initial TTL: "
  );


  Serial.println(
    SOS_INITIAL_TTL
  );


  Serial.println(
    "Hop: 0 (sender)"
  );


  if (
    pendingSOS &&
    persistentSOSGPSFix
  )
  {
    Serial.print(
      "Stored GPS Latitude: "
    );


    Serial.println(
      persistentSOSLatitude,
      6
    );


    Serial.print(
      "Stored GPS Longitude: "
    );


    Serial.println(
      persistentSOSLongitude,
      6
    );
  }
  else if (gpsFixAvailable)
  {
    Serial.print(
      "GPS Latitude: "
    );


    Serial.println(
      currentLatitude,
      6
    );


    Serial.print(
      "GPS Longitude: "
    );


    Serial.println(
      currentLongitude,
      6
    );
  }
  else
  {
    Serial.println(
      "GPS: NO FIX"
    );


    Serial.println(
      "LAT/LON = 0.000000"
    );


    Serial.println(
      "SOS transmission NOT blocked."
    );
  }


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    packet
  );


  int result =
    LoRa.endPacket();


  if (result == 1)
  {
    Serial.println(
      "Routed SOS transmitted successfully."
    );
  }
  else
  {
    Serial.println(
      "SOS transmission failed."
    );
  }


  LoRa.receive();


  Serial.println(
    "LoRa returned to RX mode."
  );


  Serial.println(
    "================================"
  );


  Serial.println();
}
void requestSOS()
{
  /*
     IMPORTANT:

     If this is a NEW SOS, create a NEW sequence.

     If this is a retransmission of a persistent SOS,
     DO NOT create a new sequence.
  */

  if (!pendingSOS)
  {
    sequenceNumber++;


    // Avoid sequence 0.

    if (
      sequenceNumber == 0
    )
    {
      sequenceNumber = 1;
    }


    saveSequenceNumber();


    activeSequence =
      sequenceNumber;


    persistentSOSSequence =
      activeSequence;


    persistentSOSLatitude =
      gpsFixAvailable
        ? currentLatitude
        : 0.0;


    persistentSOSLongitude =
      gpsFixAvailable
        ? currentLongitude
        : 0.0;


    persistentSOSGPSFix =
      gpsFixAvailable;


    /*
       SAVE BEFORE TRANSMISSION.

       This is critical.

       If power disappears immediately after this point,
       the SOS still exists in flash.
    */

    savePendingSOS();


    Serial.println();
    Serial.println(
      "NEW SOS CREATED AND PERSISTED."
    );
  }
  else
  {
    /*
       Persistent retransmission.

       Same sequence number.
    */

    activeSequence =
      persistentSOSSequence;


    Serial.println();
    Serial.println(
      "RETRANSMITTING PERSISTENT SOS."
    );


    Serial.print(
      "Sequence = "
    );


    Serial.println(
      activeSequence
    );
  }


  Serial.println();


  if (persistentSOSGPSFix)
  {
    Serial.println(
      "GPS LOCATION STORED."
    );
  }
  else
  {
    Serial.println(
      "GPS FIX NOT AVAILABLE."
    );


    Serial.println(
      "SOS WILL STILL BE TRANSMITTED."
    );
  }


  buzzerSending();


  sosTransmitRequest =
    true;
}
void startPersistentSOSRetry()
{
  if (!pendingSOS)
  {
    return;
  }


  activeSequence =
    persistentSOSSequence;


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "PERSISTENT SOS RETRY"
  );


  Serial.print(
    "Sequence: "
  );


  Serial.println(
    activeSequence
  );


  Serial.println(
    "Flash record retained until ACK."
  );


  Serial.println(
    "================================"
  );


  /*
     We don't generate a new sequence.

     We retransmit the existing SOS.
  */

  goToState(
    STATE_SENDING
  );


  sosTransmitRequest =
    true;
}
