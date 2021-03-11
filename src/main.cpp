#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LedControl.h>
#include <RTClib.h>
#include <Arduino.h>
//values for program

//data about schedule and alarms
uint8_t bzz_count;
int Schedule[7][20][4]; //arr about work days(using at workdays). Format of every elem: [[hour, minute, length, amount], ... [hour, minute, lenth, amount]]
int display_ring_help_var = 0;
int btn_time = 0; //Time when button was pushed
bool button_pressed = false;
bool first_pos, second_pos;
bool ring_pos = LOW;
bool ring_idle = true;
bool button_locked = true;
int bzzz_amount = 0;

//timers
unsigned long bzz_check_period_std = (long)100; //Ringing waiting period lenght
unsigned long bzz_check_period = bzz_check_period_std;
int btn_check_period = 1000; //Wait period for button and Blutooth
unsigned long display_update = 1000;
unsigned long display_blink_period = 100;
unsigned long button_check_interval = 1000;
unsigned long led_blink_interval = 100;
unsigned long timer_one = millis();
unsigned long button_check_timer = millis();
unsigned long timer_three = millis();
unsigned long timer_display_blink_timer = millis();
unsigned long blink_timer = millis();
unsigned long ring_timer = millis();
unsigned long bzzz_lenght = 0;

//Functions used in program
void scheduleRead();
int nearest_alarm(int d, int h, int m);
bool schedule_write();
void schedule_serial_input();
void button_check();
bool blinking(int LED, bool led_pos, unsigned long *blinking_timer);
void bzzz_mode();
void display_digits(int now_hours, int now_minutes, int closest_ring_hours, int closest_ring_minutes);
void hard_reset();
void display_button_push(int count);
void animation();
void bluetooth_translating();

//values for devices

//RTC:
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//display
LedControl lc = LedControl(9, 8, 7, 1);

//bluetooth
SoftwareSerial btSerial(2, 3);

//button
int button = 5;

//LEDs
int LED_GREEN = 10;
bool led_green_pos = HIGH;
int LED_RED = 11;
bool led_red_pos = HIGH;

//relay
int ring_relay = 4;

void setup()
{
  DateTime now = rtc.now();
  bzz_count = nearest_alarm((now.dayOfTheWeek() + 6) % 7, now.hour(), now.minute());
  Serial.begin(9600);
  btSerial.begin(9600);
  Serial.setTimeout(10000);
  Wire.begin();
  scheduleRead();
  lc.shutdown(0, false);
  lc.setIntensity(0, 5);
  lc.clearDisplay(0);
  pinMode(button, INPUT);
  pinMode(ring_relay, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  // Rewriting again
}
//Main loop. Responsible for bell rings.
void loop()
{
  DateTime now = rtc.now();
  //Whole main loop is full of errors, because new library
  //Some functions be like afsgshth wareehtr
  //Wait for lesson_check_period

  int cur_alarm_hour = Schedule[(now.dayOfTheWeek() + 6) % 7][bzz_count][0];
  int cur_alarm_minute = Schedule[(now.dayOfTheWeek() + 6) % 7][bzz_count][1];
  if (now.hour() == cur_alarm_hour)
  {
    bzz_check_period = 1000;
    if (now.minute() == cur_alarm_minute)
    {
      bzzz_amount = Schedule[(now.dayOfTheWeek() + 6) % 7][bzz_count][2] * 2;
      bzzz_lenght = Schedule[(now.dayOfTheWeek() + 6) % 7][bzz_count][3];
      Serial.print(bzzz_amount);
      Serial.print(bzzz_lenght);
      ring_idle = false;
      EEPROM.put(0, bzz_count);
      bzz_check_period = bzz_check_period_std;
    }
  }

  button_check();
  bzzz_mode();
  if (button_pressed)
  {
    led_red_pos = blinking(LED_RED, led_red_pos, &blink_timer);
  }
  else
  {
    led_green_pos = blinking(LED_GREEN, led_green_pos, &blink_timer);
  }
  if ((millis() - timer_three >= display_update) and !button_pressed and ring_idle)
  {

    display_digits(now.hour(), now.minute(), cur_alarm_hour, cur_alarm_minute);
    timer_three = millis();
  }
  bzz_count = nearest_alarm((now.dayOfTheWeek() + 6) % 7, now.hour(), now.minute());
}

void scheduleRead()
{
  unsigned long data;
  int l = 1;
  for (int i = 0; i < 7; i++)
  {
    for (int j = 0; j < 20; j++)
    {
      EEPROM.get(l, data);
      Schedule[i][j][0] = data / 100000 % 10 * 10 + data / 10000 % 10;
      Schedule[i][j][1] = data / 1000 % 10 * 10 + data / 100 % 10;
      Schedule[i][j][2] = data / 10 % 10;
      Schedule[i][j][3] = data % 10;
      if (data != 0)
      {
        Serial.println(data);
        for (int k = 0; k < 4; k++)
        {
          Serial.println(Schedule[i][j][k]);
        }
        Serial.println();
        Serial.println(l);
      }
      l += 4;
    }
  }
}

void button_check()
{
  if ((millis() - button_check_timer > button_check_interval))
  {
    button_check_timer = millis();
    first_pos = digitalRead(button);
    if (second_pos == true and first_pos == true)
    {
      button_pressed = true;
      btn_time++;
      if (btn_time > 9)
      {
        btn_time = 0;
      }
      display_button_push(btn_time);
    }
    else
    {
      if (button_pressed)
      {
        switch (btn_time)
        {
        default:
          break;
        case 1:
          ring_idle = false;
          bzzz_lenght = 1;
          bzzz_amount = 2;
          break;

        case 2:
          ring_idle = false;
          bzzz_amount = 4;
          bzzz_lenght = 2;
          break;
        case 4:
          bluetooth_translating();
          break;
        case 6:
          hard_reset();
          scheduleRead();
          break;
        }
        digitalWrite(LED_RED, LOW);
        led_red_pos = LOW;
      }
      btn_time = 0;
      button_pressed = false;
    }
  }
  second_pos = first_pos;
}

bool blinking(int LED, bool led_pos, unsigned long *blink_timer)
{
  if (millis() - *blink_timer > led_blink_interval)
  {
    led_pos = !led_pos;
    *blink_timer = millis();
  }
  digitalWrite(LED, led_pos);
  return led_pos;
}

void bzzz_mode()
{
  // ringing_display();
  if (bzzz_amount > 0)
  {
    button_locked = true;
    animation();
    if ((millis() - ring_timer > bzzz_lenght * 1000 * (bzzz_amount % 2) + ((bzzz_amount + 1) % 2) * 1000) and !ring_idle)
    {
      ring_timer = millis();
      ring_pos = !ring_pos;
      bzzz_amount--;
      digitalWrite(ring_relay, ring_pos);
      Serial.println("Ringing" + String(bzzz_amount));
    }
  }
  else
  {
    button_locked = false;
    ring_idle = true;
  }
}

void display_digits(int now_hours, int now_mins, int closest_ring_hours, int closest_ring_minutes)
{
  lc.clearDisplay(0);
  lc.setDigit(0, 3, now_hours / 10, false);
  lc.setDigit(0, 2, now_hours % 10, true);
  lc.setDigit(0, 1, now_mins / 10, false);
  lc.setDigit(0, 0, now_mins % 10, false);
  lc.setDigit(0, 7, closest_ring_hours / 10, false);
  lc.setDigit(0, 6, closest_ring_hours % 10, true);
  lc.setDigit(0, 5, closest_ring_minutes / 10, false);
  lc.setDigit(0, 4, closest_ring_minutes % 10, false);
}

void display_button_push(int count)
{
  lc.clearDisplay(0);
  lc.setDigit(0, 0, count, false);
}

void animation()
{
  if (millis() - timer_three > display_blink_period)
  {
    char signs[2] = {'-', '_'};
    lc.clearDisplay(0);
    timer_three = millis();
    for (int i = 0; i < 8; i++)
    {
      lc.setChar(0, i, signs[(i + display_ring_help_var) % 2], false);
    }
    display_ring_help_var++;
  }
}

void hard_reset()
{
  unsigned long data;
  int l = 1;
  for (int i = 0; i < 7; i++)
  {
    for (int j = 0; j < 20; j++)
    {
      data = 0;
      EEPROM.put(l, data);
      if (data != 0)
      {
        Serial.println(data);
      }
      l += 4;
    }
  }
}

void schedule_serial_input()
{
  int eeprom_count = 1;
  for (int i = 0; i < 7; i++)
  {
    int j = 0;
    while (j < 20)
    {
      Serial.setTimeout(10000);
      Serial.println("Input time and ring params about " + String(j) + " for day " + String(i) + " in format hhmmla");
      unsigned long data = Serial.parseInt();
      if (data / 100000 == 0)
      {
        Serial.setTimeout(5000);
        Serial.println("Nothing inputed or or wrong data, print 1 to skip day or 2 to stop input. Current input is " + String(data));
        int confirm_data = Serial.parseInt();
        if (confirm_data == 1)
        {
          for (int k = 20 - j; k >= 0; k--)
          {
            EEPROM.put(eeprom_count, 0);
            unsigned long data_from_eeprom = 0;
            Serial.println("Wrote " + String(EEPROM.get(eeprom_count, data_from_eeprom)) + " into index " + String(eeprom_count));
            eeprom_count += 4;
          }
          break;
        }
        else
        {
          Serial.println("Succeed " + String(data));
          Serial.println("Previous EEPROM cell value " + EEPROM.read(eeprom_count));
          EEPROM.put(eeprom_count, data);
          unsigned long data_from_eeprom = 0;
          EEPROM.get(eeprom_count, data_from_eeprom);
          Serial.println("Wrote " + String(data_from_eeprom) + " into index " + String(eeprom_count));
          eeprom_count += 4;
          j++;
        }
      }
    }
  }
  scheduleRead();
}

void bluetooth_translating()
{
  int data = 0;
  bzz_count = 0;
  int i = 0;
  int j = 0;
  int k = 0;
  while (i < 7)
  {
    animation();
    blinking(LED_RED, led_red_pos, &blink_timer);
    if (btSerial.available())
    {
      data = btSerial.read();
      if (data != 0)
      {
        switch (data - 1)
        {
        case 111:
          k++;
          break;
        case 222:
          Serial.print(Schedule[i][j][0]);
          Serial.print(Schedule[i][j][1]);
          Serial.print(Schedule[i][j][2]);
          Serial.print(Schedule[i][j][3]);
          Serial.println();
          j++;
          k = 0;
          break;
        case 33:
          i++;
          j = 0;
          k = 0;
          break;
        default:
          Schedule[i][j][k] = data - 1;
        }
      }
    }
  }
  schedule_write();
}
int nearest_alarm(int d, int h, int m)
{
  int na = 20;

  for (int i = 0; i < 20; i++)
  {
    if(Schedule[d][i][0] > h or (Schedule[d][i][1] > m and Schedule[d][i][0] == h)){
      na = i;
      Serial.print(bzz_count);
      break;
    }
  }
  return na;
}
bool schedule_write()
{
  unsigned long data;
  int l = 1;
  for (int i = 0; i < 7; i++)
  {
    for (int j = 0; j < 20; j++)
    {
      data = Schedule[i][j][0] * 10000 + Schedule[i][j][1] * 100 + Schedule[i][j][2] * 10 + Schedule[i][j][3];
      EEPROM.put(l, data);
      if (data != 0)
      {
        Serial.println(data);
      }
      l += 4;
    }
  }
  return true;
}