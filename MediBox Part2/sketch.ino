//include libraries
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

//declare OLED parameter
#define screen_width 128
#define screen_height 64
#define OLED_reset -1
#define screen_address 0x3C

//declare wifi time parameters
const char* ssid = "Wokwi-GUEST";
const char* password = ""; //if not open network enter password
int wifi_channel = 6;

//define for healthy condition for medicine
#define TEMP_HIGH 32
#define TEMP_LOW 26
#define HUMIDITY_HIGH 80
#define HUMIDITY_LOW 60

//declare buzzer parameters
int notes[] = {262, 294, 330, 349, 392, 440, 494, 523}; //C,D,E,F,G,A,B,C_H
int n_notes = sizeof(notes) / sizeof(notes[0]);

//declare pins
#define DHT_PIN 12
#define LED1_PIN 15
#define BUZZER_PIN 5
#define PB_cancel 34
#define PB_ok 18
#define PB_up 19
#define PB_down 33
#define LDR_PIN 35
#define SERVO_PIN 13

//declare global variables

//###1 related with local time
const char* NTP_SERVER  = "pool.ntp.org";
int UTC_OFFSET = 19800; // Offset for UTC+5:30 in seconds
String utc_offsets[] = {"UTC-12:00", "UTC-11:00", "UTC-10:30", "UTC-10:00", "UTC-09:30", "UTC-09:00", "UTC-08:30", "UTC-08:00", "UTC-07:00", "UTC-06:00", "UTC-05:00", "UTC-04:30", "UTC-04:00", "UTC-03:30", "UTC-03:00", "UTC-02:30", "UTC-02:00", "UTC-01:00", "UTC-00:44", "UTC-00:25:21", "UTC+00:00", "UTC+00:20", "UTC+00:30", "UTC+01:00", "UTC+01:24", "UTC+01:30", "UTC+02:00", "UTC+02:30", "UTC+03:00", "UTC+03:30", "UTC+04:00", "UTC+04:30", "UTC+04:51", "UTC+05:00", "UTC+05:30", "UTC+05:40", "UTC+05:45", "UTC+06:00", "UTC+06:30", "UTC+07:00", "UTC+07:20", "UTC+07:30", "UTC+08:00", "UTC+08:30", "UTC+08:45", "UTC+09:00", "UTC+09:30", "UTC+09:45", "UTC+10:00", "UTC+10:30", "UTC+11:00", "UTC+11:30", "UTC+12:00", "UTC+12:45", "UTC+13:00", "UTC+13:45", "UTC+14:00"};
int num_utc_offsets = sizeof(utc_offsets) / sizeof(utc_offsets[0]);

//###2 related with alarms
int days = 0, hours = 0, minutes = 0, seconds = 0;
bool alarm_enabled = false;
int num_alarms = 2;
int alarm_hours[] = {0, 0, };
int alarm_minutes[] = {0, 0, };
bool alarm_triggered[] = {false, false};

//###3 related with menu
int current_mode = 0; //store current options in our menue
int max_modes = 5;
String modes[] = {"1 -  Set  Time Zone", "2 -  Set  Alarm 1", "3 -  Set  Alarm 2", "4 -  View Alarms", "5 - Delete an Alarm"};

//###4 related with node red dashboard communication
char tempArr[6];
char humidArr[6];
char ldrArr[6];
char avgLightIntensityArr[6];

//###5 related with servo and LDR calculation
int theta_offset = 30;
float controllingFactor = 0.75;
float LDR_MAX = 4064.00; // calibrated LDR analog max value
float LDR_MIN = 32.00;  // calibrated LDR analog min value
float Tmed = 30.0; // ideal storage temperature in Celsius

//###6 Light intensity monitoring variables
unsigned long lastSampleTime = 0;
unsigned long lastSendTime = 0;
int samplingInterval = 5000;    // Default 5 seconds (in milliseconds)
int sendingInterval = 120000;   // Default 2 minutes (in milliseconds)
float lightReadings[24];        // Array to store readings (24 is enough for 2 minutes at 5-second intervals)
int readingIndex = 0;
int totalReadings = 0;

//declare objects
Adafruit_SSD1306 display(screen_width, screen_height, &Wire, OLED_reset);
DHTesp dhtSensor;
WiFiUDP ntpUDP;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET); // make timeClient object
Servo servo;


//  ##### DECLARE GLOBAL FUNCTIONS #####

//###1
void print_line(String text, String displayClearStatus = "n", int text_size = 1, int column = 0, int row = 0)
{
  if (displayClearStatus == "y") //n-do not clear display y-clear display
  {
    display.clearDisplay();
  }
  display.setTextSize(text_size); //text_size = 1 --> Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); //draw in colour text
  display.setCursor(column, row); //where printing start,  row 0-7 , column 0-127
  display.println(text);
  display.display();
}

//###2
void connect_wifi()
{
  WiFi.begin(ssid, password, wifi_channel); //connect to Wi-Fi

  unsigned long startTime = millis(); // Save the start time
  unsigned long timeout = 10000; // Set timeout duration (60 seconds)

  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - startTime > timeout)
    {
      print_line("Connection failed", "y", 1, 5, 20);
      print_line("trying reconnect...!", "n", 1, 5, 35);

      delay(5000); // wait 5 sec

      WiFi.begin(ssid, password, wifi_channel);

      startTime = millis(); // Reset the start time
    }

    print_line("Connecting to WIFI", "y", 1, 0, 5);
    delay(250);
    print_line("Connecting to WIFI.", "y", 1, 0, 5);
    delay(250);
    print_line("Connecting to WIFI..", "y", 1, 0, 5);
    delay(250);
    print_line("Connecting to WIFI...", "y", 1, 0, 5);
    delay(250);
  }

  print_line("Connected...!!!", "n", 1, 0, 20);
  delay(2000);
}

//###3
void sync_NTP_time()
{
  timeClient.update(); // Try to update the time
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();
}

//###4
void print_time_now()
{
  sync_NTP_time();
  print_line("Time: " + String(hours) + ":" + String(minutes) + ":" + String(seconds), "y", 1, 25, 30); //###1
}

//###5
void myTone(int pin, int frequency)
{
  ledcAttachPin(pin, 0);        // pin, channel
  ledcWriteTone(0, frequency);  // channel, frequency
}

//###6
void myNoTone(int pin)
{
  ledcDetachPin(pin);
}

//###7
void connectToBroker()
{
  while (!mqttClient.connected())
  {
    Serial.println("Attempting MQTT connection... ");
    if (mqttClient.connect("ESP32Client-465456464S645"))
    {
      Serial.println("Connected");
      mqttClient.subscribe("220559R-nodeRed-min_angle"); //subscribe
      mqttClient.subscribe("220559R-nodeRed-controlling_factor");
      mqttClient.subscribe("220559R-nodeRed-medication_preset");
      // subscriptions for light monitoring and slider controls
      mqttClient.subscribe("220559R-nodeRed-sampling_interval");
      mqttClient.subscribe("220559R-nodeRed-sending_interval"); 
      mqttClient.subscribe("220559R-nodeRed-ideal_temp");
    }

    else
    {
      Serial.println("Failed to connect, rc=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

//###8 function to normalize light value between 0 and 1
float normalizeLightValue(int rawValue) {
  return constrain((rawValue - LDR_MIN) / (LDR_MAX - LDR_MIN), 0.0, 1.0);
}

//###9 function of sample light intensity
void sampleLightIntensity() {
  unsigned long currentTime = millis();
  
  // Check if it's time to take a sample
  if (currentTime - lastSampleTime >= samplingInterval) {
    lastSampleTime = currentTime;
    
    // Read raw value from LDR
    int rawLight = analogRead(LDR_PIN);
    
    // Calculate average light intensity (normalized between 0-1)
    float normalizedLight = normalizeLightValue(rawLight);
    
    // Store the reading
    lightReadings[readingIndex] = normalizedLight;
    readingIndex = (readingIndex + 1) % 24;  // Cycle through array
    if (totalReadings < 24) {
      totalReadings++;
    }
    
    // Convert to char arrays for MQTT
    String(normalizedLight, 2).toCharArray(ldrArr, 6);
    
    // Publish current readings
    mqttClient.publish("220559R-medibox-light_intensity", ldrArr);
    
    Serial.print("Light sample taken: ");
    Serial.print(normalizedLight);
  }
  
  // Check if it's time to send the average
  if (currentTime - lastSendTime >= sendingInterval && totalReadings > 0) {
    lastSendTime = currentTime;
    
    // Calculate average of stored readings
    float sum = 0;
    for (int i = 0; i < totalReadings; i++) {
      sum += lightReadings[i];
    }
    float average = sum / totalReadings;
    
    // Convert to char array for MQTT
    String(average, 2).toCharArray(avgLightIntensityArr, 6);
    
    // Publish the average
    mqttClient.publish("220559R-medibox-avg_light_intensity", avgLightIntensityArr);
    
    Serial.print("Average light intensity sent: ");
    Serial.println(average);
    
    // Reset for next averaging period
    readingIndex = 0;
    totalReadings = 0;
  }
}

//###10
void updateLight()
{
  int rawLight = analogRead(LDR_PIN);  // Read raw LDR sensor values

  // Normalize light values to 0-1 range (with calibration)
  float normalizedLight = (rawLight - LDR_MAX) / (LDR_MIN - LDR_MAX);

  // Calculate servo angle based on light conditions
  // Get current temperature
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temperature = data.temperature;

  // Calculate servo angle based on the enhanced formula
  // θ = θoffset + (180 - θoffset) × I × γ × ln(ts/tu) × (T/Tmed)
  float tsToTuRatio = (float)samplingInterval / sendingInterval;
  float tempFactor = temperature / Tmed;
  
  // Apply natural logarithm with safety check for negative values
  float logFactor = 1.0; // default if ratio is invalid
  if (tsToTuRatio > 0) {
    logFactor = log(tsToTuRatio);
    // Constrain log factor to reasonable values
    logFactor = constrain(logFactor, -2.0, 2.0);
  }
  
  int servoAngle = theta_offset + (180 - theta_offset) * normalizedLight * controllingFactor * logFactor * tempFactor;
  
  // Ensure angle is within valid range
  servoAngle = constrain(servoAngle, 0, 180);
  
  Serial.print("Servo angle: ");
  Serial.println(servoAngle);
  servo.write(servoAngle);

  // publish MQTT ldr messages
  String(normalizedLight, 2).toCharArray(ldrArr, 6);
  mqttClient.publish("2220559R-medibox-light_intensity", ldrArr );
}

//###11
void ring_alarm()
{
  print_line("MEDICINE", "y", 2, 7, 10);
  print_line("TIME", "n", 2, 9, 35);

  bool break_happened = false;

  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  while (break_happened == false && digitalRead(PB_cancel) == HIGH)
  {
    //ring the buzzer
    for (int i = 0; i < n_notes; i++)
    {
      if (digitalRead(PB_cancel) == LOW)
      {
        break_happened = true;
        delay(200);
        break;
      }
      myTone(BUZZER_PIN, notes[i]); //###4
      digitalWrite(LED1_PIN, HIGH);
      delay(200);
      myNoTone(BUZZER_PIN);//###5
      digitalWrite(LED1_PIN, LOW);
      delay(10);

      data = dhtSensor.getTempAndHumidity();
      String(data.humidity, 1).toCharArray(humidArr, 6);
      String(data.temperature, 1).toCharArray(tempArr, 6);
      if (!mqttClient.connected())
      {
        Serial.println("Reconnecting to MQTT Broker");
        connectToBroker();
      }
      mqttClient.loop();
      mqttClient.publish("220559R-medibox-humidity", humidArr);
      mqttClient.publish("220559R-medibox-temperature", tempArr);
      updateLight();
    }
  }
  digitalWrite(LED1_PIN, LOW);
  myNoTone(BUZZER_PIN);//###5
}

//###12
void show_warning(float value, float threshold_low, float threshold_high, int print_row, String DisplayStatus, String message)
{
  if (value < threshold_low)
  {
    message = message + "LOW";
    print_line(message, DisplayStatus, 1, 10, print_row); //###1
  }

  if (value > threshold_high)
  {
    message = message + "HIGH";
    print_line(message, DisplayStatus, 1, 10, print_row); //###1
  }
}

//###13
void check_temp_and_humidity()
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.humidity, 1).toCharArray(humidArr, 6);
  String(data.temperature, 1).toCharArray(tempArr, 6);
  mqttClient.publish("220559R-medibox-humidity", humidArr);
  mqttClient.publish("220559R-medibox-temperature", tempArr);

  int n = 0;

  while (data.temperature < TEMP_LOW || data.temperature > TEMP_HIGH || data.humidity < HUMIDITY_LOW || data.humidity > HUMIDITY_HIGH )
  {
    show_warning(data.temperature, TEMP_LOW, TEMP_HIGH, 10, "y", "TEMPERATURE "); //###7
    show_warning(data.humidity, HUMIDITY_LOW, HUMIDITY_HIGH, 30, "y", "HUMIDITY "); //###7

    n++;

    if (n % 2 == 0)
    {
      digitalWrite(LED1_PIN, HIGH);
      myTone(BUZZER_PIN, notes[0]);
      delay(500);
    }

    else
    {
      digitalWrite(LED1_PIN, LOW);
      myNoTone(BUZZER_PIN);
      delay(500);
    }

    data = dhtSensor.getTempAndHumidity();
    String(data.humidity, 1).toCharArray(humidArr, 6);
    String(data.temperature, 1).toCharArray(tempArr, 6);
    if (!mqttClient.connected())
    {
      Serial.println("Reconnecting to MQTT Broker");
      connectToBroker();
    }
    mqttClient.loop();
    mqttClient.publish("220559R-medibox-humidity", humidArr);
    mqttClient.publish("220559R-medibox-temperature", tempArr);
    sampleLightIntensity();
  }

  digitalWrite(LED1_PIN, LOW);
  myNoTone(BUZZER_PIN);
}

//###14
void update_time_with_check_alarm_and_check_warning(){
  print_time_now(); //###3

  if (alarm_enabled == true)
  {
    for (int i = 0; i < num_alarms; i++)
    {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes)
      {
        ring_alarm(); //###6
        alarm_triggered[i] = true;
      }
    }
  }
  check_temp_and_humidity();//###8
}

//###15
int wait_for_button_press()
{
  int buttons[] = {PB_up, PB_down, PB_ok, PB_cancel};
  int numButtons = sizeof(buttons) / sizeof(buttons[0]);

  while (true)
  {
    for (int i = 0; i < numButtons; i++)
    {
      if (digitalRead(buttons[i]) == LOW)
      {
        delay(200);
        return buttons[i];
      }
    }
  }
}

//###16
void view_alarms(){

}

//###17
void delete_alarm(){

}

//###18
void set_time_unit(int &unit, int max_value, String message)
{

  int temp_unit = unit;

  while (true)
  {
    print_line(message + String(temp_unit), "y");

    int pressed = wait_for_button_press();//###10

    delay(200);

    switch (pressed)
    {
      case PB_up:
        temp_unit = (temp_unit + 1) % max_value;
        break;
      case PB_down:
        temp_unit = (temp_unit - 1 + max_value) % max_value;
        break;
      case PB_ok:
        unit = temp_unit;
        return;
      case PB_cancel:
        return;
    }
  }
}

//###19
void set_alarm(int alarm)
{
  set_time_unit(alarm_hours[alarm], 24, "Enter hour: ");//###11
  set_time_unit(alarm_minutes[alarm], 60, "Enter minute: ");//###11

  print_line("Alarm is set", "y");//###1

  alarm_enabled = true;

  delay(1000);
}

//###20
void disable_alarm()
{
  alarm_enabled = false;
  print_line("Alarms disabled", "y");//###1
  delay(1000);
}

//###21
int extract_utc_offset_value(int index)
{
  int offset = 0; // Parse UTC_OFFSET to calculate offset in seconds
  int hoursOffset = 0, minutesOffset = 0, secondsOffset = 0;

  sscanf(utc_offsets[index].c_str(), "%*c%*c%*c%d:%d:%d", &hoursOffset, &minutesOffset, &secondsOffset);// Extract hours, minutes, and seconds from UTC_OFFSET
  offset = hoursOffset * 3600 + minutesOffset * 60 + secondsOffset;// Calculate total offset in seconds

  // Handle negative offsets
  if (utc_offsets[index].startsWith("UTC-"))
  {
    offset = -offset;
  }
  return offset;
}

//###22
void set_time_zone()
{
  int index = 34; //index of default utc_offset
  int index_max = num_utc_offsets; //maximum index of array

  while (true)
  {
    print_line("Enter UTC offset  ", "y");//###1
    print_line(utc_offsets[index], "n", 1, 0, 15); //###1

    int pressed = wait_for_button_press(); //###10
    delay(200);

    if (pressed == PB_up)
      index = (index + 1) % index_max;

    else if (pressed == PB_down)
      index = (index - 1 + index_max) % index_max;

    else if (pressed == PB_ok)
    {
      UTC_OFFSET = extract_utc_offset_value(index);
      NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET);
      timeClient.begin();// Start the NTP client
      delay(500);
      print_line("Time zone is set", "y", 1, 10, 30); //###1
      delay(1000);
      break;

    }
    else if (pressed == PB_cancel)
      break;
  }
}

//###23
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

//###24
void go_to_menu()
{
  while (digitalRead(PB_cancel) == HIGH)
  {
    print_line(modes[current_mode], "y", 1, 0, 30); //###1

    int pressed = wait_for_button_press();//###10
    delay(200);
  }
}

//###25
void receiveCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String preset;
  char payloadCharAr[length];

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
    preset += (char)payload[i];
  }

  Serial.println();

  int temp_theta_offset = 0;
  float temp_controllingFactor = 0;
  float temp_Tmed = 0;
  int temp_samplingInterval = 0;
  int temp_sendingInterval = 0;

  if (strcmp(topic, "220559R-nodeRed-min_angle") == 0)
  {
    temp_theta_offset = String(payloadCharAr).toInt();
    if (temp_theta_offset >= 0 && temp_theta_offset <= 120) {
      theta_offset = temp_theta_offset;
      Serial.print("New theta_offset: ");
      Serial.println(theta_offset);
    }
  }
  else if (strcmp(topic, "220559R-nodeRed-controlling_factor") == 0)
  {
    temp_controllingFactor = String(payloadCharAr).toFloat();
    if (temp_controllingFactor >= 0 && temp_controllingFactor <= 1) {
      controllingFactor = temp_controllingFactor;
      Serial.print("New controllingFactor: ");
      Serial.println(controllingFactor);
    }
  }
  else if (strcmp(topic, "220559R-nodeRed-ideal_temp") == 0)
  {
    temp_Tmed = String(payloadCharAr).toFloat();
    if (temp_Tmed >= 10 && temp_Tmed <= 40) {
      Tmed = temp_Tmed;
      Serial.print("New Tmed: ");
      Serial.println(Tmed);
    }
  }
  else if (strcmp(topic, "220559R-nodeRed-sampling_interval") == 0)
  {
    temp_samplingInterval = String(payloadCharAr).toInt();
    if (temp_samplingInterval > 0) {
      samplingInterval = temp_samplingInterval * 1000; // Convert seconds to milliseconds
      Serial.print("New samplingInterval: ");
      Serial.println(samplingInterval);
    }
  }
  else if (strcmp(topic, "220559R-nodeRed-sending_interval") == 0)
  {
    temp_sendingInterval = String(payloadCharAr).toInt();
    if (temp_sendingInterval > 0) {
      sendingInterval = temp_sendingInterval * 1000; // Convert seconds to milliseconds
      Serial.print("New sendingInterval: ");
      Serial.println(sendingInterval);
    }
  }
  else if (strcmp(topic, "220559R-nodeRed-medication_preset") == 0)
  {
    Serial.println(preset);

    if (preset == "custom")
    {
      // Already handled by the other topics - this is just a flag
      Serial.println("Using custom settings");
    }
    else
    {
      // Apply preset values based on the selected medication
      if (preset == "tablet_a")
      {
        theta_offset = 45;
        controllingFactor = 0.6;
        Tmed = 28;
      }
      else if (preset == "tablet_b")
      {
        theta_offset = 60;
        controllingFactor = 0.8;
        Tmed = 30;
      }
      else if (preset == "tablet_c")
      {
        theta_offset = 35;
        controllingFactor = 0.5;
        Tmed = 25;
      }
      
      Serial.println("Applied preset values:");
      Serial.print("theta_offset: ");
      Serial.println(theta_offset);
      Serial.print("controllingFactor: ");
      Serial.println(controllingFactor);
      Serial.print("Tmed: ");
      Serial.println(Tmed);
    }
  }
}

//###26
void setupMqtt()
{
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}



// ######### MAIN CODE ###################

void setup()
{
  //initialize pins mode
  pinMode(DHT_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PB_cancel, INPUT);
  pinMode(PB_ok, INPUT);
  pinMode(PB_up, INPUT);
  pinMode(PB_down, INPUT);
  pinMode(LDR_PIN, INPUT);

  servo.attach(SERVO_PIN); // Extended pulse range

  Serial.begin(115200);

  //ledcSetup() function call before any other LEDC functions
  ledcSetup(0, 2000, 8);  // channel, frequency, resolution

  // Initialize the DHT22 sensor
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  //SSD1306_SWITCHAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, screen_address))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); //don't proceed loop forever
  }

  //show initial display buffer contents on the screen
  //the library initiallizes this with an adafruit splash screen
  display.display();
  delay(1000); //pause for 1 sec

  //clear the buffer
  display.clearDisplay();

  connect_wifi(); //###20

  print_line("Syncing NTP time...", "y", 1, 10, 20);

  timeClient.begin();// Start the NTP client
  delay(500);

  // Wait for the NTP client to get the first valid time
  sync_NTP_time(); //###2
  delay(500);

  print_line("Time config synced", "n", 1, 6, 35);
  delay(1000);
  display.clearDisplay();
  delay(2000);

  //show welcome message
  print_line("Welcome to Medibox!", "y", 1, 10, 30);
  delay(2000);

  //start mqtt connection
  setupMqtt();
}

void loop()
{
  if (!mqttClient.connected())
  {
    Serial.println("Reconnecting to MQTT Broker");
    connectToBroker(); //###7
  }

  mqttClient.loop(); //enter mqtt

  update_time_with_check_alarm_and_check_warning(); //###14

  updateLight(); //###10

  if (digitalRead(PB_ok) == LOW)
  {
    delay(200);
    go_to_menu(); //###24
  }
}
