# 🔐 Hardware Password Wallet & Auto-Login Tool (ESP32)

An advanced, hardware-based password manager and auto-login device built on top of the **ESP32** microcontroller. This project secures your credentials locally using a **Morse Code Touch Lock**, stores them in JSON format on the flash memory via **LittleFS**, and allows credential management through an interactive local web server with full CRUD capabilities (Add, Show, Edit, Delete).

---

## 📺 Project in Action (Demo)

Here is a quick video demonstrating the Morse code unlock, LCD menu navigation, and the dynamic configuration web interface:

[![Watch the video](https://img.youtube.com/vi/AeZRYYUWhIE/maxresdefault.jpg)](https://www.youtube.com/watch?v=AeZRYYUWhIE)

---

## ✨ Features

* **Morse Code Authentication:** Secured by a capacitive touch pins unlock sequence (`touchRead`) acting as a hardware PIN.
* **Dual Boot Modes:**
  * **Normal Mode:** Functions as a Bluetooth HID Keyboard (`BleKeyboard`) to safely inject usernames, passwords, or perform full **Auto-Login** actions directly into your PC.
  * **Config Mode:** Spins up a Wi-Fi Access Point (`ESP32 PasswordWallet`) with a Captive DNS Portal to manage data through a custom web portal.
* **Full Web CRUD Interface:** Modern web UI hosted on the ESP32 to View, Create, Edit, and Delete accounts dynamically through raw JSON API endpoints.
* **Local Flash Storage:** Parses and manages records directly on the onboard flash memory inside a `wallet.json` file utilizing **LittleFS** and **ArduinoJson**.
* **Intuitive Hardware Interface:** Features a `16x2 I2C LCD` display with a two-button structural navigation menu (SEND and SWITCH) for selecting platforms, specific accounts, and discrete actions.

---

## 🛠️ Hardware Components

* **Microcontroller:** ESP32 Development Board.
* **Display:** 16x2 LCD with I2C Backboard Interface.
* **Input Mechanisms:**
  * 2x Tactile Push Buttons (SEND on GPIO 18, SWITCH on GPIO 16).
  * 1x Capacitive Touch Sensor connection (GPIO 4).

---

## 🚀 How It Works & Navigation

1. **Unlock Device:** Upon powering up, the device enters a `LOCKED` state. Input your predefined 8-bit Morse Code sequences (dots/dashes) via the touch sensor to unlock.
2. **Standard HID Mode:**
   * **SWITCH Button:** Cycles through unique Platforms, Accounts, or Actions (Send Username / Send Password).
   * **SEND Button (Short Press):** Types out the highlighted credential via Bluetooth.
   * **SEND Button (Long Press - 2s):** Automatically types the username, presses `TAB`, injects the password, and hits `ENTER` for instant hands-free login.
3. **Web Config Mode:** Hold down the **SWITCH** button while passing the Morse code validation stage. Connect to the `ESP32 PasswordWallet` Wi-Fi access point, navigate to `192.168.4.1`, and manage your wallet records through your browser.

---

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
