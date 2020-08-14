/*****************************************************
   ESP32-bike-monitor.ino
   
   ESP32 monitors e-bike battery voltage and current
   ACS712 20 Amp current sensor
   LCD 1602 display
   
   Karl Berger
   10 August 2020

   LCD bar graph adapted from https://github.com/prampec/LcdBarGraph
   By Balázs Kelemen

   2020/08/07 add clock, add constraint to Ibat
   2020/08/06 simplify lcdBarValue
   2020/08/02 improve graph labels, add constraints
   2020/07/27 add OTA
   2020/07/17 indexed data files, cleanup SD functions
   2020/07/07 initial
******************************************************/

//TODO

// For general sketch
#include <Wire.h>                 // [builtin] I2C driver
#include <LiquidCrystal_I2C.h>    // [manager] LCD driver https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include <Ticker.h>               // [builtin] https://arduino-esp8266.readthedocs.io/en/2.7.2/libraries.html?highlight=ticker#ticker
#include <ezTime.h>               // [manager] Rop Gonggrijp https://github.com/ropg/ezTime
#include <WiFi.h>                 // [builtin] for WiFi

// For SD card
#include <FS.h>                   // [builtin] file system
#include <SD.h>                   // [builtin] SD card
#include <SPI.h>                  // [builtin] SPI driver

// for LCD bar graph
#include "barGraphSymbols.h"      // custom definitions

// Wi-Fi & timezone credentials
const char WIFI_SSID[]     = "your wi-fi ssid";
const char WIFI_PASSWORD[] = "your wi-fi password";
const String TIME_ZONE     = "America/New_York";  // your Olson timezone

// ***************** CONNECTIONS ***********************
// use TTGO T8 V1.7 TF Socket connections - non-standard
const int SD_CS    = 13;
const int SD_MOSI  = 15;
const int SD_SCK   = 14;
const int SD_MISO  =  2;

// I2C Bus - standard for reference
// SDA             = 21
// SCL             = 22

// sensor connections
// voltage divider: 50 Vdc full scale
// 120 K in series with 10K in parallel with 39 K
// 10 || 39 = 7.96 K
// ratio is 16.0769 : 1 ( 50 : 3.11 )
const int PIN_VBAT = 34;           // ADC1_6
const int PIN_IBAT = 33;           // ADC1_5

// touch sensor
const int PIN_DISPLAY_MODE = 12;   // Touch5
const int TOUCH_THRESHOLD = 50;    // not touched > 70, touched < 20
const long TOUCH_DELAY = 200;      // minimum touch time for detection
bool touchState = false;           // the latched reading from the input pin

// ************* global variables **********************
const float I_SENSE  = 0.100;      // volts per amp for 20 A ACS712
const float VH = 50.0;             // voltage divider input Vmax
const float VL = 3.11;             // adc Vmax

const float LCD_INTERVAL_S = 1.0;  // display update in SECONDS
const int   DATA_INTERVAL_MS = 10; // read interval in ms
const int   FRAME_COUNT   = 4;     // number of display screen formats

// DATA FILE
// data file is /[root][index].csv, root + index must be < 9 characters
char datafileRoot[ 7 ] = "bike";   // 6 char max
char datafileShortName[ 9 ] = "";  // for LCD display
char datafileName[ 15 ] = "";      // full file name
char INDEX_FN[] = "/index.txt";    // index file

float VzeroI;                      // ACS712 output at 0 A
bool  readDataFlag = false;        // should read data = true
bool  displayFlag = false;         // should display data = true
bool  sdOK = false;                // SD card mount status
float recordID = 0;                // record time in seconds
float Vbat = 0;                    // battery voltage
float Ibat = 0;                    // battery current
float Wbat = 0;                    // battery power
float ampHour = 0;                 // A-h used
float wattHour = 0;                // W-h used

// *********** device instantiations ************
// device_name( I2C address, columns, rows )
// typical addresses: 0x27, 0x20, 0x3F
LiquidCrystal_I2C lcd( 0x27, 16 , 2 );

Ticker readTick;                   // read V-I sensors
Ticker displayTick;                // display on LCD

Timezone myTZ;                     // for timezone

/************************************************
 ************* Ticker callbacks *****************
 ***********************************************/
// called by readTick
void readValues()
{
  readDataFlag = true;
} // readValues()

// called by displayTick
void displayValues()
{
  displayFlag = true;
} // displayValues()

/************************************************
 ****************** setup ***********************
 ***********************************************/
void setup()
{
  Serial.begin( 115200 );
  SPI.begin( SD_SCK, SD_MISO, SD_MOSI );    // use TTGO T8 V1.7 TF Socket connections
  // wire.begin( SDA, SCL);                 // uncomment for non-standard I2C bus
  WiFi.mode( WIFI_STA );                    // station mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);     // connection runs in background
  lcd.init();                               // initialize the lcd
  lcd.backlight();                          // turn on backlight
  lcd.print("Iot-Kits  Logger");            // splash screen
  defineBarGraphSymbols();                  // for graph displays

  delay( 3000 );
  // zero the current sensor at no load
  VzeroI = ACS712Calibrate( PIN_IBAT );

  int bufLen = 17;                          // for 16 column lcd
  char buf[ bufLen ];
  snprintf( buf, bufLen, " VzeroI = %4.2f", VzeroI );
  lcd.setCursor( 0, 1 );
  lcd.print( buf );
  delay( 2000 );

  sdOK = mountSD();
  lcd.setCursor( 0, 1 );
  if ( sdOK )
  {
    snprintf( buf, bufLen, "Log: %s     ", datafileShortName );
    lcd.print( buf );
  }
  else
  {
    lcd.print("** No SD Card **");
  }
  delay( 5000 );

  // check if Wi-Fi connected
  lcd.setCursor( 0, 1 );
  if ( WiFi.status() == WL_CONNECTED )
  {
    lcd.print("Wi-Fi  connected");

    myTZ.setLocation( TIME_ZONE );
    waitForSync();                  // NTP sync
    lcd.setCursor( 0, 1 );
    lcd.println( myTZ.dateTime("Y-m-d H:i:s") );
    delay( 3000 );
  }
  else
  {
    lcd.print("**  No Wi-Fi  **");
    WiFi.mode( WIFI_OFF );         // no need for wifi when not connected
  }

  // start Tickers and attach to callbacks
  readTick.attach_ms( DATA_INTERVAL_MS, readValues );
  displayTick.attach( LCD_INTERVAL_S, displayValues );
} // setup()

/************************************************
 ******************** loop **********************
 ***********************************************/
void loop()
{
  // Vbat, Ibat, and Wbat are global
  static int frame = 1;               // start with dashboard frame
  static int count = 0;               // times data is read per display cycle
  static float IbatV = 0;             // holds voltage representing current

  detectTouch( PIN_DISPLAY_MODE );

  if ( readDataFlag )                 // ticker call to read data
  {
    // read and accumulate the V & I sensors
    readDataFlag = false;             // reset ticker flag

    unsigned int Vadc = analogRead( PIN_VBAT );
    Vbat += adcLinearized( Vadc );    // accumulate linearized voltage reading

    unsigned int Iadc = analogRead( PIN_IBAT );
    IbatV += adcLinearized( Iadc );   // accumulate linearized voltage reading for current

    count++;                          // increment number of readings
  }

  if ( displayFlag )                  // ticker call to display data
  {
    // display data on LCD
    displayFlag = false;              // reset ticker flag

    // process the readings
    Vbat = Vbat / count;              // get average adc voltage
    Vbat = VH * Vbat / VL;            // scale it for voltage divider
    IbatV = IbatV / count;            // get average adc voltage representing current
    Ibat = ( IbatV - VzeroI ) / I_SENSE;  // convert to current
    // constrain Ibat to positive values
    // or set VzeroI = IbatV to prevent negative result
    // it should not be negative on a bike without regenerative braking
    // negative values mess up display of Ibat and Wbat
    if ( Ibat < 0 )
    {
      Ibat = 0;
    }

    ////// For Testing only - remove after testing //////////////
    //    Ibat = 0.10 * random(0, 100);
    //    Vbat = 0.10 * random(320, 400);
    /////////////////////////////////////////////////////////////

    Wbat = Vbat * Ibat;               // calculate power

    // integrate values
    ampHour = ampHour + ( Ibat * LCD_INTERVAL_S / 3600 );
    wattHour = wattHour + ( Wbat * LCD_INTERVAL_S / 3600 );

    // touch sense & display select
    if ( touchState == true )
    {
      touchState = false;             // reset touchSense flag
      frame++;                        // increment frame ID
      if ( frame > FRAME_COUNT )
      {
        frame = 1;                    // rollover to first frame
      }
      displayFrame( frame, true );    // display new frame & refresh screen
    }
    else
    {
      displayFrame( frame, false );   // display current frame w/o refresh if no touch sensed
    }

    // log data to SD card
    if ( sdOK )
    {
      logSDCard();                    // write data to SD card
    }

    // reset cummultives used in averaging
    count = 0;
    Vbat = 0;
    IbatV = 0;

    recordID = recordID + LCD_INTERVAL_S;  // time of record in seconds
  }
} // loop()

/************************************************
 ************ adcLinearized *********************
 ***********************************************/
// https://w4krl.com/esp32-analog-to-digital-conversion-accuracy/
float adcLinearized( unsigned int adc )
{
  float volts;
  if ( adc > 3000 )
  {
    volts = 0.0005 * adc + 1.0874;
  }
  else
  {
    volts = 0.0008 * adc + 0.1372;
  }
  return volts;
}

/************************************************
 ********** ACS712Calibrate *********************
 ***********************************************/
float ACS712Calibrate( int pin )
{
  // find average ACS712 output at zero current
  float V0I = 0.0;
  int count = 20;         // readings to average
  for ( int i = 0; i < count; i++ ) {
    unsigned int adc = analogRead( pin );
    V0I += adcLinearized( adc );
    delay( 10 );          // 10 ms between readings
  }
  V0I = V0I / count;      // find average reading
  return V0I;
} // ACS712Calibrate()

/************************************************
 ****************** mountSD *********************
 ***********************************************/
bool mountSD()
{
  int fileIndex = 0;       // initial file index
  int bufLen = 20;         // size for longest expected filename + path
  char buf[ bufLen ];      // holds file name
  char idx[ 3 ] = "";      // read & store file index 0 to 99
  bool result = false;     // result of file operations

  // mount card
  result = SD.begin( SD_CS );
  if ( result == false )
  {
    Serial.println("Card Mount Failed");
    return false;
  }

  // card mount is sucessful - open INDEX.txt
  // If the file doesn't exist, create it with initial file index
  File file = SD.open( INDEX_FN );
  if ( file == true )
  {
    // read index and get last number, increment, write to index, and use it for data filename
    int count = 0;  // char array index
    // could possibly use .parseInt
    while ( file.available() )
    {
      idx[ count ] = file.read();
      count++;
    }
    fileIndex = atoi( idx );        // convert string array to integer
    fileIndex++;                    // increment fileIndex
    if ( fileIndex > 99 )
    {
      fileIndex = 0;                // rollover at 99
    }

    file.close();
  }

  itoa( fileIndex, idx, 10 );       // convert integer to string array

  // create or update index file
  result = writeFile( INDEX_FN, idx );
  if ( result == false )
  {
    return false;
  }

  // now handle data file
  // assemble short filename for display
  strcpy( datafileShortName, datafileRoot );
  strcat( datafileShortName, idx );

  // assemble full filename
  strcpy( datafileName, "/" );
  strcat( datafileName, datafileShortName );
  strcat( datafileName, ".csv" );
  Serial.print("Creating file ");
  Serial.println( datafileName );

  bufLen = 35;
  char str[ bufLen ];
  // write file with header
  strncpy( str, "Time,ID,Vbat,Ibat,Wbat,A-h,W-h\r\n", bufLen );
  result = writeFile( datafileName, str );

  return result;
}  // mountSD()

/************************************************
 **************** logSDCard *********************
 ***********************************************/
// Write the sensor readings on the SD card
void logSDCard()
{
  int bufLen = 25;
  char timeString[ bufLen ] = "";
  strncpy( timeString, myTZ.dateTime("Y-m-d H:i:s").c_str(), bufLen );
  bufLen = 60;   // size for longest expected string
  // 2020-08-07 12:39:02,42.7,10.2,435.5,10.0,370.4 / 46 plus
  char buf[ bufLen ];
  snprintf( buf, bufLen, "%s,%.1f,%.1f,%.2f,%.1f,%.2f,%.1f\r\n",
            timeString, recordID, Vbat, Ibat, Wbat, ampHour, wattHour );
  appendFile( datafileName, buf );
} // logSDCard()

/************************************************
 **************** writeFile *********************
 ***********************************************/
// Write to the SD card
bool writeFile( const char *path, const char *message )
{
  bool result;
  File file = SD.open( path, FILE_WRITE );

  if ( file == false )
  {
    Serial.println("Failed to open file for writing");
    return false;
  }

  // write data
  result = file.print( message );
  if ( result == false )
  {
    Serial.println("Write failed");
    return false;
  }

  file.close();
  return true;
} // writeFile()

/************************************************
 *************** appendFile *********************
 ***********************************************/
// Append data to the SD card
bool appendFile( const char *path, const char *message)
{
  bool result;
  File file = SD.open( path, FILE_APPEND );

  if ( file == false )
  {
    Serial.println("Failed to open file for appending");
    return false;
  }

  // append data
  result = file.print( message );
  if ( result == false )
  {
    Serial.println("Append failed");
    return false;
  }

  return true;
  file.close();
} // appendFile()

/************************************************
 **************** displayFrame ******************
 ************************************************/
void displayFrame( int frame, bool refreshScreen )
{
  switch ( frame )
  {
    case 1:  // dashboard
      dashBoard();
      break;
    case 2:  // current
      ampsGraph( refreshScreen );
      break;
    case 3:  // wattage
      wattsGraph( refreshScreen );
      break;
    case 4:  // time
      clockDisplay( refreshScreen );
      break;
  }
} // displayFrame()

/************************************************
 *************** dashBoard **********************
 ************************************************/
// text display of main values
void dashBoard()
{
  int bufLen = 17;            // 16-character lcd
  char buf[ bufLen ];         // buffer for display line
  // top row
  lcd.setCursor( 0, 0 );
  snprintf( buf, bufLen, "%4.1fV %4.1fA %3.0fW", Vbat, Ibat, Wbat );
  lcd.print( buf );

  // bottom row
  lcd.setCursor( 0, 1 );
  snprintf( buf, bufLen, "%4.2fAh    %4.1fWh", ampHour, wattHour );
  lcd.print( buf );
} // dashBoard()


void ampsGraph( bool refresh )
{
  if ( refresh )
  {
    lcd.clear();
    lcd.setCursor( 0, 0 );
    lcd.print( axis16amp );

  }
  lcd.setCursor( 0, 1 );
  lcdBarValue( Ibat, 16 );
}

void wattsGraph( bool refresh )
{
  if ( refresh )
  {
    lcd.clear();
    lcd.setCursor( 0, 1 );
    lcd.print( axis400watt );
  }
  lcd.setCursor( 0, 0 );
  lcdBarValue( Wbat, 400 );
}

void clockDisplay( bool refresh )
{
  if ( refresh )
  {
    lcd.clear();
  }
  lcd.setCursor( 0, 0 );
  lcd.print( myTZ.dateTime("g:i A"));  // HH:MM AM/PM
  int bufLen = 17;
  char buf[ bufLen ];
  int ss = recordID;
  int hh = ss / 3600;
  ss -= hh * 3600;
  int mm = ss / 60;
  ss -= 60 * mm;
  snprintf( buf, bufLen, "Ride: %02d:%02d:%02d", hh, mm, ss );
  lcd.setCursor( 0, 1 );
  lcd.print( buf );
}

/************************************************
 ************* defineBarGraphSymbols ************
 ***********************************************/
void defineBarGraphSymbols()
{
  // set custom segments
  // BAR_0 and BAR_5 use built in characters
  lcd.createChar( BAR_1,  bar1 );
  lcd.createChar( BAR_2,  bar2 );
  lcd.createChar( BAR_3,  bar3 );
  lcd.createChar( BAR_4,  bar4 );
  lcd.createChar( NUM_0,  num0 );
  lcd.createChar( NUM_5,  num5 );
  lcd.createChar( NUM_10, num10 );
  lcd.createChar( NUM_15, num15 );
} // defineBarGrapghSymbols()

/************************************************
 **************** lcdBarValue *******************
 ************************************************/
void lcdBarValue( float value, float fullScale )
{
  const int NUM_COLS = 16;                  // width of display in characters
  const int FS_PIXELS = 5 * NUM_COLS;  // number of pixel columns for full scale
  static int prevPixels = 0;
  static int prevFullChars = 0;
  int pixels = ( value / fullScale ) * FS_PIXELS;
  // constrain pixels to LCD range
  if ( pixels > FS_PIXELS )
  {
    pixels = FS_PIXELS;
  }
  if ( pixels < 1 )
  {
    pixels = 0;
    lcd.write( 0xA1 );            // 0xA1, 0xA5, 0x2E
  }
  // calculate full & partial character counts
  int fullChars = value * NUM_COLS / fullScale;
  int partialChars = pixels - 5 * fullChars;

  // if pixel count does not change, do not draw anything
  if ( pixels != prevPixels )
  {
    // write full characters
    for ( int i = 0; i < fullChars; i++ )
    {
      lcd.write( BAR_5 );           // use built in full character
    }

    // write the partial character
    if ( partialChars > 0 )         // 1 = BAR_1, 2 = BAR_2, etc
    {
      lcd.write( partialChars );    // use the custom defined characters
    }

    // clear characters left over from the previous draw
    // use a full blank to overwrite partials
    for ( int i = fullChars; i < prevFullChars + 1; i++ )
    {
      lcd.write( BAR_0 );           // print blank character
    }
  }
  // save current bar lengths
  prevFullChars = fullChars;
  prevPixels = pixels;
} // lcdBarValue()

/************************************************
 *************** detetcTouch ********************
 ************************************************/
void detectTouch( int pin )
{
  // touchState is global and is reset in loop()
  static bool prevState = false;
  static unsigned long prevTouchTime = millis();
  bool currentState = false;

  int value = touchRead( pin );

  if ( value < TOUCH_THRESHOLD )
  {
    currentState = true;
  }

  // If the switch changed, due to noise or pressing:
  if ( currentState != prevState )
  {
    // reset the debouncing timer
    prevTouchTime = millis();
  }

  if ( ( millis() - prevTouchTime ) > TOUCH_DELAY )
  {
    // whatever the touchState is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state

    // if currentState is true, set touchState to true, it will be reset in the display routine
    if ( currentState == true )
    {
      touchState = true;
    }
  }

  // save the currentState. Next time through the loop, it'll be the prevState:
  prevState = currentState;
} // detectTouch()
