#ifndef TERRALINK_KEYPAD_MANAGER_H
#define TERRALINK_KEYPAD_MANAGER_H

#include <Arduino.h>

void keypadManagerBegin();

void keypadManagerLoop();

char getKeypadKey();

void processKeypadKey(char key);

#endif