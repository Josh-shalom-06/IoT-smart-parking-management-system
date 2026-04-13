# 🚗 IoT-Enabled Smart Parking Management System

An intelligent IoT-based parking solution designed to optimize parking space utilization using **ESP32, RFID, ultrasonic sensors, Django, Flutter, and Firebase**.

The system provides **real-time slot monitoring, automated entry/exit control, and smart reservation management** through both web and mobile applications.

---

## 📌 Features
- 🔍 Real-time parking slot occupancy detection
- 📶 ESP32-based sensor monitoring
- 🪪 RFID-based secure vehicle authentication
- 🚧 Automated entry and exit gate control
- 📱 Flutter mobile application for users
- 🌐 Django web dashboard for admin monitoring
- ☁️ Firebase Realtime Database integration
- 📍 Slot reservation and cancellation
- 📊 Admin dashboard with live parking updates

---

## 🛠️ Tech Stack

### Hardware
- ESP32
- Ultrasonic Sensors
- RFID Module
- Servo Motors
- IR Sensor
- I2C 16x2 LCD Display

### Software
- Python
- Django
- Flutter
- Firebase Realtime Database
- Arduino IDE
- VS Code

---

## 🏗️ Project Architecture
The system consists of three main modules:

1. **Hardware Layer (ESP32 and peripherals)**
   - Reads sensor data
   - RFID authentication
   - Controls gates
   - Updates slot status

2. **Backend Layer**
   - Django web app
   - Firebase database sync
   - Admin dashboard

3. **Frontend Layer**
   - Django web dashboard
   - Flutter mobile application
   - Login and reservation
   - Slot monitoring UI

---

## 📂 Project Structure

```text
IoT-smart-parking-management-system/
│
├── Django_Web_app/
├── ESP32_codes/
├── Flutter_Mobile_app/
├── Project_demonstration/
├── .gitignore
└── README.md
```
---
## 🚀 Setup and Run 

### 1. Clone the Repository
```bash
git clone https://github.com/Josh-shalom-06/IoT-smart-parking-management-system.git
cd IoT-smart-parking-management-system
```
---

### 2. ⚙️ Upload ESP32 Code
1. Open Arduino IDE
2. Connect ESP32 board
3. Open required .ino file from ESP32_codes/
4. Select correct COM port
5. Upload sketch

### 3. 🌐 Run Django Web Application
```bash
cd Django_Web_app
```
#### Create virtual environment
```bash
python -m venv env
```
#### Activate virtual environment
```bash
env\Scripts\activate
```
#### Install dependencies
```bash
pip install django firebase-admin pyrebase4
```
#### Run server
```bash
python manage.py runserver
```
### 4. 📱 Run Flutter Mobile App

```bash
cd Flutter_Mobile_app
```
#### Install dependencies
```bash
flutter pub get
```
#### Check connected devices
```bash
flutter devices
```
#### Run the app
```bash
flutter run
```
---

## 🎥 Project Demonstration
Demo video and project documentation are available in:

- `Project_demonstration/IoT_SPMS.mp4`
- `Project_demonstration/Project demonstration.docx`
