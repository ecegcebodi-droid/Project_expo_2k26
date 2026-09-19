#include "keypad_manager.h"
#include "command_manager.h"
#include "config.h"

const char keyMap[4][4] =
{
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

const uint8_t rowPins[4] =
{
    KEY_ROW_1,
    KEY_ROW_2,
    KEY_ROW_3,
    KEY_ROW_4
};

const uint8_t colPins[4] =
{
    KEY_COL_1,
    KEY_COL_2,
    KEY_COL_3,
    KEY_COL_4
};

void keypadManagerBegin()
{
    for (int r = 0; r < 4; r++)
    {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }

    for (int c = 0; c < 4; c++)
        pinMode(colPins[c], INPUT_PULLUP);
}

char getKeypadKey()
{
    for (int r = 0; r < 4; r++)
    {
        for (int i = 0; i < 4; i++)
            digitalWrite(rowPins[i], HIGH);

        digitalWrite(rowPins[r], LOW);

        for (int c = 0; c < 4; c++)
        {
            if (digitalRead(colPins[c]) == LOW)
            {
                delay(20);

                if (digitalRead(colPins[c]) == LOW)
                {
                    while (digitalRead(colPins[c]) == LOW)
                        yield();

                    return keyMap[r][c];
                }
            }
        }
    }

    return '\0';
}

void processKeypadKey(char key)
{
    if (key == '\0')
        return;

    Serial.print("[KEYPAD] ");
    Serial.println(key);

    /*
       Keypad navigation is handled by command_manager.
    */

    processOperatorKey(key);
}

void keypadManagerLoop()
{
    char key = getKeypadKey();

    if (key != '\0')
        processKeypadKey(key);
}