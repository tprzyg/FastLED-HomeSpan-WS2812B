////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

#include "extras/PwmPin.h"                              // NEW! Include this HomeSpan "extra" to create LED-compatible PWM signals on one or more pins

struct DEV_LED : Service::LightBulb {                   // ON/OFF LED

  int ledPin;                                           // pin number defined for this LED
  SpanCharacteristic *power;                            // reference to the On Characteristic

  DEV_LED(int ledPin) : Service::LightBulb() {          // constructor() method

    power = new Characteristic::On();
    this->ledPin = ledPin;
    pinMode(ledPin, OUTPUT);

  } // end constructor

  boolean update() {                                    // update() method

    digitalWrite(ledPin, power->getNewVal());

    return (true);                                      // return true

  } // update
};

//////////////////////////////////

// Here's the new code defining DEV_DimmableLED - changes from above are noted in the comments

struct DEV_DimmableLED : Service::LightBulb {           // Dimmable LED

  LedPin *ledPin;                                       // NEW! Create reference to LED Pin instantiated below
  SpanCharacteristic *power;                            // reference to the On Characteristic
  SpanCharacteristic *level;                            // NEW! Create a reference to the Brightness Characteristic instantiated below

  DEV_DimmableLED(int pin) : Service::LightBulb() {     // constructor() method

    power = new Characteristic::On();

    level = new Characteristic::Brightness(1);         // NEW! Instantiate the Brightness Characteristic with an initial value of 50% (same as we did in Example 4)
    level->setRange(0, 100, 1);                         // NEW! This sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1% (different from Example 4 values)

    this->ledPin = new LedPin(pin);                     // NEW! Configures a PWM LED for output to the specified pin.  Note pinMode() does NOT need to be called in advance

  } // end constructor

  boolean update() {                                    // update() method

    // Here we set the brightness of the LED by calling ledPin->set(brightness), where brightness=0-100.
    // Note HomeKit sets the on/off status of a LightBulb separately from its brightness, which means HomeKit
    // can request a LightBulb be turned off, but still retains the brightness level so that it does not need
    // to be resent once the LightBulb is turned back on.

    // Multiplying the newValue of the On Characteristic ("power", which is a boolean) with the newValue of the
    // Brightness Characteristic ("level", which is an integer) is a short-hand way of creating the logic to
    // set the LED level to zero when the LightBulb is off, or to the current brightness level when it is on.

    ledPin->set(power->getNewVal()*level->getNewVal());

    return (true);                                      // return true

  } // update
};

//////////////////////////////////

struct DEV_RgbLED : Service::LightBulb {       // RGB LED (Command Cathode)

  LedPin *redPin, *greenPin, *bluePin;
  
  SpanCharacteristic *power;                   // reference to the On Characteristic
  SpanCharacteristic *H;                       // reference to the Hue Characteristic
  SpanCharacteristic *S;                       // reference to the Saturation Characteristic
  SpanCharacteristic *V;                       // reference to the Brightness Characteristic
  
  DEV_RgbLED(int red_pin, int green_pin, int blue_pin) : Service::LightBulb(){       // constructor() method

    power=new Characteristic::On();                    
    H=new Characteristic::Hue(0);              // instantiate the Hue Characteristic with an initial value of 0 out of 360
    S=new Characteristic::Saturation(0);       // instantiate the Saturation Characteristic with an initial value of 0%
    V=new Characteristic::Brightness(100);     // instantiate the Brightness Characteristic with an initial value of 100%
    V->setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    
    this->redPin=new LedPin(red_pin);        // configures a PWM LED for output to the RED pin
    this->greenPin=new LedPin(green_pin);    // configures a PWM LED for output to the GREEN pin
    this->bluePin=new LedPin(blue_pin);      // configures a PWM LED for output to the BLUE pin
 
    char cBuf[128];
    sprintf(cBuf,"Configuring RGB LED: Pins=(%d,%d,%d)\n",redPin->getPin(),greenPin->getPin(),bluePin->getPin());
    Serial.print(cBuf);
    
  } // end constructor

  boolean update(){                         // update() method

    boolean p;
    float v, h, s, r, g, b;

    h=H->getVal<float>();                      // get and store all current values.  Note the use of the <float> template to properly read the values
    s=S->getVal<float>();
    v=V->getVal<float>();                      // though H and S are defined as FLOAT in HAP, V (which is brightness) is defined as INT, but will be re-cast appropriately
    p=power->getVal();

    char cBuf[128];
    sprintf(cBuf,"Updating RGB LED: Pins=(%d,%d,%d): ",redPin->getPin(),greenPin->getPin(),bluePin->getPin());
    LOG1(cBuf);

    if(power->updated()){
      p=power->getNewVal();
      sprintf(cBuf,"Power=%s->%s, ",power->getVal()?"true":"false",p?"true":"false");
    } else {
      sprintf(cBuf,"Power=%s, ",p?"true":"false");
    }
    LOG1(cBuf);
      
    if(H->updated()){
      h=H->getNewVal<float>();
      sprintf(cBuf,"H=%.0f->%.0f, ",H->getVal<float>(),h);
    } else {
      sprintf(cBuf,"H=%.0f, ",h);
    }
    LOG1(cBuf);

    if(S->updated()){
      s=S->getNewVal<float>();
      sprintf(cBuf,"S=%.0f->%.0f, ",S->getVal<float>(),s);
    } else {
      sprintf(cBuf,"S=%.0f, ",s);
    }
    LOG1(cBuf);

    if(V->updated()){
      v=V->getNewVal<float>();
      sprintf(cBuf,"V=%.0f->%.0f  ",V->getVal<float>(),v);
    } else {
      sprintf(cBuf,"V=%.0f  ",v);
    }
    LOG1(cBuf);

    // Here we call a static function of LedPin that converts HSV to RGB.
    // Parameters must all be floats in range of H[0,360], S[0,1], and V[0,1]
    // R, G, B, returned [0,1] range as well

    LedPin::HSVtoRGB(h,s/100.0,v/100.0,&r,&g,&b);   // since HomeKit provides S and V in percent, scale down by 100

    int R, G, B;

    R=p*r*100;                                      // since LedPin uses percent, scale back up by 100, and multiple by status fo power (either 0 or 1)
    G=p*g*100;
    B=p*b*100;

    sprintf(cBuf,"RGB=(%d,%d,%d)\n",R,G,B);
    LOG1(cBuf);

    redPin->set(R);                      // update the ledPin channels with new values
    greenPin->set(G);    
    bluePin->set(B);    
      
    return(true);                               // return true
  
  } // update
};
      
//////////////////////////////////

struct DEV_FastLED : Service::LightBulb {               // ON/OFF LED

  CRGB *ledArray;                                       // reference to the LED Array object
  int numLeds;
  SpanCharacteristic *power;                            // reference to the On Characteristic
  SpanCharacteristic *H;                                // reference to the Hue Characteristic
  SpanCharacteristic *S;                                // reference to the Saturation Characteristic
  SpanCharacteristic *V;                                // reference to the Brightness Characteristic

  DEV_FastLED(CRGB* myLeds, int mySize) : Service::LightBulb() {      // constructor() method

    power = new Characteristic::On();
    H = new Characteristic::Hue(0);                     // instantiate the Hue Characteristic with an initial value of 0 out of 360
    S = new Characteristic::Saturation(0);              // instantiate the Saturation Characteristic with an initial value of 0%
    V = new Characteristic::Brightness(100);            // instantiate the Brightness Characteristic with an initial value of 100%
    V->setRange(1, 100, 1);                             // sets the range of the Brightness to be from a min of 1%, to a max of 100%, in steps of 1%

    this->ledArray = myLeds;
    this->numLeds = mySize;

    char cBuf[128];
    sprintf(cBuf, "Configuring RGB LED Strip with %d lights\n", numLeds );
    Serial.print(cBuf);

    fill_solid(ledArray, numLeds, CRGB::White);
    FastLED.setBrightness( 0 );
    FastLED.show();

  } // end constructor

  boolean update() {                                    // update() method

    boolean p;
    float v, h, s;

    h = H->getVal<float>();
    s = S->getVal<float>();
    v = V->getVal<float>();
    p = power->getVal();
    char cBuf[128];
    sprintf(cBuf, "Updating  Strip with %d lights\n", numLeds );
    LOG1(cBuf);

    if (power->updated()) {
      p = power->getNewVal();
      sprintf(cBuf, "Power=%s->%s, ", power->getVal() ? "true" : "false", p ? "true" : "false");
    } else {
      sprintf(cBuf, "Power=%s, ", p ? "true" : "false");
    }
    LOG1(cBuf);

    if (H->updated()) {
      h = H->getNewVal<float>();
      sprintf(cBuf, "H=%.0f->%.0f, ", H->getVal<float>(), h);
    } else {
      sprintf(cBuf, "H=%.0f, ", h);
    }
    LOG1(cBuf);

    if (S->updated()) {
      s = S->getNewVal<float>();
      sprintf(cBuf, "S=%.0f->%.0f, ", S->getVal<float>(), s);
    } else {
      sprintf(cBuf, "S=%.0f, ", s);
    }
    LOG1(cBuf);

    if (V->updated()) {
      v = V->getNewVal<float>();
      sprintf(cBuf, "V=%.0f->%.0f  ", V->getVal<float>(), v);
    } else {
      sprintf(cBuf, "V=%.0f \n ", v);
    }
    LOG1(cBuf);

    int fH, fS, fV;

    // convert h,s,v into FastLED values (0-255 range rather than 0-360 for h, 0-100 for s and v)
    fH = map(h, 0, 360, 0, 255);
    fS = map(s, 0, 100, 0, 255);
    fV = map(v, 0, 100, 0, 255);

    // Create a CHSV object and convert into CRGB color object using hsv2rgb_spectrum (another option is to use hsv2rgb_rainbow).
    // Use full brigthness when converting HSV to RGB (not sure why, when using actual fV it was creating weird color effects).
    CHSV color_hsv;
    color_hsv.setHSV(fH, fS, 255);
    CRGB color_rgb;    
    hsv2rgb_spectrum(color_hsv, color_rgb);

    // Set the whole ribbom to the selected color
    fill_solid(ledArray, numLeds, color_rgb);
    // If p is true (power is on) set the ribbon brightness to converted v value (fV is 0-255)
    // constrained by MIN/MAX_BRIGHTNESS defined in the main sketch, otherwise set brighness to 0 (turn off the ribbon LEDs)
    if (p) {
      FastLED.setBrightness( constrain(fV, MIN_BRIGHTNESS, MAX_BRIGHTNESS) );
    } else {
      FastLED.setBrightness( 0 );
    }
    FastLED.show();

    return (true);                                      // return true

  } // update
};

//////////////////////////////////
