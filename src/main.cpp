#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <DigitLedDisplay.h>
#include <RTClib.h>
#include <Arduino.h>
//values for program

//data about schedule and alarms
int Schedule[7][20][3];//arr about work days(using at workdays). Format of every elem: [[hour, minute, length], ... [hour, minute, lenth],]
int bzzCount = 0;

//timers
unsigned long lesson_check_period_std = (long)1000 * 60 * 5; //Ringing waiting period lenght
unsigned long lesson_check_period = lesson_check_period_std;
unsigned long btn_check_period = (long) 1000 * 5; //Wait period for button and Blutooth
unsigned long led_blink_and_button_check_interval = (long) 50; //Led Blinking interval
unsigned long timer_one = millis();
unsigned long timer_two = millis();

//Functions used in program
void scheduleRead();
void scheduleSort();
void button_check();
bool blinking(int LED, bool pos);
void bzzz_mode(unsigned long lenght);
void display_digits(int now_hours, int now_minutes, int closest_ring_hours, int closest_ring_minutes);
void hard_reset();



//values for devices

//RTC:
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//display
DigitLedDisplay ld = DigitLedDisplay(7, 5, 6);

//button
int button = 13;

//LEDs
int LED_GREEN = 7;
bool green_pos = LOW;
int LED_RED = 6;
bool red_pos = LOW;

//relay
int RingRelay = 4;

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  scheduleRead();
  ld.setBright(7);
  ld.setDigitLimit(8);
  // Rewriting again
}
//Main loop. Responsible for bell rings.
void loop()
{
  DateTime now = rtc.now();
  //Whole main loop is full of errors, because new library
  //Some functions be like afsgshth wareehtr
  //Wait for lesson_check_period
  int cur_alarm_hour;
  int cur_alarm_minute;
  unsigned long cur_alarm_legth;
  if (millis() - timer_one >= lesson_check_period) {
    cur_alarm_hour = Schedule[now.dayOfTheWeek()][bzzCount][1];
    cur_alarm_minute = Schedule[now.dayOfTheWeek()][bzzCount][2];
    cur_alarm_legth = Schedule[now.dayOfTheWeek()][bzzCount][3];
    if(now.hour() == cur_alarm_hour){
      lesson_check_period = 1000 * 5;
      if(now.minute() == cur_alarm_minute){
        bzzz_mode(cur_alarm_legth);
        bzzCount++;
      }
    }
  }
  if (millis() - timer_two >= btn_check_period) {
    button_check();
  }
  display_digits(now.hour(), now.minute(), cur_alarm_hour, cur_alarm_minute);
}

//Looks strange
// Probblems!!! Wrong reading
void scheduleRead(){
  int data;
  int l;
  for(int i = 0; i <7; i++){
    for(int j = 0; j < 20; j++){
      data = EEPROM[l];
      Schedule[i][j][0] = (data/10000)*10 + (data/1000)%10;
      Schedule[i][j][1] = ((data/100)%10)*10 + (data/10)%10;
      Schedule[i][j][2] = data%10;
      l++;
    }
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
      green_pos = blinking(LED_GREEN, green_pos);
      btn_time++;
<<<<<<< Updated upstream
=======
      if (btn_time > 15)
      {
        btn_time = 0;
      }
      display_button_push(btn_time);
>>>>>>> Stashed changes
    }
  }
  if((second_pos == first_pos) and (first_pos == HIGH)){
    switch (btn_time) {
      case 1 :  bzzz_mode(0); break;
      case 2: bzzz_mode(1); break;
      case 6: hard_reset(); break;
      default: break;
    }
  }
}
bool blinking(int LED, bool pos){
  digitalWrite(LED, !pos);
  pos = !pos;
  return pos;
}
void bzzz_mode(unsigned long lenght){

  unsigned long local_timer = millis();
  while(millis()- local_timer <= 1000 + 4000*lenght){
    digitalWrite(RingRelay, HIGH);
  }
  digitalWrite(RingRelay, LOW);
}
//Needs rewrite, because new library. 
void display_digits(int now_hours, int now_mins, int closest_ring_hours, int closest_ring_minutes){
  ld.write(now_hours/10, 1);
  ld.write(now_hours%10, 2);
  ld.write(now_mins/10, 3);
  ld.write(now_mins%10, 4);
  ld.write(closest_ring_hours/10, 5);
  ld.write(closest_ring_hours%10, 6);
  ld.write(closest_ring_minutes/10, 7);
  ld.write(closest_ring_minutes%10, 8);
  ld.clear();
}
<<<<<<< Updated upstream
void hard_reset(){
  for(int i = 0; i < 256; i++){
    EEPROM.put(i, 0);
=======

void sorting_schedule()
{
  int p = 0;
  int data;

  for (int i = 0; i < 7; i++)
  {
    if (received_data[p] == 222)
    {
      p++;
      continue;
    }
    for (int j = 0; j < 20; j++)
    {
      if (received_data[p] == 111)
      {
        p++;
        break;
      }
      for (int k = 0; k < 4; k++)
      {
        Schedule[i][j][k] = received_data[p];
        p++;
        Serial.print(Schedule[i][j][k]);
      }
      Serial.println();
    }
>>>>>>> Stashed changes
  }
  for(int i = 1; i <7; i++){
    for(int j = 1; j < 20; j++){
      for(int k = 1; k < 3; k++)
      Schedule[i][j][k] = 0;
    }
  }
<<<<<<< Updated upstream
=======
}

void time_adjustment(h, m, s)
{
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
  return (now.day() * 1000000 + now.month() * 10000 + now.year());
}

int up_time(y, m, d)
{
  DateTime now = rtc.now();
  ut = (now.year() - y) * 365 + (now.month() - m) * 30 + (now.day() - d);
  return ut;
>>>>>>> Stashed changes
}