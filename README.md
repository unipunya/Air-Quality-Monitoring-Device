# **Air Quality Monitoring Device**

This Air Quality Monitoring Device is an IoT-based solution designed to monitor indoor air quality by tracking dust particles, temperature, oxygen levels, carbon monoxide (CO), and nitrogen dioxide (NO2). The device offers real-time data visualization and alerts through a web app, making it suitable for various indoor environments, including homes, offices, and public spaces.

## **Table of Contents**
- [**Introduction**](#introduction)
- [**Features**](#features)
- [**Components**](#components)
- [**System Architecture**](#system-architecture)
- [**Installation**](#installation)
- [**Usage**](#usage)
- [**Future Work**](#future-work)
- [**Contributors**](#contributors)

## **Introduction**
This Air Quality Monitoring Device aims to address the challenges associated with indoor air pollution by providing an automated, real-time monitoring system. Users can track air quality remotely through a cloud-based database and receive alerts when thresholds are exceeded, thus maintaining a healthy indoor environment.

## **Features**
- **Real-Time Monitoring**: Tracks air quality metrics in real-time and uploads data to a cloud database.
- **Threshold-Based Alerts**: Notifies users when air quality metrics exceed safe levels.
- **Remote Access**: View air quality data from any device through a web application.
- **Historical Data**: Allows users to track air quality trends over time.
- **User-Friendly Interface**: Provides an intuitive web-based dashboard for easy monitoring and visualization.

## **Components**
- **Raspberry Pi Pico W**: Microcontroller for sensor integration and data processing.
- **Sensors**:
  - **DS18B20**: Measures temperature.
  - **CJMCU-6814**: Monitors CO and NO2 levels.
  - **DSM501A**: Detects dust particle concentrations.
  - **AO-03**: Measures oxygen levels.
- **GSM Module (SIM800L)**: Enables data transmission in areas without Wi-Fi.
- **Firebase**: Cloud database for real-time data storage and retrieval.
- **Web Application**: Built using React to visualize and manage air quality data.

## **System Architecture**
The system is structured around a Raspberry Pi Pico W microcontroller that collects data from various sensors, processes it, and sends it to Firebase using Wi-Fi or GSM. A React-based web application retrieves data from Firebase, enabling remote access to air quality information.

## **Installation**
1. **Hardware Setup**:
   - Connect each sensor to the Raspberry Pi Pico W as outlined in the circuit diagrams.
   - Connect the GSM module and power management components.
2. **Firmware**:
   - Flash the provided MicroPython code onto the Raspberry Pi Pico W to enable data collection and transmission to Firebase.
3. **Web Application**:
   - Clone this repository.
   - Install dependencies:
     ```bash
     npm install
     ```
   - Start the application:
     ```bash
     npm start
     ```
4. **Firebase Setup**:
   - Set up a Firebase Realtime Database and configure it for secure data storage.
   - Update the Firebase configuration in the MicroPython and web application code.

## **Usage**
1. Power on the device and ensure connectivity via Wi-Fi or GSM.
2. Access the web application to monitor real-time data and view historical trends.
3. Set custom thresholds for alerts based on air quality parameters.

## **Future Work**
- **Enhanced Data Storage**: Implement long-term storage for historical analysis in Firebase.
- **Mobile App**: Develop a mobile app for easier access and push notifications.
- **Battery Optimization**: Improve power management to extend device runtime.

## **Contributors**
- A.M.A.D. Weerasinghe - Leader (Temperature Sensor, Database, Code Integration)
- H.D.U. Nipunya (Gas Sensor, 3D Design)
- W.D.N.A. Fernando (Oxygen Sensor, Web App, Battery Management)
- D.L.M.J. Liyanage (GSM Module, Battery Management, LCD Display)
- H.A.I.S. Leelarathna (Dust Sensor, PCB Design)

## **License**
This project is licensed under the MIT License.

