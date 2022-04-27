#include <Arduino.h>

/*

Curve Tracer

ver 1.0

*/

#include "config.h"
#include "mods.h"

//-------------------------------------------------------------------------
// setup
//   Arduino setup function
//-------------------------------------------------------------------------

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pin_DAC_CS, OUTPUT);

  ILI9341fast = false;
  ILI9341Begin(2, 4, 3, TFT_WID, TFT_HGT, ILI9341_Rotation3);
  BeginTouch(7, 3);

  Serial.begin(9600);
  Serial.println("Curve Tracer");

  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000UL, MSBFIRST, SPI_MODE0)); // SPI for DAC

  TurnOffLoad(tkNothing);
  DrawMenuScreen();
}

//-------------------------------------------------------------------------
// loop
//   Arduino loop function
//-------------------------------------------------------------------------

void loop(void)
{
  static unsigned long time = 0;
  TkindDUT kind;

  ExecSerialCmd();

  PrintADCs();

  if (millis() - time > 1000 && !ExecSerialTx)
  {
    kind = TestDeviceKind(curkind, false);
    if ((curkind == tkNothing) && kind != tkNothing)
    {
      delay(500); // so component is fully inserted
      // Serial.println('a');
      kind = TestDeviceKind(kind, true);
      if (kind != tkNothing)
      {
        ScanKind(kind);
      }
      curkind = kind;
    }
    else if ((curkind != tkNothing) && kind == tkNothing)
    {
      DrawMenuScreen();
      curkind = tkNothing;
    }

    if (HaveTouch() && (curkind != tkNothing))
    {
      DrawMenuScreen();
      curkind = tkNothing;
    }
    time = millis();
  }

  if (curkind == tkNothing)
  {
    MainMenuTouch();
    if (!ExecSerialTx)
      TurnOffLoad(tkNothing);
  }
}
