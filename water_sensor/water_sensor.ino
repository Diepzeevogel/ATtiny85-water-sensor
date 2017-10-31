#include <avr/sleep.h>
#include <avr/wdt.h>

unsigned char watchdog_counter = 0;
unsigned char battery_counter = 0;
int water_average = 0;
const byte water_sensor = A1;

void setup() {
  /* TODO:
  Turn RF transmitter off, but ready to use.
  */
  
  for(int x=0; x<8; x++){
    water_average += analogRead(water_sensor);
  }
  water_average /= 8;
  
  //pinMode(water_sensor, INPUT_PULLUP);
  pinMode(2, INPUT); //Input
  digitalWrite(2, HIGH); //Pullup
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
}

ISR(WDT_vect){
  watchdog_counter++;
}

void loop() {
  ADCSRA &= ~(1<<ADEN); //Disable ADC for water measurement.
  setup_watchdog(9); //Start the watchdog.
  sleep_mode(); //Shut the microcontroller down.

  if(watchdog_counter > 15){ //15 because 120 seconds => Runs every 2 minutes.
    watchdog_counter = 0;
    battery_counter++;
    
    if(check_water()){
      transmit_message("WATER DETECTED");
    }
    else if(battery_counter > 30){ //Runs every 
      battery_counter = 0;
      if(check_battery()){
        transmit_message("BATTERY LOW");
      }
    }
    
  }
}

bool check_water(){
  //Turn on ADC for water measurement
  //Do the measurement
  //Is there water, yes or no, return bool.
  
  ADCSRA |= (1<<ADEN); //Enable ADC
  if(abs(analogRead(water_sensor) - water_average) > 100){
    ADCSRA &= ~(1<<ADEN); //Disable ADC
    return true;
  }
  ADCSRA &= ~(1<<ADEN); //Disable ADC
  return false;
}

bool check_battery(){
  long voltage = readVcc();
  if(voltage < 3000L) return true;
  else return false;
}






//Enable and disable ADC first?????





long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(MUX3) | _BV(MUX2);
  
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void transmit_message(String message){
  //Turn on RF transmitter.
  //Send the message.
  //Turn off RF transmitter.
}

void setup_watchdog(int timer_prescaler) {
  if (timer_prescaler > 9 ) timer_prescaler = 9; //Limit incoming amount to legal settings

  byte bb = timer_prescaler & 7; 
  if (timer_prescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}
