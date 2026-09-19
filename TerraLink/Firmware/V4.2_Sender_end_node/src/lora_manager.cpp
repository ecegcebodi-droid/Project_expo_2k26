#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
bool checkForFinalACK(
  const String &received
)
{
  String srcString;
  String dstString;
  String seqString;
  String ttlString;
  String hopString;


  if (
    !getPacketField(
      received,
      "SRC:",
      srcString
    ) ||
    !getPacketField(
      received,
      "DST:",
      dstString
    ) ||
    !getPacketField(
      received,
      "SEQ:",
      seqString
    )
  )
  {
    return false;
  }


  if (
    !packetHasType(
      received,
      "ACK"
    )
  )
  {
    return false;
  }


  if (
    srcString.toInt() !=
    RECEIVER_NODE_ID ||
    dstString.toInt() !=
    NODE_ID
  )
  {
    return false;
  }


  uint32_t ackSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  /*
     CRITICAL:

     ACK must match the currently persistent SOS.

     This prevents an old ACK from clearing a new SOS.
  */

  if (
    ackSequence !=
    persistentSOSSequence
  )
  {
    Serial.println(
      "ACK ignored - sequence mismatch."
    );


    Serial.print(
      "Expected: "
    );


    Serial.println(
      persistentSOSSequence
    );


    Serial.print(
      "Received: "
    );


    Serial.println(
      ackSequence
    );


    return false;
  }


  getPacketField(
    received,
    "TTL:",
    ttlString
  );


  getPacketField(
    received,
    "HOP:",
    hopString
  );


  receivedAckSequence =
    ackSequence;


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "FINAL ROUTED RESCUE ACK CONFIRMED"
  );


  Serial.print(
    "ACK HOP: "
  );


  Serial.println(
    hopString.length()
      ? hopString
      : "unknown"
  );


  Serial.print(
    "ACK TTL remaining: "
  );


  Serial.println(
    ttlString.length()
      ? ttlString
      : "unknown"
  );


  Serial.println(
    "================================"
  );


  return true;
}
void processLoRaPacket(
  const String &received
)
{
  if (
    packetHasType(
      received,
      "MSG"
    )
  )
  {
    handleRescueMessage(
      received
    );


    return;
  }


  if (
    packetHasType(
      received,
      "LOCREQ"
    )
  )
  {
    handleLocationRequest(
      received
    );


    return;
  }


  if (
    packetHasType(
      received,
      "ACK"
    )
  )
  {
    if (
      checkForFinalACK(
        received
      )
    )
    {
      portENTER_CRITICAL(
        &timerMux
      );


      finalAckReceived =
        true;


      portEXIT_CRITICAL(
        &timerMux
      );
    }


    return;
  }


  if (
    packetHasType(
      received,
      "LOC"
    )
  )
  {
    Serial.println(
      "LOCATION packet received/looped back; ignored."
    );


    return;
  }


  if (
    packetHasType(
      received,
      "HELLO"
    )
  )
  {
    Serial.println(
      "HELLO received; sender does not maintain routing table."
    );


    return;
  }


  Serial.println(
    "Packet not recognized."
  );
}
void LoRaTask(
  void *parameter
)
{
  Serial.println(
    "LoRa task running on Core 0."
  );


  while (true)
  {
    // ========================================================
    // SOS TRANSMISSION REQUEST
    // ========================================================

    if (
      sosTransmitRequest
    )
    {
      sosTransmitRequest =
        false;


      portENTER_CRITICAL(
        &timerMux
      );


      finalAckReceived =
        false;


      portEXIT_CRITICAL(
        &timerMux
      );


      sendSOS();
    }


    // ========================================================
    // RECEIVE LoRa PACKETS
    // ========================================================

    int packetSize =
      LoRa.parsePacket();


    if (
      packetSize > 0
    )
    {
      String received = "";


      while (
        LoRa.available()
      )
      {
        received +=
          (char)LoRa.read();
      }


      received.trim();


      Serial.println();
      Serial.println(
        "--------------------------------"
      );


      Serial.print(
        "LoRa RX: "
      );


      Serial.println(
        received
      );


      Serial.print(
        "RSSI: "
      );


      Serial.println(
        LoRa.packetRssi()
      );


      Serial.println(
        "--------------------------------"
      );


      processLoRaPacket(
        received
      );


      LoRa.receive();
    }


    vTaskDelay(
      pdMS_TO_TICKS(10)
    );
  }
}
