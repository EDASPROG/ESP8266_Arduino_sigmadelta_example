
#ifdef ESP8266
extern "C" {
  #include "user_interface.h"
  #include "sigma_delta.h"
}
#endif

int led = D4;           // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 1;    // 

void setup() { 
  sigma_delta_setup(led);
}

void loop() {
  set_sigma_duty_156KHz(brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }
  // wait for 10 milliseconds to see the dimming effect
  delay(10);
}
