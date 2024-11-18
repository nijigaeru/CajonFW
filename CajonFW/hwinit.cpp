
#include "pinasign.h"
#include "hwinit.h"

void HwInit(void);

void HwInit(void)
{
    pinMode(PIN_SW1, INPUT);
    pinMode(PIN_SW2, INPUT);
    pinMode(PIN_SW3, INPUT);
    pinMode(PIN_SW4, INPUT);
    pinMode(PIN_FET1, OUTPUT);
    digitalWrite(PIN_FET1, LOW);
    pinMode(PIN_FET2, OUTPUT);
    digitalWrite(PIN_FET2, LOW);
    pinMode(PIN_FET3, OUTPUT);
    digitalWrite(PIN_FET3, LOW);
    pinMode(PIN_FET4, OUTPUT);
    digitalWrite(PIN_FET4, LOW);
    pinMode(PIN_FET5, OUTPUT);
    digitalWrite(PIN_FET5, LOW);
    pinMode(PIN_FET6, OUTPUT);
    digitalWrite(PIN_FET6, LOW);
    pinMode(PIN_FET7, OUTPUT);
    digitalWrite(PIN_FET7, LOW);
    pinMode(PIN_FET8, OUTPUT);
    digitalWrite(PIN_FET8, LOW);
}
