#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Pin Definitions
#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER "pool.ntp.org"

// Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// Global Variables for Time and Alarms
int days = 0, hours = 0, minutes = 0, seconds = 0;
int UTC_OFFSET = 19800; // Default UTC+5:30
int n_alarms = 2;
int alarm_hours[] = {0, 0};
int alarm_minutes[] = {0, 0};
bool alarm_active[] = {false, false};  // Track whether each alarm is active

// Notes for Buzzer Alarm
int C = 262, D = 295, E = 330, F = 349, G = 394, A = 440, B = 496, C_H = 532;
int notes[] = {C, D, E, F, G, A, B, C_H};
int n_notes = 8;
// Menu System Variables
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 -  Set  Time Zone", "2 -  Set  Alarm 1", "3 -  Set  Alarm 2", "4 -  View Alarms", "5 - Delete an Alarm"};

// Function to Display Text on OLED
void print_line(String text, int column, int row, int text_size){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

void setup(){
  // Configure pins
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  // Initialize DHT Sensor
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  // Initialize serial monitor and OLED display
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // turn on OLED display
  display.display();
  delay(1000);
  // Connect to WiFi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    display.clearDisplay();
    print_line("Connecting to wifi", 0, 0, 2);
  }

  // Display WiFi Connection Success
  display.clearDisplay();
  print_line("Connected to wifi", 0, 0, 2);
  delay(1000);
  configTime(UTC_OFFSET, 0, NTP_SERVER);

  // Welcome Message
  display.clearDisplay();
  print_line("Welcome  to Medibox!", 10, 0, 2);
  delay(1000);
  display.clearDisplay();
}

void print_time_now(void){
  display.clearDisplay();
  // Format the time manually (HH:MM:SS with leading zeros)
  String formattedTime = "";
  if (hours < 10){
    formattedTime += "0";
  }
  formattedTime += String(hours) + ":";
  if (minutes < 10){
    formattedTime += "0";
  }
  formattedTime += String(minutes) + ":";
  if (seconds < 10){
    formattedTime += "0";
  }
  formattedTime += String(seconds);

  print_line(formattedTime, 0, 0, 2);
}

// Function to Update and Print Current Time
void update_time(void){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
}

// Function to Wait for Button Press and Return Pressed Button
int wait_for_button_press(){
  while (true)
  {
    if (digitalRead(PB_UP) == LOW)
    {
      delay(100);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW)
    {
      delay(100);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW)
    {
      delay(100);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW)
    {
      delay(100);
      return PB_CANCEL;
    }
    update_time();
  }
}

void set_time_zone(){
  int temp_hour = UTC_OFFSET / 3600;
  int temp_minute = (UTC_OFFSET % 3600) / 60;
  
  while (true){
    display.clearDisplay();
    print_line("Enter UTC Hour: " + String(temp_hour), 0, 0, 2);
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP){
      temp_hour++;
    } else if (pressed == PB_DOWN){
      temp_hour--;
    } else if (pressed == PB_OK){
      break;
    } else if (pressed == PB_CANCEL){
      return;
    }
  }

  while (true){
    display.clearDisplay();
    print_line("Enter UTC Minute: " + String(temp_minute), 0, 0, 2);
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP){
      temp_minute += 15; // Increment in steps of 15 minutes
      if (temp_minute >= 60) temp_minute = 0;
    } else if (pressed == PB_DOWN){
      temp_minute -= 15;
      if (temp_minute < 0) temp_minute = 45;
    } else if (pressed == PB_OK){
      break;
    } else if (pressed == PB_CANCEL){
      return;
    }
  }

  UTC_OFFSET = (temp_hour * 3600) + (temp_minute * 60);
  configTime(UTC_OFFSET, 0, NTP_SERVER);
  update_time();
  
  display.clearDisplay();
  print_line("Time Zone Set", 0, 0, 2);
  delay(1000);
}

void view_alarms(){
  display.clearDisplay();
  print_line("Alarm 1 : " + (alarm_active[0] ? (String(alarm_hours[0]) + ":" + String(alarm_minutes[0])) : "Inactive"), 0, 0, 2);
  print_line("Alarm 2 : " + (alarm_active[1] ? (String(alarm_hours[1]) + ":" + String(alarm_minutes[1])) : "Inactive"), 0, 30, 2);
  delay(3000);
}

void delete_alarm(){
  int selected_alarm = 0;
  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm: " + String(selected_alarm + 1), 0, 0, 2);
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP) {
      selected_alarm = (selected_alarm + 1) % n_alarms; // Cycle forward
    } 
    if (pressed == PB_DOWN) {
      selected_alarm = (selected_alarm - 1 + n_alarms) % n_alarms; // Cycle backward
    } 
    if (pressed == PB_OK) {
      alarm_active[selected_alarm] = false; // Deactivate the alarm
      alarm_hours[selected_alarm] = 0;
      alarm_minutes[selected_alarm] = 0;
      display.clearDisplay();
      print_line("Alarm " + String(selected_alarm + 1) + " Deleted", 0, 0, 2);
      delay(1000);
      break;
    } 
    if (pressed == PB_CANCEL) {
      break;
    }
  }
}

void snooze_alarm(int index){
  alarm_minutes[index] += 5;
  if (alarm_minutes[index] >= 60) {
    alarm_minutes[index] -= 60; // Reset minutes to valid range
    alarm_hours[index]++;       // Increment hour

    if (alarm_hours[index] >= 24) {
      alarm_hours[index] = 0; // Reset hour if it exceeds 23 (24-hour format)
    }
  }
}

void ring_alarm(int num) {
  display.clearDisplay();
  print_line("MEDICINE  TIME!", 0, 0, 2);
  digitalWrite(LED_1, HIGH);

  bool alarm_cancelled = false;
  bool alarm_completed = false;
  unsigned long start_time = millis();
  const unsigned long alarm_duration = 60000; // 1 minute alarm duration

  // Ring the buzzer for alarm_duration or until cancelled
  while (!alarm_cancelled && (millis() - start_time) < alarm_duration) {
    // Check if we're still in the alarm minute
    update_time();
    if (!(alarm_hours[num] == hours && alarm_minutes[num] == minutes)) {
      break;
    }

    // Play alarm tone sequence
    for (int i = 0; i < n_notes && !alarm_cancelled; i++) {
      tone(BUZZER, notes[i], 200);
      delay(300);
      noTone(BUZZER);
      delay(100);
      
      // Check for cancel button
      if (digitalRead(PB_CANCEL) == LOW) {
        alarm_cancelled = true;
        delay(100); // debounce
        break;
      }
    }

    // Check if we completed full duration without cancellation
    if (!alarm_cancelled && (millis() - start_time) >= alarm_duration) {
      alarm_completed = true;
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
  display.display();

  // Only snooze if:
  // 1. Alarm completed full duration without cancellation AND
  // 2. We're still in the alarm minute
  if (alarm_completed && alarm_hours[num] == hours && alarm_minutes[num] == minutes) {
    snooze_alarm(num);
    
    // Show snooze confirmation
    display.clearDisplay();
    print_line("Snoozed for 5", 0, 0, 2);
    print_line("minutes", 0, 25, 2);
    display.display();
    delay(1000);
  }
}

void update_time_with_check_alarm(void){
  configTime(UTC_OFFSET, 0, NTP_SERVER);
  update_time();
  display.clearDisplay();
  print_time_now();

  
  for (int i = 0; i < n_alarms; i++)
  {
    if (alarm_active[i] == true && alarm_hours[i] == hours && alarm_minutes[i] == minutes && seconds == 0)
    {
      ring_alarm(i);
    }
  }
}

void set_alarm(int alarm_index){

  int temp_hour = hours;
  while (true)
  {
    display.clearDisplay();
    print_line("Set Alarm: " + String(alarm_index + 1), 0, 0, 2);
    print_line("Enter hour: " + String(temp_hour), 0, 30, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP)
    {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    if (pressed == PB_DOWN)
    {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0)
      {
        temp_hour = 23;
      }
    }

    if (pressed == PB_OK)
    {
      delay(200);
      alarm_hours[alarm_index] = temp_hour;
      break;
    }

    if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  int temp_minute = minutes;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP)
    {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    if (pressed == PB_DOWN)
    {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0)
      {
        temp_minute = 59;
      }
    }

    if (pressed == PB_OK)
    {
      delay(200);
      alarm_minutes[alarm_index] = temp_minute;
      break;
    }

    if (pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  alarm_active[alarm_index] = true;
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}

void run_mode(int mode){
  if (mode == 0){
    set_time_zone();
  }
  else if (mode == 1 || mode == 2){
    set_alarm(mode - 1);
  }
  else if (mode == 3){
    view_alarms();
  }
  else if (mode == 4){
    delete_alarm();
  }
}

void go_to_menu(){
  current_mode = 0;
  while (digitalRead(PB_CANCEL) == HIGH){
    display.clearDisplay();
    print_line("MAIN MENU", 0, 0, 1);
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    print_line(modes[current_mode], 0, 20, 2);
    
    int pressed = wait_for_button_press();
    if (pressed == PB_UP){
      delay(100);
      current_mode = (current_mode + 1) % max_modes;
    }
    else if (pressed == PB_DOWN){
      delay(100);
      current_mode -= 1;
      if (current_mode < 0){
        current_mode = max_modes - 1;
      }
    }
    else if (pressed == PB_OK){
      delay(100);
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL){
      delay(100);
      break;
    }
    update_time();
  }
}

void check_temp(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  // Fill a row with spaces
  display.setCursor(0, 40);
  for(int i = 0; i < 10; i++) {
    display.print(" ");
  }
  display.setCursor(0, 50);
  for(int i = 0; i < 14; i++) {
    display.print(" ");
  }
  display.display();


  if (data.temperature > 32)
  {
    print_line("TEMP HIGH", 0, 40, 1);
  }
  else if (data.temperature < 24)
  {
    print_line("TEMP LOW", 0, 40, 2);
  }

  if (data.humidity > 80)
  {
    print_line("HUMIDITY HIGH", 0, 50, 1);
  }
  else if (data.humidity < 65)
  {
    print_line("HUMIDITY LOW", 0, 50, 1);
  }
}

void loop(){
  // put your main code here, to run repeatedly:
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW)
  {
    delay(70);
    // Enter menu system
    go_to_menu();
  }
  check_temp();
}