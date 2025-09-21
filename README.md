# 🧠💊 Smart Medibox Project

**Author:** Nuwandi Samarawickrama   

## Project Overview

The **Medibox project** is developed as part of **EN2853: Embedded Systems and Applications** to help users manage their medication schedules and maintain safe storage conditions for sensitive medicines. This project is implemented using an **ESP32**, **sensors**, **OLED display**, and a **Node-RED dashboard**.  

The project is divided into **two main parts**:

---

### part 1: Basic Medibox Simulation

**Objective:** Create a working simulation of a Medibox that reminds users to take their medicine.  

**Features:**
- **Set Time Zone:** Configure the local time zone by providing the UTC offset.  
- **Manage Alarms:** Set, view, and delete up to 2 alarms.  
- **Display Time:** Fetch current time from an NTP server over Wi-Fi and show it on an OLED.  
- **Alarm Indication:** Ring alarms using a buzzer, LED, or OLED message.  
- **Alarm Control:** Stop or snooze the alarm (snooze duration: 5 minutes) using a push button.  
- **Environment Monitoring:** Monitor temperature and humidity inside the storage area.  
  - Healthy temperature: 24°C ≤ T ≤ 32°C  
  - Healthy humidity: 65% ≤ H ≤ 80%  
  - Warnings are displayed on the OLED or indicated with LED/buzzer if limits are exceeded.

---

### Part 2: Enhanced Medibox with Node-RED

**Objective:** Improve the Medibox with advanced monitoring and remote visualization.  

**Features:**
- **Light Intensity Monitoring:** Use an LDR sensor to measure light intensity at regular intervals.  
  - Sampling interval (ts) and data sending interval (tu) are configurable from the dashboard.  
  - Average light intensity is sent to Node-RED for visualization.  
- **Dashboard Visualization:**  
  - Numerical display for the latest average light intensity (range 0–1).  
  - Historical plot of average values over time.  
  - Sliders to adjust sampling interval, sending interval, minimum servo angle (θoffset), controlling factor (γ), and ideal storage temperature (Tmed).  
- **Shaded Sliding Window:** A servo motor adjusts a shaded sliding window to control light entering the Medibox.  
  - Servo angle is calculated based on the light intensity, temperature, and configurable parameters using the formula:  
    ```
    θ = θoffset + (180 − θoffset) × I × γ × ln(ts/tu) × (T/Tmed)
    ```
    - θoffset: minimum servo angle (default 30°)  
    - I: light intensity (0–1)  
    - γ: controlling factor (default 0.75)  
    - ts: sampling interval  
    - tu: sending interval  
    - T: measured temperature  
    - Tmed: ideal storage temperature (default 30°C)  
- **Temperature Monitoring:** DHT11 sensor measures ambient temperature inside the storage area to adjust the servo.  

---

### Hardware & Software Components
- **ESP32 microcontroller**  
- **OLED display** for time and warnings  
- **DHT11 sensor** for temperature & humidity  
- **LDR** for light intensity  
- **Servo motor** for shaded sliding window  
- **Buzzer / LEDs** for alarm indication  
- **Node-RED dashboard** for monitoring and parameter control  
- **MQTT broker:** `test.mosquitto.org` for communication  

---

### How the System Works
1. The ESP32 fetches current time via NTP and displays it on the OLED.  
2. Users can set alarms, which will ring using buzzer/LED/OLED messages.  
3. Temperature and humidity are continuously monitored, with warnings triggered when values exceed healthy limits.  
4. The LDR sensor measures light intensity and sends averaged readings to the Node-RED dashboard.  
5. The servo-controlled sliding window adjusts automatically to maintain optimal light conditions based on sensor readings and user-defined parameters.  

---

### Project Benefits
- **Medication Compliance:** Ensures users take medicine on time.  
- **Safe Storage:** Maintains temperature and light conditions suitable for sensitive medications.  
- **Remote Monitoring:** Users can check conditions and adjust parameters via Node-RED.  
- **Customizable:** Dashboard sliders allow adjustments to suit different medicines.  

---



### License
This project is released under the **MIT License**. See `LICENSE` for details.

---

### Notes
- Healthy temperature: 24°C–32°C  
- Healthy humidity: 65%–80%  
- Servo angle and light control follow the formulas in Assignment 2.  
- All sensors and parameters are fully configurable from the Node-RED dashboard.
