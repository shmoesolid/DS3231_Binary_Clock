// ORIGINAL:
//
// Binary Clock v0.2
// Gabriel J. Pensky

// UPDATED:
//
// v0.2.s.1?
// Converted to use DS3231 with Rinky-Dink Electronics (Henning Karlsen) library v1.01
// ( Web: http://www.rinkydinkelectronics.com/library.php?id=73 )
// Cleaned up for readability for us commoners
// Shane B. Koehler (aka ShmoeSOLID)

// README:
//
// To use the hardware I2C (TWI) interface of the Arduino you must connect
// the pins as follows:
//
// Arduino Uno/2009:
// ----------------------
// DS3231:  SDA pin   -> Arduino Analog 4 or the dedicated SDA pin
//          SCL pin   -> Arduino Analog 5 or the dedicated SCL pin
//
// Arduino Leonardo:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 2 or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 3 or the dedicated SCL pin
//
// Arduino Mega:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL pin
//
// Arduino Due:
// ----------------------
// DS3231:  SDA pin   -> Arduino Digital 20 (SDA) or the dedicated SDA1 (Digital 70) pin
//          SCL pin   -> Arduino Digital 21 (SCL) or the dedicated SCL1 (Digital 71) pin
//
// The internal pull-up resistors will be activated when using the 
// hardware I2C interfaces.
//
// You can connect the DS3231 to any available pin but if you use any
// other than what is described above the library will fall back to
// a software-based, TWI-like protocol which will require exclusive access 
// to the pins used, and you will also have to use appropriate, external
// pull-up resistors on the data and clock signals.
//

// include the great library
#include <DS3231.h>

// intialize the DS3231 using the hardware interface (tested with Uno)
DS3231 rtc(SDA, SCL);

// Macros to convert the bcd values of the registers to normal integer variables.
// The code uses separate variables for the high byte and the low byte
// of the bcd, so these macros handle both bytes separately.
//#define bcd2bin(h,l)    (((h)*10) + (l)) // not used but left in for reference
#define bin2bcd_h(x)   ((x)/10)
#define bin2bcd_l(x)   ((x)%10)

// Constants -----------------------------------------------------------------------------------------------------------
const int BUTTON_RESET_THRESHOLD = 500; // time in milliseconds (can hold time change button(s) x time before it changes again)
enum TimeType { Hour, Minute }; // for our IncreaseByOne function

// Variables -----------------------------------------------------------------------------------------------------------
unsigned long timen = 0; // 1 second update timer for updating our time
unsigned long timeb = 0; // button timer for reset

// byte values for each column
byte s0 = B00000000; // low seconds
byte s1 = B00000000; // high seconds
byte m0 = B00000000; // low minutes
byte m1 = B00000000; // high minutes
byte h0 = B00000000; // low hours
byte h1 = B00000000; // high hours

// define our time per library (doesn't need to be global but whatever)
Time t;

// Setup -----------------------------------------------------------------------------------------------------------------
void setup()
{
  // set certain pins to output and high
  for (int i = 2; i <= 11; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }

  Serial.begin(9600); // 115200

  // Uncomment the next line if you are using an Arduino Leonardo (untested)
  //while (!Serial) {}

  rtc.begin();

  // >> SET YOUR CURRENT TIME HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  // uses 24 hour ONLY currently
  rtc.setTime(0, 0, 0); // hours, minutes, seconds

  // 24h easy mode: if hours over 12, subtract by 2, if first digit is a 1, remove it, if first digit is a 2, make it a 1
  // ie: 15 - 2 = 13, first digit 1 so remove, hour is 3
  // ie: 19 - 2 = 17, first digit 1 so remove, hour is 7
  // ie: 22 - 2 = 20, first digit 2 so make 1, hour is 10
  // ie: 23 - 2 = 21, first digit 2 so make 1, hour is 11
}

// Loop ----------------------------------------------------------------------------------------------------------
void loop()
{
  // default force time update false
  boolean forceTimeUpdate = false;

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // handle actual LED lighting from our time byte variables

  // go through our columns
  for (int coln = 0; coln <= 4; coln++)
  {
    // coln + 6 is the actual pin location
    // set high until complete with column(s)
    digitalWrite(coln + 6, HIGH);

    switch (coln)
    {

      // handle low seconds (all bits)
      case 0:

        // go through each row in our current column
        for (int rown = 0; rown <= 3; rown++)
        {
          // s0 is our byte
          // rown is the bit location in our seconds byte
          // rown + 2 is the actual pin #
          if (BitActive(s0, rown)) SwitchOnOff(rown + 2);
        }

        break;

      // handle high seconds (all bits) and high hours (first bit only)
      case 1:

        // seconds
        for (int rown = 0; rown <= 3; rown++)
        {
          if (BitActive(s1, rown)) SwitchOnOff(rown + 2);
        }

        // hour
        if (BitActive(h1, 0)) SwitchOnOff(5);

        break;

      // handle low minutes (all bits)
      case 2:
        for (int rown = 0; rown <= 3; rown++)
        {
          if (BitActive(m0, rown)) SwitchOnOff(rown + 2);
        }

        break;

      // handle high minutes (all bits) and high hours (2nd bit only)
      case 3:

        // minutes
        for (int rown = 0; rown <= 3; rown++)
        {
          if (BitActive(m1, rown)) SwitchOnOff(rown + 2);
        }

        // hour
        if (BitActive(h1, 1)) SwitchOnOff(5);

        break;

      // handle low hours (all bits)
      case 4:
        for (int rown = 0; rown <= 3; rown++)
        {
          if (BitActive(h0, rown)) SwitchOnOff(rown + 2);
        }

        break;

    }

    // shut down power on current column(s)
    digitalWrite(coln + 6, LOW);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // handle buttons

  // button 1 .. hours
  // button is pressed and timer is not running
  // lets update the time (hours button will override the minutes button if pressed at the same time)
  if (digitalRead(A1) == 0 && timeb == 0)
  {
    IncreaseTimeByOne(Hour);
    forceTimeUpdate = true;
  }

  // button 2 .. minutes
  else if (digitalRead(A2) == 0 && timeb == 0)
  {
    IncreaseTimeByOne(Minute);
    forceTimeUpdate = true;
  }

  // threshold button timer handling
  // if either button pressed, increase timer
  if (digitalRead(A1) == 0 || digitalRead(A2) == 0)
  {
    // increase timer
    timeb++;

    // reset if over threshold seconds
    // this allows the time to be increased again if still holding
    if (timeb > BUTTON_RESET_THRESHOLD)
      timeb = 0;
  }

  // reset timer if both buttons not pressed
  // this allows for instant reset upon release
  else
  {
    timeb = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // handle time change per second or forced if manually changed hour/min via buttons
  // FYI time change manually will lose fractions of a second based on current setup
  
  if (timen <= millis() - 1000 || forceTimeUpdate)
  {
    char buffer[80];     // the code uses 70 characters (don't know if needed but left in bc sure why not [untested without, don't have arduino])

    // set our t time using library getTime method
    // convert each var into byte vars accordingly using predefined high/low conversions
    t = rtc.getTime();
    s0 = bin2bcd_l(t.sec); // low
    s1 = bin2bcd_h(t.sec); // high
    m0 = bin2bcd_l(t.min); // low
    m1 = bin2bcd_h(t.min); // high
    h0 = bin2bcd_l(t.hour); // low
    h1 = bin2bcd_h(t.hour); // high

    timen = millis(); // set new timen with ms program been running
  }
}

// -------------------------------------------------------------------------------------------------------------
// SwitchOnOff
//
// This function sets pin location low to high real quick for visualization of LED
// LED basically flashes quick, appears to stay on to human eye
//
void SwitchOnOff(int pin)
{
  digitalWrite(pin, LOW); // sets oppoisite of our column so LED lights up
  delayMicroseconds(100); // waits
  digitalWrite(pin, HIGH); // reset to high so it is off by default
}

// -------------------------------------------------------------------------------------------------------------
// BitActive
//
// Simply makes bitRead easier to read/understand
// if the bit we need in the byte is a 1, we are active, if a 0, not active
//
// EXAMPLES:
//
// BYTE: 00001001
//              ^ location (0) = true
// BYTE: 00001001
//             ^ location (1) = false
// BYTE: 00001001
//            ^ location (2) = false
// BYTE: 00001001
//           ^ location (3) = true
//
boolean BitActive(byte b, int location)
{
  return ( bitRead(b, location) == 1 ) ? true : false;
}

// -------------------------------------------------------------------------------------------------------------
// IncreaseTimeByOne
//
// Increases either hour or min by 1 based on TimeType enum
//
void IncreaseTimeByOne(TimeType type)
{
  // get current time
  Time tmpTime = rtc.getTime();

  if (type == Hour)
  {
    // increase hours by 1
    int tmpHour = tmpTime.hour + 1;
    if (tmpHour >= 24) tmpHour = tmpHour - 24;
    tmpTime.hour = tmpHour;
  }
  else if (type == Minute)
  {
    // increase minutes by 1
    int tmpMin = tmpTime.min + 1;
    if (tmpMin >= 60) tmpMin = tmpMin - 60;
    tmpTime.min = tmpMin;
  }

  // set new time
  rtc.setTime(tmpTime.hour, tmpTime.min, tmpTime.sec);
}
