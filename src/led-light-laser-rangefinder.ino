//======================================================================
// OBJECT FILE COPYRIGHT NOTICE
//======================================================================

static const char copyright[] =
  "Copyright (C) 2017 Sean Foley, All rights reserved.";

/*======================================================================
  FILE:
    led-light-laser-rangefinder.ino

  CREATOR:
    Sean Foley

  SERVICES:
    VL53L0X sensor reading, neopixel animation

  GENERAL DESCRIPTION:
    I stuffed some 16 neopixel rings behind the "eyes" of a large
    styrofoam skull.  A microcontroller reads the distance using a VL53L0X 
    laser time-of-flight sensor, and then based on the distance reading
    the neopixels rings get animated accordingly.

    I've tried to make most of the code generic.  All of the animation
    will work on any grbw collection of neopixels.  The pixelLookXXX() 
    calls do assume a certain positioning of the neopixel rings.
    The pixelsRingSpinXX() calls wont give a spin illusion on linear pixels,
    those will animate more like a time's square scrolling message sign 
    (except it will be 1 pixel).

    PARTS:
    VL53L0x Laser time-of-flight sensor 50mm-1200mm range
    https://www.adafruit.com/product/3317
    
    Adafruit WICED WiFi Feather - STM32F205 with Cypress WICED WiFi
    NOTE - you can use a cheaper micro without the wifi
    https://www.adafruit.com/product/3056

    Adafruit Feather 32u4 Basic Proto - this is a good substitute
    if you don't want the cost/complexity of the WICED feather
    https://www.adafruit.com/product/2771

    FeatherWing OLED - 128x32 OLED Add-on 
    NOTE - not essential and the code can be built without OLED support
    https://www.adafruit.com/product/2900

    2 Pcs RJ45 8-pin Connector (8P8C) and Breakout Board Kit 
    NOTE - I separated the sensor and micro into two different pcb's
    so I could have mounting flexibility. I use a small CAT5 Ethernet
    cable as an i2c bus between the pcb's
    https://www.amazon.com/gp/product/B0156JXSF8/

    2 Adafruit Perma-Proto Mint Tin Size Breadboard PCB
    https://www.adafruit.com/product/723

  Copyright (C) 2017 Sean Foley  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Please give credit - It helps me know if stuff I put out is helping peeps
  2. Give back - help someone 
  3. Buy me a beer sometime. Or buy anyone a beer sometime.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  ======================================================================*/

//======================================================================
// INCLUDES AND VARIABLE DEFINITIONS
//======================================================================

//----------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------

// Undefine this or comment it out if
// you are not using an OLED feather display
#define USE_OLED_DISPLAY

// This is the pin to use to indicate activity
// On the adafruit feathers this is an LED on the board
#define PIN_LED_ACTIVITY PA15

#define SERIAL_BAUD_RATE 115000

// The neopixel data pin.  PC7 is on a WICED feather, 
// 6 is probably a good start on a M0/32u4 based micro
#define PIN         PC7 

// The number of pixels on the ring?
#define NEOPIXEL_RING_PIXELS   16

// How many neopixel rings are you using?
#define NEOPIXEL_RING_COUNT    2

// The total number of neopixels
#define TOTAL_PIXELS  NEOPIXEL_RING_PIXELS * NEOPIXEL_RING_COUNT

//----------------------------------------------------------------------
// Include Files
//----------------------------------------------------------------------

#include <Wire.h>
#include <SPI.h>

// The VL53L0X laser rangefinder sensor
#include <VL53L0X.h>

#include <Adafruit_NeoPixel.h>

#ifdef USE_OLED_DISPLAY
// Provides primitives to abstract some
// of the lower level stuff
#include <Adafruit_GFX.h>

// For the oled feather display
#include <Adafruit_SSD1306.h>
#endif

//----------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------

// None.

//----------------------------------------------------------------------
// Global Constant Definitions
//----------------------------------------------------------------------

// This will make it easier to pass around colors.
// Values are grbw respectively.
const uint32_t COLOR_GREEN  = Adafruit_NeoPixel::Color(255, 0, 0, 0);
const uint32_t COLOR_YELLOW = Adafruit_NeoPixel::Color(140, 255, 0, 0);
const uint32_t COLOR_RED    = Adafruit_NeoPixel::Color(0, 255, 0, 0);
const uint32_t COLOR_BLUE   = Adafruit_NeoPixel::Color(0, 0, 255, 0);
const uint32_t COLOR_WHITE  = Adafruit_NeoPixel::Color(0, 0, 0, 255);
const uint32_t COLOR_BLACK  = Adafruit_NeoPixel::Color(0, 0, 0, 0);

//----------------------------------------------------------------------
// Global Data Definitions
//----------------------------------------------------------------------

#ifdef USE_OLED_DISPLAY
Adafruit_SSD1306 display = Adafruit_SSD1306();
#endif

// The laser rangefinder
VL53L0X sensor;

// This is our neopixel object
Adafruit_NeoPixel pixels =
  Adafruit_NeoPixel(TOTAL_PIXELS, PIN, NEO_RGBW + NEO_KHZ800);

//----------------------------------------------------------------------
// Static Variable Definitions
//----------------------------------------------------------------------

// None.

//----------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------

// None.

//----------------------------------------------------------------------
// Required Libraries
//----------------------------------------------------------------------

// None. (Where supported these should be in the form
// of C++ pragmas).

//======================================================================
// FUNCTION IMPLEMENTATIONS
//======================================================================

/*======================================================================
  FUNCTION:
    flashActivityLed()

  DESCRIPTION:
    Simple function that will toggle an LED on/off.  Call this
    function periodically to allow someone to look at the
    microcontroller and know it isn't locked up.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none

  ======================================================================*/
void flashActivityLed()
{
  // Get the current state of the pin
  int state = digitalRead(PIN_LED_ACTIVITY);

  // Invert it and write the output
  digitalWrite(PIN_LED_ACTIVITY, !state);
}

/*======================================================================
  FUNCTION:
    initOledDisplay()

  DESCRIPTION:
    Initializes the OLED display

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none

  ======================================================================*/
void initOledDisplay()
{
#ifdef USE_OLED_DISPLAY
  // Setup the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Display the data
  display.display();

  display.setTextSize(4);
  display.setTextColor(WHITE);
#endif
}

/*======================================================================
  FUNCTION:
    oledPrint()

  DESCRIPTION:
    Displays text on the OLED display.  Use textSize to set
    the font size:
      1 = small
      2 = normal
      3 = med
      4 = big

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    Clears the oled display so any prior text/graphics
    will get nuked

  ======================================================================*/
void oledPrint(const char *text, byte textSize = 4)
{
#ifdef USE_OLED_DISPLAY
  display.setTextSize(textSize);
  display.setTextColor(WHITE);

  display.clearDisplay();
  display.setCursor(0, 0);

  display.print(text);
  display.display();
#endif
}

/*======================================================================
  FUNCTION:
    oledDisplayDistance()

  DESCRIPTION:
    Displays the distance in mm on the OLED display.  Use textSize to set
    the font size:
      1 = small
      2 = normal
      3 = med
      4 = big

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    Clears the oled display so any prior text/graphics
    will get nuked

  ======================================================================*/
void oledDisplayDistance(uint16_t distance, byte textSize = 4)
{
#ifdef USE_OLED_DISPLAY
  display.setTextSize(textSize);
  display.setTextColor(WHITE);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(distance);
  display.print("mm");
  display.display();
#endif
}

/*======================================================================
  FUNCTION:
    setPixelBrightness()

  DESCRIPTION:
    Helper method to set the brightness.  
    Brightness can be 0..255, with 0 = off, 255 = high

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void inline setPixelBrightness(uint8_t brightness)
{
  // Setting the brightness causes "bit rot" (loss) which
  // can skew the colors over time. The library guidance is
  // to call this once.  However, since we are re-initializing
  // the pixel buffer, bit rot shouldn't be a concern.
  // You can also set the brightness by scaling the RGB/W values 
  // in the setPixelColor() call.  For example, a full red would be r=255, 
  // 50% would be r=127. So to control brightness, just scale the rbg 
  // values by the same amount.
  pixels.setBrightness(brightness);
}

/*======================================================================
  FUNCTION:
    pixelsAllPulse()

  DESCRIPTION:
    This function will pulse all the pixels by the specified color.
    It will flip/flop by ramping the brightness level up/down each
    time it is called.  If we did a loop with a delay internal to this
    method, it would delay the ability of the caller to responsive
    to distance changes.

    Just call it multiple times with the color you want to pulse.  You
    need about 10-50ms between calls to allow for decent pulse effect.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsAllPulse(uint32_t color)
{
  // The min and max brightness levels
  const uint8_t MIN = 10;
  const uint8_t MAX = 255;

  // The larger the step, the more dramatic the changes
  // in brightness.  Smaller values give more subtle
  // changes.
  const uint8_t STEP = 10;

  // We need to use something bigger than a byte
  // We step (up/down) the brightness value, then
  // bounds check it.  Since we bounds check after-the-fact
  // it is possible to exceed a byte range (0..255).
  static uint16_t brightness = MAX;

  // Note that this is a signed 8 bit int
  // We need the sign because we are going to
  // flip flop from +1 to -1
  static int8_t countdirection = 1;

  const int8_t UP = 1;
  const int8_t DOWN = -1;

  // What we want is to ramp down from MAX to MIN, then
  // ramp up from MIN to MAX.  This will give a nice
  // pulsing effect.
  brightness = brightness + (STEP * countdirection);

  // Bounds checking
  if ( brightness <= MIN )
  {
    // count up
    countdirection = UP;

    // bounds check to minimum
    brightness = MIN;
  }

  // Bounds checking the other extreme
  if (brightness >= MAX)
  {
    // count down
    countdirection = DOWN;

    brightness = MAX;
  }

  setPixelBrightness(brightness);

  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    pixels.setPixelColor(i, color);
  }

  // We've set the entire array (i.e. the buffer), so
  // now call show to display it (i.e. make it active)
  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsAllStrobe()

  DESCRIPTION:
    This function will strobe (alternate) all of the pixels between a minimum
    brightness to a maximum brightness.  If we handled the delay internally
    to this method, it would delay the ability of the caller to responsive
    to distance changes.

    Just call it multiple times with the color you want to strobe.  You
    need about 10-50ms between calls to allow for decent strobe effect.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsAllStrobe(uint32_t color)
{
  // Min is 25% bright, max is 100%
  const uint8_t MIN = 64;
  const uint8_t MAX = 255;

  static uint8_t brightness = MIN;

  // Flip/flop between the extremes
  switch ( brightness )
  {
    case MIN:
      brightness = MAX;
      break;

    case MAX:
      brightness = MIN;
      break;

    default:
      brightness = MAX;
      break;
  }

  setPixelBrightness(brightness);

  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    pixels.setPixelColor(i, color);
  }

  // We've set the entire array (i.e. the buffer), so
  // now call show to display it (i.e. make it active)
  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsAllOn()

  DESCRIPTION:
    This function will turn on all of the pixels with the
    specified color and brightness level.

    Brightness can be 0..255, with 0 = off, 255 = high

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsAllOn(uint32_t color, uint8_t brightness = 255)
{
  setPixelBrightness(brightness);

  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    pixels.setPixelColor(i, color);
  }

  // We've set the entire array (i.e. the buffer), so
  // now call show to display it (i.e. make it active)
  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsAllOff()

  DESCRIPTION:
    This function will turn all of the pixels off

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsAllOff()
{
  for (int i = 0; i < TOTAL_PIXELS; i++)
  {
    pixels.setPixelColor(i, COLOR_BLACK);
  }

  // We've set the entire array (i.e. the buffer), so
  // now call show to display it (i.e. make it active)
  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsLookLeft()

  DESCRIPTION:
    This function will turn on one distinct pixel in each ring
    to give the illusion of a eyeball looking left.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsLookLeft(uint32_t color, uint8_t brightness = 255)
{
  // We need to start from a known state
  // so turn everything off
  pixelsAllOff();

  // Setting the brightness causes "bit rot" (loss) which
  // can skew the colors over time. The library guidance is
  // to call this once.  However, since we are re-initializing
  // the pixel buffer, bit rot shouldn't be a concern.
  pixels.setBrightness(brightness);

  // These are 16 neopixel ring specific offsets that
  // also match the orientation of the rings' mounting
  pixels.setPixelColor(0, color);
  pixels.setPixelColor(16, color);

  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsLookRight()

  DESCRIPTION:
    This function will turn on one distinct pixel in each ring
    to give the illusion of a eyeball looking right.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsLookRight(uint32_t color, uint8_t brightness = 255)
{
  // We need to start from a known state
  // so turn everything off
  pixelsAllOff();

  setPixelBrightness(brightness);

  // These are 16 neopixel ring specific offsets that
  // also match the orientation of the rings' mounting
  pixels.setPixelColor(7, color);
  pixels.setPixelColor(25, color);

  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsLookUp()

  DESCRIPTION:
    This function will turn on one distinct pixel in each ring
    to give the illusion of a eyeball looking up.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsLookUp(uint32_t color, uint8_t brightness = 255)
{
  // We need to start from a known state
  // so turn everything off
  pixelsAllOff();

  setPixelBrightness(brightness);

  // These are 16 neopixel ring specific offsets that
  // also match the orientation of the rings' mounting
  pixels.setPixelColor(5, color);
  pixels.setPixelColor(21, color);

  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsLookDown()

  DESCRIPTION:
    This function will turn on one distinct pixel in each ring
    to give the illusion of a eyeball looking down.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsLookDown(uint32_t color, uint8_t brightness = 255)
{
  // We need to start from a known state
  // so turn everything off
  pixelsAllOff();

  setPixelBrightness(brightness);

  // These are 16 neopixel ring specific offsets that
  // also match the orientation of the rings' mounting
  pixels.setPixelColor(12, color);
  pixels.setPixelColor(29, color);

  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsRingSpin()

  DESCRIPTION:
    Each time this function is called, it will advance around the ring 
    displaying a distint pixel in each ring.  Subsequent calls will
    give the illusion of a light spinning around the rings.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void pixelsRingSpin(uint32_t color, uint8_t brightness = 255, bool clockwise = false)
{
  static int8_t offset = -1;

  // We need to start from a known state
  // so turn everything off
  pixelsAllOff();

  setPixelBrightness(brightness);

  // Advance around the ring
  if ( clockwise == false )
  {
    offset += 1;

    // Once we have shown each distinct pixel in
    // one ring, then reset and start over
    int r = offset % NEOPIXEL_RING_PIXELS;
    if ( 0 == r )
    {
      offset = 0;
    }
  }
  else
  {
    offset -= 1;

    if ( offset < 0 )
    {
      offset = NEOPIXEL_RING_PIXELS - 1;
    }
  }

  // We loop over N number of rings and set the
  // distinct pixel for each ring.  If the rings
  // are oriented so the first pixel in each ring
  // are aligned, then rings will spin in unison
  for (int i = 0; i < NEOPIXEL_RING_COUNT; i++)
  {
    int x = offset + (i * NEOPIXEL_RING_PIXELS );

    pixels.setPixelColor(x, color);
  }

  pixels.show();
}

/*======================================================================
  FUNCTION:
    pixelsRingSpinClockwise()

  DESCRIPTION:
    Helper method that calls pixelsRingSpin() in a clockwise 
    direction

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void inline pixelsRingSpinClockwise(uint32_t color, uint8_t brightness = 255)
{
  pixelsRingSpin(color, brightness, true);
}

/*======================================================================
  FUNCTION:
    pixelsRingSpinCounterClockwise()

  DESCRIPTION:
    Helper method that calls pixelsRingSpin() in a counter- 
    clockwise direction
    
  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none.

  ======================================================================*/
void inline pixelsRingSpinCounterClockwise(uint32_t color, uint8_t brightness = 255)
{
  pixelsRingSpin(color, brightness, false);
}

/*======================================================================
  FUNCTION:
    animatePixelDisplayByDistance()

  DESCRIPTION:
    This function contains the logic that drives the animation
    based on the distance reading the sensor returns.
    
  RETURN VALUE:
    none.

  SIDE EFFECTS:
    The neopixel animation could/will change after this call

  ======================================================================*/
void animatePixelDisplayByDistance(uint16_t distance)
{
  const uint16_t DISTANCE_MM_WARNING = 762;
  const uint16_t DISTANCE_MM_STOP = 583;
  const uint16_t DISTANCE_MM_CRITICAL = 483;
  const uint16_t DISTANCE_MM_OK = DISTANCE_MM_WARNING + 1;

  if ( distance <= DISTANCE_MM_CRITICAL )
  {
    pixelsAllStrobe(COLOR_RED);
    return;
  }

  if ( distance <= DISTANCE_MM_STOP )
  {
    pixelsAllOn(COLOR_RED);
    return;
  }

  if ( distance <= DISTANCE_MM_WARNING )
  {
    pixelsAllOn(COLOR_YELLOW);
    return;
  }

  if ( distance >= DISTANCE_MM_OK )
  {
    pixelsAllPulse(COLOR_GREEN);
  }
}

/*======================================================================
  FUNCTION:
    setup()

  DESCRIPTION:
    Any setup and initialization should be put in this function.
    It is called once prior to the call to start the main loop().

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none

  ======================================================================*/
void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

  delay(1000);

  Serial.println( "Initializing...");

  flashActivityLed();

  // Initialize the oled display
  initOledDisplay();

  oledPrint("init i2c...", 1 );

  // Initialize the i2c interfacing...
  Wire.begin();

  delay(500);

  oledPrint("init neopixels...", 1);

  // Initialize the neopixel object. Note
  // that the pixel API doesn't seem to support
  // any way to confirm it was initialized ok
  pixels.begin();

  pixelsAllOff();

  // Boot animation.  Let's display each of
  // the grbw leds individually.  This will give a 
  // visual if any of the leds have died
  const int SPINS = 250;
  const int SPIN_WAIT = 5;
  for (int i = 0; i < SPINS; i++)
  {
    pixelsRingSpinClockwise(COLOR_WHITE, 255);
    delay(SPIN_WAIT);
  }

  for (int i = 0; i < SPINS; i++)
  {
    pixelsRingSpinCounterClockwise(COLOR_BLUE, 255);
    delay(SPIN_WAIT);
  }

  for (int i = 0; i < SPINS; i++)
  {
    pixelsRingSpinClockwise(COLOR_GREEN, 255);
    delay(SPIN_WAIT);
  }

  for (int i = 0; i < SPINS; i++)
  {
    pixelsRingSpinCounterClockwise(COLOR_RED, 255);
    delay(SPIN_WAIT);
  }

  pixelsAllOff();

  Serial.println( "Starting distance sensor..." );

  oledPrint("init VL53L0X...", 1);

  bool sensorOk = sensor.init();

  if ( sensorOk == true )
  {
    sensor.setTimeout(500);

    // The longer the timing budget, the more accurate
    // the reading.  Valid values can be 20ms to 5s.
    bool timingOk = sensor.setMeasurementTimingBudget(50000);
    if ( timingOk == false )
    {
      Serial.println( "timing budget failed..");
    }

    Serial.print( "timing budget is " );
    Serial.println( sensor.getMeasurementTimingBudget() );

    // Start continuous back-to-back mode (take readings as
    // fast as possible).  To use continuous timed mode
    // instead, provide a desired inter-measurement period in
    // ms (e.g. sensor.startContinuous(100)).
    sensor.startContinuous();
  }
  else
  {
    Serial.println( "VL53L0X sensor init failed." );
  }

  delay(500);
}

/*======================================================================
  FUNCTION:
    smoothing()

  DESCRIPTION:
    The laser rangefinder will have values that will bounce around, even
    when pointed at a static object (i.e. no change in distance).
    Therefore, we need a function that will "smooth" out these readings.
    We want a function that will:
      - Converge to a result quickly
      - Responsive to large/quick distance changes

  RETURN VALUE:
    The smoothed distance reading

  SIDE EFFECTS:
    none

  ======================================================================*/
uint16_t smoothing( uint16_t reading )
{
  // The max amount that can change
  // between subsequent readings
  const int16_t RANGE_DETLA_CHANGE = 15;

  const int16_t NO_READING_SET = -1;

  // TODO - need to put in bounds checking/overflow logic

  static int count = 0;
  static float avg = 0;
  static int sum = 0;
  static int lastreading = NO_READING_SET;

  if ( lastreading == NO_READING_SET  )
  {
    lastreading = reading;
  }

  int deviation = abs(reading - lastreading);

  if ( deviation >= RANGE_DETLA_CHANGE )
  {
    Serial.println( "Big change in range detected, reseting smothing." );

    // A big change in range happened, so we need
    // to reset the smoothing
    count = 0;
    sum   = 0;
    avg   = 0;
    lastreading = NO_READING_SET;
  }

  count++;
  sum += reading;
  avg = sum / (float)count;

  Serial.print( "Smoothed: " );
  Serial.println( avg );

  return avg;
}

/*======================================================================
  FUNCTION:
    loop()

  DESCRIPTION:
    This is your sketch's main entry point.

  RETURN VALUE:
    none.

  SIDE EFFECTS:
    none

  ======================================================================*/
void loop()
{
  flashActivityLed();

  uint16_t rawdistance = sensor.readRangeContinuousMillimeters();

  if (rawdistance > 8000)
  {
    pixelsRingSpinClockwise(COLOR_GREEN);
    oledPrint("OUT RANGE", 2);
    return;
  }

  Serial.print("raw distance: " );
  Serial.print(rawdistance);
  Serial.print("mm ");

  if (sensor.timeoutOccurred())
  {
    Serial.print(" TIMEOUT");
  }

  uint16_t distance = smoothing(rawdistance);

  oledDisplayDistance(distance);

  animatePixelDisplayByDistance(distance);

  Serial.println();
}
/*=====================================================================
  // IMPLEMENTATION NOTES
  //=====================================================================

    None

  =====================================================================*/




