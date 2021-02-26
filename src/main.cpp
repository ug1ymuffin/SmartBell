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
int received_data[560];
int display_ring_help_var = 0;
int btn_time = 0; //Time when button was pushed
bool button_pressed = false;
bool first_pos, second_pos;
bool ring_pos = LOW;
bool ring_idle = true;
bool button_locked = true;
bool boolSet;
int bzzz_amount = 0;

//timers
unsigned long bzz_check_period_std = (long)1000 * 60 * 5; //Ringing waiting period lenght
unsigned long bzz_check_period = bzz_check_period_std;
unsigned long btn_check_period = (long)1000; //Wait period for button and Blutooth
unsigned long display_update = (long)1000 * 1;
unsigned long display_blink_period = (long)100;
unsigned long button_check_interval = (long)1000;
unsigned long led_blink_interval = (long)100;
unsigned long timer_one = millis();
unsigned long button_check_timer = millis();
unsigned long timer_three = millis();
unsigned long timer_display_blink_timer = millis();
unsigned long blink_timer_one = millis();
unsigned long blink_timer_two = millis();
unsigned long ring_timer = millis();
unsigned long bzzz_lenght = 0;

//Functions used in program
void scheduleRead();
void scheduleSort();
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
LedControl lc = LedControl(8, 7, 9, 1);

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
  EEPROM.get(0, bzz_count);
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
  int cur_alarm_hour = Schedule[now.dayOfTheWeek() - 1][bzz_count][0];
  int cur_alarm_minute = Schedule[now.dayOfTheWeek() - 1][bzz_count][1];
  if (millis() - timer_one <= bzz_check_period)
  {
    if (now.hour() == cur_alarm_hour)
    {
      bzz_check_period = 1000 * 5;
      if (now.minute() == cur_alarm_minute)
      {
        bzzz_amount = (int)Schedule[3];
        bzzz_lenght = (long)Schedule[2];
        ring_idle = false;
        bzz_count++;
        EEPROM.put(0, bzz_count);
        bzz_check_period = bzz_check_period_std;
      }
    }
  }
  button_check();
  bzzz_mode();
  led_green_pos = blinking(LED_GREEN, led_green_pos, &blink_timer_one);
  if (button_pressed)
  {
    led_red_pos = blinking(LED_RED, led_red_pos, &blink_timer_two);
  }
  if (millis() - timer_three >= display_update and !button_pressed and ring_idle)
  {
    display_digits(now.hour(), now.minute(), cur_alarm_hour, cur_alarm_minute);
    timer_three = millis();
  }
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
  if (millis() - button_check_timer > button_check_interval and !button_locked)
  {
    second_pos = digitalRead(button);
    if ((first_pos = HIGH) and (second_pos == HIGH))
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
      switch (btn_time)
      {
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
      case 3:
        schedule_serial_input();
        break;
      case 5:
        break;
      case 6:
        hard_reset();
        break;
      case 7:

        break;
      case 8:
      default:
        break;
      }
      btn_time = 0;
      button_pressed = false;
      digitalWrite(LED_RED, LOW);
      led_red_pos = LOW;
    }
    button_check_timer = millis();
  }
  else
  {
    first_pos = digitalRead(button);
  }
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
    timer_three = display_blink_period;
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
  for (int i = 0; i < 256; i++)
  {
    EEPROM.put(i, 0);
  }
  for (int i = 1; i < 7; i++)
  {
    for (int j = 1; j < 20; j++)
    {
      for (int k = 1; k < 3; k++)
        Schedule[i][j][k] = 0;
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
        else if (confirm_data == 2)
        {
          scheduleRead();
          return 1;
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
  }
}
void bluetooth_translating()
{
  unsigned long local_timer = millis();
  int v = 0;
  int hours, minutes, seconds;
  while (millis() - local_timer <= 1000 * 60)
  {
    if (btSerial.available())
    {
      animation();
      blinking(LED_GREEN, led_green_pos, &blink_timer_one);
      blinking(LED_RED, led_red_pos, &blink_timer_two);
      data = btSerial.read();
      if (data > 0)
      {
        received_data[v] = data;
        Serial.println(received_data[v]);
        if (received_data[v] == 222 or received_data[v] == 111)
        {
          boolSet++;
          if (boolSet >= 7)
          {
            sorting_schedule();
          }
        }
      }
      if (data == 84)
      {
        for (int i = 0; i < 0; i++)
        {
          hours = btSerial.read();
          minutes = btSerial.read();
          seconds = btSerial.read();
        }
        //time_adjustment(hours, minutes, seconds);
      }
      v++;
    }
  }
}


void time_adjustment(h, m, s)
{
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
  return (now.day()*1000000 + now.month()*10000 + now.year());
}

int up_time(y, m, d){
  DateTime now = rtc.now();
  ut = (now.year() - y)*365 + (now.month()- m)*30 + (now.day() - d);
  return ut;
}