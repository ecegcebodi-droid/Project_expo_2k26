#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void sendMessageACK(
  int messageID,
  uint32_t sequence
)
{
  String ackPacket =
    "SRC:" + String(NODE_ID) +
    ",DST:" + String(RECEIVER_NODE_ID) +
    ",SEQ:" + String(sequence) +
    ",TTL:" + String(REVERSE_PACKET_TTL) +
    ",HOP:0" +
    ",MSGID:" + String(messageID) +
    ",TYPE:MSGACK";


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "SENDING ROUTED MESSAGE ACK"
  );


  Serial.println(
    ackPacket
  );


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    ackPacket
  );


  int result =
    LoRa.endPacket();


  if (result == 1)
    Serial.println(
      "MSGACK SENT"
    );
  else
    Serial.println(
      "MSGACK TRANSMISSION FAILED"
    );


  LoRa.receive();


  Serial.println(
    "================================"
  );
}
void sendCurrentLocation()
{
  String packet =
    "SRC:" + String(NODE_ID) +
    ",DST:" + String(RECEIVER_NODE_ID) +
    ",SEQ:" + String(activeSequence) +
    ",TTL:" + String(REVERSE_PACKET_TTL) +
    ",HOP:0";


  packet +=
    ",LAT:";


  if (gpsFixAvailable)
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


  if (gpsFixAvailable)
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
    ",TYPE:LOC";


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "SENDING ROUTED LOCATION UPDATE"
  );


  Serial.println(
    packet
  );


  LoRa.idle();


  LoRa.beginPacket();


  LoRa.print(
    packet
  );


  int result =
    LoRa.endPacket();


  if (result == 1)
    Serial.println(
      "LOCATION UPDATE SENT"
    );
  else
    Serial.println(
      "LOCATION UPDATE FAILED"
    );


  LoRa.receive();


  Serial.println(
    "================================"
  );
}
bool handleRescueMessage(
  const String &packet
)
{
  String srcString;
  String dstString;
  String seqString;
  String msgIDString;


  if (
    !getPacketField(
      packet,
      "SRC:",
      srcString
    ) ||
    !getPacketField(
      packet,
      "DST:",
      dstString
    ) ||
    !getPacketField(
      packet,
      "SEQ:",
      seqString
    ) ||
    !getPacketField(
      packet,
      "MSGID:",
      msgIDString
    )
  )
  {
    Serial.println(
      "MSG rejected - missing routing fields."
    );


    return false;
  }


  if (
    !packetHasType(
      packet,
      "MSG"
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
    Serial.println(
      "MSG ignored - wrong SRC/DST."
    );


    return false;
  }


  uint32_t messageSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  if (
    messageSequence !=
    activeSequence
  )
  {
    Serial.println(
      "MSG ignored - sequence mismatch."
    );


    return false;
  }


  int messageID =
    msgIDString.toInt();


  int msgStart =
    packet.indexOf(
      ",MSG:"
    );


  if (msgStart == -1)
  {
    Serial.println(
      "MSG rejected - no message text."
    );


    return false;
  }


  msgStart += 5;


  String message =
    packet.substring(
      msgStart
    );


  message.trim();


  if (
    message.length() == 0
  )
  {
    return false;
  }


  if (
    message.length() >
    MAX_RESCUE_MESSAGE_LENGTH
  )
  {
    message =
      message.substring(
        0,
        MAX_RESCUE_MESSAGE_LENGTH
      );
  }


  bool duplicate =
    (
      messageID ==
      lastReceivedMessageID
    ) &&
    (
      messageSequence ==
      lastReceivedMessageSequence
    );


  Serial.println();
  Serial.println(
    "================================"
  );


  Serial.println(
    "ROUTED RESCUE MESSAGE RECEIVED"
  );


  Serial.print(
    "MSG ID: "
  );


  Serial.println(
    messageID
  );


  Serial.print(
    "SEQ: "
  );


  Serial.println(
    messageSequence
  );


  Serial.print(
    "MESSAGE: "
  );


  Serial.println(
    message
  );


  Serial.println(
    "================================"
  );


  // Always ACK, including duplicates.

  sendMessageACK(
    messageID,
    messageSequence
  );


  if (duplicate)
  {
    Serial.println(
      "Duplicate rescue message."
    );


    Serial.println(
      "MSGACK resent; display suppressed."
    );


    return true;
  }


  lastReceivedMessageID =
    messageID;


  lastReceivedMessageSequence =
    messageSequence;


  portENTER_CRITICAL(
    &timerMux
  );


  message.toCharArray(
    receivedRescueMessage,
    MAX_RESCUE_MESSAGE_LENGTH + 1
  );


  receivedMessageID =
    messageID;


  receivedMessageSequence =
    messageSequence;


  rescueMessageReceived =
    true;


  portEXIT_CRITICAL(
    &timerMux
  );


  return true;
}
bool handleLocationRequest(
  const String &packet
)
{
  String srcString;
  String dstString;
  String seqString;


  if (
    !getPacketField(
      packet,
      "SRC:",
      srcString
    ) ||
    !getPacketField(
      packet,
      "DST:",
      dstString
    ) ||
    !getPacketField(
      packet,
      "SEQ:",
      seqString
    )
  )
  {
    return false;
  }


  if (
    !packetHasType(
      packet,
      "LOCREQ"
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


  uint32_t requestedSequence =
    strtoul(
      seqString.c_str(),
      NULL,
      10
    );


  if (
    requestedSequence !=
    activeSequence
  )
  {
    Serial.println(
      "LOCREQ ignored - sequence mismatch."
    );


    return false;
  }


  Serial.println(
    "ROUTED LOCATION REQUEST RECEIVED"
  );


  // Update GPS before replying.

  updateGPS();


  sendCurrentLocation();


  return true;
}
