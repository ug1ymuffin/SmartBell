#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Arduino.h>
//values for program

//data about schedule and alarms
int Schedule[7][20][3];//arr about work days(using at workdays). Format of every elem: [[hour, minute, lenth], ... [hour, minute, lenth],]
int currentAlarm[3];
int green_pos = LOW;
int red_pos = LOW;

//timers
unsigned long lesson_check_period = (long)1000 * 60 * 5; //Ringing waiting period lenght
unsigned long btn_check_period = (long) 1000 * 5; //Wait period for button and Blutooth
unsigned long led_blink_and_button_check_interval = (long) 50 //Led Bli

//values for devices

//RTC:
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//display
LedControl lc=LedControl(12,11,10,1);

//button
int button = 13;

//LEDs
int LED_GREEN = 7;
int LED2_RED = 6;

//relay
int RingRelay = 4;

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  scheduleRead();
  timer_one = millis();
  timer_two = millis();
  // Rewriting again
  lc.shutdown(0,false);
  lc.setIntensity(0,8);
  lc.clearDisplay(0);
}
//Main loop. Responsible for bell rings.
void loop()
{
  //Whole main loop is full of errors, because new library
  //Some functions be like afsgshth wareehtr
  //Wait for lesson_check_period
  if (millis() - timer_one >= lesson_check_period) {
    //If midnight, count for rings will be zero
    if ((Clock.getHour(h12, PM) == 0) and (Clock.getMinute() <= 10)) {
      bzzCount = 0;
    }
    //Main part of the bell
    //Check if current hour is equal to current ring hour and if it workday. If yes, setting wait period to shorter
    if (Clock.getHour(h12, PM) == (currentSchedule[bzzCount]) / 1000 and Clock.getDoW() != 7) {
      //Check if less than 5 minutes until next bell ring. If yes, setting wait period to shorter
      if ((currentSchedule[bzzCount] % 1000) / 10 - Clock.getMinute() <= 5) {
        //Check if current time(hhmm) equal to current bell ring time. If yes, starting BZZZ.
        if ((currentSchedule[bzzCount] % 1000) / 10 == Clock.getMinute()) {
          bzzz_mode(currentSchedule[bzzCount]);
          //Reset wait time to longest value
          lesson_check_period = (long) 1000 * 60 * 5;
        }
        lesson_check_period = (long) 1000 * 10;
      }
      lesson_check_period = (long) 1000 * 60;
    }
  }
  if (millis() - timer_two >= btn_check_period) {
    button_check();

    }
  }
  while (millis() - timer_three >= 100) {

  }
  display_digits(Clock.getHour(h12, PM), Clock.getMinute(), currentSchedule[bzzCount]);
  Serial.print(Clock.getHour(h12, PM));
  Serial.println(Clock.getMinute());
}

//Looks strange
//Collecting array of time pieces into one integer
void scheduleSort(int*ringTime) {
  standartSchedule[sortCount] = ringTime[0] * 10000 + ringTime[1] * 1000 + ringTime[2] * 100 + ringTime[3] * 10 + ringTime[4];
}
// Probblems!!! Wrong reading
void scheduleRead() {
  int k, i = 0;
  while (i < 18) {
    EEPROM.get(k, standartSchedule[i]);
    i++;
    k = k + 1;
  }
}

void button_check() {
  bool first_pos, second_pos;
  unsigned long local_timer = millis();
  int btn_time = 0; //Time when button was pushed
  first_pos = digitalRead(button);
  while(second_pos == HIGH){
    if(millis() - local_timer >= led_blink_and_button_check_interval){
         second_pos = digitalRead(button);
          green_pos = blinking(GREEN_LED, green_pos);
      
    }
  }
  if((second_pos == first_pos) and (fisrt_pos == HIGH){
    switch (btn_time) {
      case 5 :  bzzz_mode(0); break;
      case 10: bzzz_mode(1); break;
      case 15: setupBlu = false; break;
      case 30: hard_reset(); break;
      default: break;
    }
  }
}
bool blinking(LED, pos){
  digitalWrite(LED, !pos);
  pos = !pos;
}
void bzzz_mode(int lenght){
  unsigned long local_timer = millis();
  while(millis()- local_timer <= 1000 + 4000*lenght){
    digitalWrite(RingRelay, HIGH);
  }
  digitalWrite(RingRelay, LOW);
}
void display_digits(int now_hours, int now_mins, int closest_ring_hours, int closest_ring_minutes){
  /*Maybe some day it will become more beautiful*/
  unsigned long local_timer = millis()
  lc.setDigit(0, 0, now_hours/10, false);
  lc.setDigit(0, 1, now_hours%10, false);
  lc.setDigit(0, 2, now_mins/10, false);
  lc.setDigit(0, 3, now_mins%10, false);
  lc.setDigit(0, 4, closest_ring/1000, false);
  lc.setDigit(0, 5, (closest_ring%1000)/100, false);
  lc.setDigit(0, 6, ((closest_ring%1000)%100)/10, false);
  lc.setDigit(0, 7, ((closest_ring%1000)%100)%10, false);
}
void hard_reset(){
  scheduleReset();
  for(int i = 0; i < 256; i++){
    EEPROM.put(i, 0);
  }
}
