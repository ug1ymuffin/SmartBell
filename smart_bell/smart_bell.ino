#include <ArduinoSort.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <DS3231.h>
//standart position in schedule: hhmml; h - hours, m - minutes, l - lenth of the bell ring;
int standartSchedule[18];//arr about work days(using at workdays)
int currentSchedule[18];//current schedule(using at workdays and Saturdays(filled with suitable positions))
int rawSaturdaySchedule[4];//arr about saturday's lessons(using in satMode to fill currentSchedule). No ring time
int saturdaySchedule[18];//arr with timed saturday's rings.
int button_time = 0;//variable about how long button was pressed
int sortCount; //count variable for sorting
bool setupBlu = false;//If standart schedule is set - true, else - false.
bool satSetup = false;//If Saturday schedule is set - true, else - false.
int i, k, j;
int bzzCount = 0;
unsigned long lesson_check_period = (long)1000 * 60 * 5; //Ringing waiting period lenght
unsigned long bnb_check_period = (long) 1000 * 5; //Wait period for button and Blutooth
unsigned long timer_one;
unsigned long timer_two;
bool h12, PM;

SoftwareSerial BTserial(2, 3); // Blutooth module pins
LedControl lc=LedControl(12,11,10,1);
DS3231 Clock;
int button = 13;
int LED1_PIN = 7;
int RingRelay = 4;
int

void setup()
{
  Serial.begin(9600);
  BTserial.begin(9600);
  Wire.begin();
  //  setupBlu = EEPROM.readInt(111);
  if (setupBlu = true) {
    scheduleRead();
  }
  timer_one = millis();
  timer_two = millis();
  lc.shutdown(0,false);
  lc.setIntensity(0,8);
  lc.clearDisplay(0);
}
//Main loop. Responsible for bell rings.
void loop()
{
  //Wait for lesson_check_period
  if (millis() - timer_one >= lesson_check_period) {
    //If midnight, count for rings will be zero
    if ((Clock.getHour(h12, PM) == 0) and (Clock.getMinute() <= 5)) {
      bzzCount = 0;
      memcpy(standartSchedule, currentSchedule, 18);
      satSetup = false;
    }
    //Check if Dow is saturday and schedule isn't set up for saturday
    if (Clock.getDoW() == 6 and satSetup == false) {
      memcpy(saturdaySchedule, currentSchedule, 18);
      satSetup = true;
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
  if (millis() - timer_two >= bnb_check_period) {
    button_check();
    if (BTserial.available() && setupBlu == false) {
      bluReceive();

    }
  }
  display_digits(Clock.getHour(), Clock.getMinute(), currentSchedule[bzzCount]);
}
//receiving Data. Because I had some problems with reciving more than 64 numbers, I have to use this method.
void bluReceive() {
  digitalWrite(LED1_PIN, HIGH); // Let user know that, arduino knows that his phone connected
  scheduleReset();
  int rawRing[4];//local array contains info about lesson's start and end
  int countA = 0;
  int countB = 0;
  int countSat = 0;
  int receivedData;
  if (BTserial.read() == 0) {
    unsigned long timer = millis(); // Reseting timer
    //Receiving data about workday
    while (countA < 18) { //
      while (countB < 5) { //reciving data about one lesson
        // Wait for 0.1 second
        if (millis() - timer >= 100) {
          //Android is sending wrong digits, which is greater by 48. That's very strange.
          rawRing[countB] = BTserial.read() - 48;
          countB += 1;
          timer = millis(); //Reseting timer to wait for some time.
        }
      }
      //Sending array of digits to tranform it into one big number
      scheduleSort(rawRing);
      countA += 1;
    }
    timer = millis();
    //receiving data about saturday
    while (countSat < 5) {
      if (millis() - timer >= 100) {
        receivedData = BTserial.read() - 48;
        if (millis() - timer >= 200) {
          saturdaySchedule[countSat] = receivedData * 10 + (BTserial.read() - 48);
        }
      }
    }
    setupBlu = true;
    //
    //EEPROM.update(111, setupBlu);
    scheduleWrite(standartSchedule, saturdaySchedule);
    digitalWrite(LED1_PIN, LOW);
  }
}
//Collecting array of time pieces into one integer
void scheduleSort(int*ringTime) {
  standartSchedule[sortCount] = ringTime[0] * 10000 + ringTime[1] * 1000 + ringTime[2] * 100 + ringTime[3] * 10 + ringTime[4];
}
//Writing current schedules to EEPROM
void scheduleWrite(int*schedule_std, int*schedule_sat) {
  int k, i = 0;
  while (i < 18) {
    EEPROM.put(k, standartSchedule[i]);
    i++;
    k = k + 2;
  }
  i = 0;
  while (i < 4) {
    EEPROM.put(k, saturdaySchedule[i]);
    k = k + 2;
    i++;
  }
}
//Almost the same as Writing

void scheduleRead() {
  int k, i = 0;
  while (i < 18) {
    EEPROM.get(k, standartSchedule[i]);
    i++;
    k = k + 2;
  }
  i = 0;
  while (i< 4) {
    EEPROM.get(k, saturdaySchedule[i]);
    k = k + 2;
    i++;
  }
}

//Creating schedule for saturday

void saturdayScheduleMaker() {
  int countSat;
  int currTime = saturdaySchedule[1];
  scheduleReset();
  for (int i = 0; i < saturdaySchedule[0] * 2 + 1; i++) {
    for (int k; k < 2; k++) {
      if (currTime % 100 + saturdaySchedule[2 + k] >= 60) {
        currTime = (currTime / 100 + 1) * 100 + (currTime % 100 + saturdaySchedule[2 + k]) % 60;
      }
      else {
        currTime = currTime + saturdaySchedule[2 + k];
      }
      currentSchedule[i] = currTime * 10;
    }
  }
  setupBlu = true;
}
void scheduleReset() {
  for (int i = 0; i < 18; i++) {
    currentSchedule[i] = 0;
  }
}
void button_check() {
  bool first_pos, second_pos;
  unsigned long local_timer = millis();
  first_pos = digitalRead(button);
  while (millis() - local_timer < 100 ) {
    second_pos = digitalRead(button);
  }
  if ((first_pos == second_pos) and (first_pos == 1)) {
    button_time += 5;
    blinking();
  }
  else {
    switch (button_time) {
      case 5 :  bzzz_mode(0); break;
      case 10: bzzz_mode(1); break;
      case 15: setupBlu = false; break;
      case 30: hard_reset(); break;
      default: break;
    }
  }
}
void blinking(){
  unsigned long local_timer = millis();
  while(millis() - local_timer <= 50){
    digitalWrite(LED1_PIN, HIGH);
  }
  digitalWrite(LED1_PIN, LOW);
}
void bzzz_mode(int lenght){
  unsigned long local_timer = millis();
  while(millis()- local_timer <= 1000 + 4000*lenght){
    digitalWrite(RingRelay, HIGH);
  }
  digitalWrite(RingRelay, LOW);
}
void display_digits(int now_hours, int now_mins, int closest_ring){
  lc.setDigit(0, 0, now_hours/10, false);
  lc.setDigit(0, 1, now_hours%10, false);
  lc.setDigit(0, 2, now_mins/10, false);
  lc.setDigit(0, 3, now_mins%10, false);
  lc.setDigit(0, 4, closest_ring/1000, false);
  lc.setDigit(0, 5, (closest_ring%1000)/100, false);
  lc.setDigit(0, 6, ((closest_ring%1000)%100)/10, false);
  lc.setDigit(0, 7, ((closest_ring%1000)%100)%10, false);
}
