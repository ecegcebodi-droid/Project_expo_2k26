#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
bool getPacketField(
  const String &packet,
  const char *field,
  String &value
)
{
  String token =
    String(field);


  int searchFrom =
    0;


  while (true)
  {
    int pos =
      packet.indexOf(
        token,
        searchFrom
      );


    if (pos < 0)
      return false;


    if (
      pos == 0 ||
      packet.charAt(pos - 1) == ','
    )
    {
      int valueStart =
        pos + token.length();


      int valueEnd =
        packet.indexOf(
          ',',
          valueStart
        );


      if (valueEnd < 0)
        valueEnd =
          packet.length();


      value =
        packet.substring(
          valueStart,
          valueEnd
        );


      value.trim();


      return true;
    }


    searchFrom =
      pos + 1;
  }
}
bool packetHasType(
  const String &packet,
  const char *type
)
{
  String value;


  if (
    !getPacketField(
      packet,
      "TYPE:",
      value
    )
  )
  {
    return false;
  }


  return value.equalsIgnoreCase(
    type
  );
}
