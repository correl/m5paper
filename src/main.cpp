#include <M5EPD.h>

void setup()
{
    disableCore0WDT();
    M5.begin();
    M5.TP.SetRotation(90);
    M5.EPD.SetRotation(90);
    M5.EPD.Clear(true);
}

void loop()
{
}
