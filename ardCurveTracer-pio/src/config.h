
#include <SimpleILI9341.h>

#include <avr/pgmspace.h>

#include <SPI.h>

int val = 0;
byte i = 0;
int prev_x = 0, prev_y = 0;
bool ExecSerialTx = false;                              // send scans to PC
bool SendAdcValues = false;                             // send ADC values to PC
int minYposGain, maxYposGain, minBaseGain, maxBaseGain; // used when calc gain

const int TFT_WID = 320;
const int TFT_HGT = 240;
const int ADC_MAX = 1024;

const int pin_DAC_CS = 10;
const int pin_ADC_NPN_Vcc = 0;
const int pin_ADC_NPN_Vce = 1;
const int pin_ADC_PNP_Vce = 2;
const int pin_ADC_PNP_Vcc = 3;
const int pin_Adc_12V = 7;
const int pin_Adc_Bat = 6;

const int R1 = 33;  // ADC input potential divider upper resistor k-ohms
const int R2 = 47;  // ADC input potential divider lower resistor k-ohms
const int R3 = 100; // collector resistor ohms
const int R4 = 68;  // DAC op amp feedback upper resistor k-ohms
const int R5 = 33;  // DAC op amp feedback lower resistor k-ohms
const int R6 = 10;  // measure battery volts potential divider upper resistor k-ohms
const int R7 = 33;  // measure battery volts potential divider lower resistor k-ohms

const int DacVref = 40; // DAC Vref in 100s of mV
const int AdcVref = 5;  // ADC Vref in V
const int mAmax = 50;   // Ic for top of screen
const int Ib_inc = 50;  // increment for Ibase step
const unsigned long TimeoutPeriod = 60000;
int MinVgate = 0;
int MaxVgate = 12;
int MinIbase = 0;
int MaxIbase = 350;
long ngJFET = 255;

// be careful using these inside if statements:
#define SerialPrint(s)       \
    {                        \
        if (ExecSerialTx)    \
            Serial.print(s); \
    }
#define SerialPrintLn(s)       \
    {                          \
        if (ExecSerialTx)      \
            Serial.println(s); \
    }

enum TkindDUT
{
    tkNothing,
    tkPNP,
    tkNPN,
    tkPMOSFET,
    tkNMOSFET,
    tkNJFET,
    tkPJFET,
    tkPDiode,
    tkNDiode
};
TkindDUT curkind;

enum TclassDUT
{
    tcBipolar,
    tcMOSFET,
    tcJFET
};
TclassDUT CurDUTclass = tcBipolar;

const uint8_t bmpPNP[] PROGMEM = {
    21, 0, // width
    30, 0, // height
    0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF1, 0xFF, 0x3F, 0xC7, 0xF9, 0xFF,
    0x1F, 0xCF, 0xFC, 0x7E, 0x7F, 0xF1, 0xF3, 0xFF, 0xC7, 0x9F, 0xFF, 0x1C, 0xFF, 0xFC, 0x67, 0xFF,
    0xF1, 0x3F, 0xFF, 0xC1, 0xFF, 0xFF, 0x00, 0x1F, 0xFC, 0x00, 0xFF, 0x93, 0xFF, 0xF0, 0x9F, 0xFE,
    0x0C, 0xFF, 0xC0, 0x67, 0xFC, 0x07, 0x3F, 0xF0, 0x39, 0xFF, 0x83, 0xCF, 0xF8, 0x1E, 0x7F, 0x8D,
    0xF3, 0xF8, 0xFF, 0x9F, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x9F, 0xFF, 0xFC};

const uint8_t bmpNPN[] PROGMEM = {
    21, 0, // width
    30, 0, // height
    0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xE7, 0xFC, 0x7F, 0x3F, 0xC7,
    0xF9, 0xFC, 0x7F, 0xCF, 0xC7, 0xFE, 0x7C, 0x7F, 0xF3, 0xC7, 0xFF, 0x9C, 0x7F, 0xFC, 0xC7, 0xFF,
    0xE4, 0x7F, 0xFF, 0x07, 0xFC, 0x00, 0x7F, 0xE0, 0x03, 0xFF, 0xFE, 0x0F, 0xFF, 0xF2, 0x3B, 0xFF,
    0x98, 0x9F, 0xFC, 0xE0, 0x7F, 0xE7, 0x83, 0xFF, 0x38, 0x0F, 0xF9, 0x80, 0x7F, 0xCF, 0x01, 0xFE,
    0x7E, 0x0F, 0xF3, 0xFC, 0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF0};

const uint8_t bmpNMOSFET[] PROGMEM = {
    23, 0, // width
    34, 0, // height
    0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF,
    0xFF, 0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFE, 0x60, 0x01, 0xFC, 0xC0, 0x03, 0xF9, 0x9F, 0xFF,
    0xF3, 0x3F, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFB, 0xFF, 0x99, 0xC7, 0xFF, 0x32, 0x0F, 0xFE, 0x60,
    0x01, 0xFC, 0xC0, 0x03, 0xF9, 0x90, 0x67, 0xF3, 0x38, 0xCF, 0xE7, 0xFD, 0x9F, 0xCF, 0xFF, 0x3F,
    0x99, 0xFE, 0x7F, 0x33, 0xFC, 0x00, 0x60, 0x00, 0x00, 0xC0, 0x03, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF,
    0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF9, 0xFF,
    0xFF, 0xF0};

const uint8_t bmpPMOSFET[] PROGMEM = {
    23, 0, // width
    34, 0, // height
    0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xE7, 0xFF,
    0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x00, 0x0C, 0xFE, 0x00, 0x19, 0xFF, 0xFF, 0x33, 0xFF,
    0xFE, 0x67, 0xFF, 0xFF, 0xCF, 0xFF, 0xBF, 0x9F, 0xFC, 0x73, 0x3F, 0xE0, 0xE6, 0x7F, 0x80, 0x0C,
    0xFE, 0x00, 0x19, 0xFC, 0x07, 0x33, 0xF9, 0x8E, 0x67, 0xF3, 0xDF, 0xCF, 0xE7, 0xFF, 0x9F, 0xCF,
    0xF3, 0x3F, 0x9F, 0xE6, 0x7F, 0x00, 0x0C, 0x00, 0x00, 0x18, 0x00, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF,
    0xF3, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x3F, 0xFF, 0xFE, 0x7F,
    0xFF, 0xFC};

const uint8_t bmpNJFET[] PROGMEM = {
    21, 128, // width (run-length encoded)
    30, 0,   // height
    0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x05, 0x02, 0x0C,
    0x02, 0x05, 0x02, 0x0C, 0x09, 0x0C, 0x09, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x06, 0x01, 0x0C, 0x02, 0x04,
    0x03, 0x0C, 0x02, 0x02, 0x05, 0x05, 0x2C, 0x05, 0x02, 0x02, 0x05, 0x05, 0x02, 0x05, 0x02, 0x04, 0x03, 0x05, 0x02, 0x0D,
    0x01, 0x05, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13};

const uint8_t bmpPJFET[] PROGMEM = {
    21, 128, // width (run-length encoded)
    30, 0,   // height
    0x00, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x0C, 0x02, 0x05,
    0x02, 0x0C, 0x02, 0x05, 0x02, 0x0C, 0x09, 0x0C, 0x09, 0x0C, 0x02, 0x13, 0x02, 0x13, 0x02, 0x0F, 0x01, 0x03, 0x02, 0x0D,
    0x03, 0x03, 0x02, 0x0B, 0x05, 0x03, 0x02, 0x07, 0x2A, 0x04, 0x05, 0x03, 0x02, 0x05, 0x02, 0x06, 0x03, 0x03, 0x02, 0x05,
    0x02, 0x08, 0x01, 0x0A, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02};

const uint8_t bmpPDiodeBig[] PROGMEM = {
    20, 128, // width run-length encoded
    24, 0,   // height
    0x00, 0x09, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09, 0x14, 0x01, 0x12, 0x03, 0x10, 0x05,
    0x0E, 0x07, 0x0C, 0x09, 0x0A, 0x0B, 0x08, 0x0D, 0x06, 0x0F, 0x04, 0x11, 0x02, 0x09, 0x28, 0x09, 0x02, 0x12, 0x02, 0x12,
    0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09};

const uint8_t bmpNDiodeBig[] PROGMEM = {
    20, 128, // width run-length encoded
    24, 0,   // height
    0x00, 0x09, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09, 0x28, 0x09, 0x02, 0x11, 0x04, 0x0F,
    0x06, 0x0D, 0x08, 0x0B, 0x0A, 0x09, 0x0C, 0x07, 0x0E, 0x05, 0x10, 0x03, 0x12, 0x01, 0x14, 0x09, 0x02, 0x12, 0x02, 0x12,
    0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09};

const uint8_t bmpPDiodeSmall[] PROGMEM = {
    15, 0, // width
    28, 0, // height
    0xFE, 0x01, 0xF8, 0x03, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC,
    0xFE, 0x00, 0x06, 0x00, 0x1E, 0x00, 0x7E, 0x01, 0xFE, 0x07, 0xFE, 0x1F, 0xFE, 0x7F, 0x00, 0x02,
    0x00, 0x07, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC, 0xFF, 0xF9,
    0xFF, 0xF0, 0x07, 0xF0, 0x00};

const uint8_t bmpNDiodeSmall[] PROGMEM = {
    15, 0, // width
    28, 0, // height
    0x00, 0xFE, 0x00, 0xFF, 0xF9, 0xFF, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE,
    0x7F, 0xFC, 0xFE, 0x00, 0x04, 0x00, 0x0F, 0xE7, 0xFF, 0x87, 0xFE, 0x07, 0xF8, 0x07, 0xE0, 0x07,
    0x80, 0x06, 0x00, 0x07, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC,
    0xFC, 0x01, 0xF8, 0x07, 0xF0};

const uint8_t bmpUpDownArrow[] PROGMEM = {
    24, 0, // width
    14, 0, // height
    0xFD, 0xFF, 0xFF, 0xF8, 0xFF, 0x1F, 0xF0, 0x7F, 0x1F, 0xE0, 0x3F, 0x1F, 0xC0, 0x1F, 0x1F, 0x80,
    0x0F, 0x1F, 0x00, 0x07, 0x1F, 0xF8, 0xE0, 0x00, 0xF8, 0xF0, 0x01, 0xF8, 0xF8, 0x03, 0xF8, 0xFC,
    0x07, 0xF8, 0xFE, 0x0F, 0xF8, 0xFF, 0x1F, 0xFF, 0xFF, 0xBF};
