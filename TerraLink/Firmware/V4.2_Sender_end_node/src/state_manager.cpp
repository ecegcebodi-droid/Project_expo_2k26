#include <Arduino.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"
void goToState(
  SystemState newState
)
{
  currentState =
    newState;


  stateStartTime =
    millis();


  lastUIUpdate =
    0;
}
