#include <avr/wdt.h>            // library for default watchdog functions
#include <avr/interrupt.h>      // library for interrupts handling
#include <avr/sleep.h>          // library for sleep
#include <avr/power.h>          // library for power control
volatile int nbr_remaining;               // Sleep multiplier X 8 sec


// Put the Arduino to deep sleep. Only an interrupt can wake it up.
void sleep(int ncycles)
{  
  nbr_remaining = ncycles; // defines how many cycles should sleep

  // Set sleep to full power down.  Only external interrupts or
  // the watchdog timer can wake the CPU!
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
 
  // Turn off the ADC while asleep.
  power_adc_disable();
 
  while (nbr_remaining > 0){ // while some cycles left, sleep!

  // Enable sleep and enter sleep mode.
  sleep_mode();

  // CPU is now asleep and program execution completely halts!
  // Once awake, execution will resume at this point if the
  // watchdog is configured for resume rather than restart
 
  // When awake, disable sleep mode
  sleep_disable(); 
  }
 
  // put everything on again
  power_all_enable(); 
}

// Interrupt routine
ISR(WDT_vect) {
    // Check if we are in sleep mode or it is a genuine WDR.
    if(nbr_remaining > 0) {
        // not hang out, just waiting
        // decrease the number of remaining avail loops
        nbr_remaining = nbr_remaining - 1;
        wdt_reset();
    }
    else {
        // must be rebooted
        // configure
        MCUSR = 0;                          // reset flags
                                            // Put timer in reset-only mode:
        WDTCSR |= 0b00011000;               // Enter config mode.
        WDTCSR =  0b00001000 | 0b000000;    // clr WDIE (interrupt enable...7th from left)
                                            // set WDE (reset enable...4th from left), and set delay interval
                                            // reset system in 16 ms...
                                            // unless wdt_disable() in loop() is reached first
        // reboot
        while(1);
    }
}



//--------------------------------------------------------------------------------------
// function to configure the watchdog: let it sleep 8 seconds before firing
// when firing, configure it for resuming program execution by default
// hangs will correspond to watchdog fired when nbr_remaining <= 0 and will
// be determined in the ISR function

void configure_wdt(void) {
   cli();                           // disable interrupts for changing the registers

  MCUSR = 0;                       // reset status register flags

                                   // Put timer in interrupt-only mode:                                       
  WDTCSR |= 0b00011000;            // Set WDCE (5th from left) and WDE (4th from left) to enter config mode,
                                   // using bitwise OR assignment (leaves other bits unchanged).
  WDTCSR =  0b01000000 | 0b100001; // set WDIE: interrupt enabled
                                   // clr WDE: reset disabled
                                   // and set delay interval (right side of bar) to 8 seconds

  sei();                           // re-enable interrupts 
}
