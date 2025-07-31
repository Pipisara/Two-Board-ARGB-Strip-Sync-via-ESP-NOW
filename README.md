# 🎇 Dual ESP32 ARGB LED Controller (Master–Receiver Sync)

Welcome to the **Dual ESP32 ARGB LED Controller** project! 🎉 This system controls two ESP32 boards that drive synchronized ARGB LED strips (e.g., WS2812B) with wireless communication via ESP-NOW and a web interface for color control.

## 🌟 Features

- **Wireless Synchronization:** 📡 Perfectly synced animations across two ESP32 boards using ESP-NOW
- **Web UI Control:** 🌐 Hosted web interface for HEX color input on the main ESP32
- **Continuous LED Treatment:** Treats both strips as one continuous LED strip
- **Special Receiver Animation:** Last 16 LEDs on receiver have unique behavior in all patterns
- **Automatic Animation Loop:** Cycles through patterns after 10 seconds of inactivity

## 🛠️ Components Used

- **2x ESP32 Development Boards**
- **2x WS2812B/WS2811 ARGB LED strips**
- **5V Power Supply (adequate current rating)**
- **Connecting wires**
- **Optional capacitors/resistors for LED stability**

## 🚀 Getting Started

To get started with this project, upload the respective firmware to your ESP32 boards and connect your LED strips.

### Installation

1. Set the LED counts (`mainLedCount`, `receiverLedCount`) in both sketches
2. Flash the main and receiver ESP32 firmware separately
3. Connect your device to the Wi-Fi network hosted by the main ESP32
4. Open the web UI and enter HEX color codes to control the LEDs

## 🤝 Contributing

Contributions are welcome! If you have suggestions or improvements, feel free to create a pull request.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📖 Introduction

The **Dual ESP32 ARGB LED Controller** is an advanced lighting control system that enables synchronized animations across two separate LED strips controlled by ESP32 microcontrollers. This project demonstrates wireless communication between microcontrollers and responsive web interface control for lighting systems.

### Project Overview

This system consists of two ESP32 boards working in a master-receiver configuration:
- **Main Board (ESP32-A):** Hosts a web interface for color input, controls its own LED strip, and sends commands to the receiver
- **Receiver Board (ESP32-B):** Controls a second LED strip and executes animations based on commands from the main board

The system features automatic pattern cycling with sophisticated synchronization, treating both physical LED strips as one continuous logical strip. Special attention is given to the last 16 LEDs of the receiver strip, which perform unique animations in all patterns.

### Objectives

- Create perfectly synchronized LED animations across two separate controllers
- Implement wireless communication between ESP32 boards using ESP-NOW
- Develop a responsive web interface for color control
- Demonstrate advanced LED strip control techniques
- Provide a foundation for expandable, synchronized lighting systems


## 🔧 System Overview

This project consists of two ESP32 boards:

- **Main Board (ESP32-A):** Hosts the web UI, controls its own LED strip, and sends commands.
- **Receiver Board (ESP32-B):** Controls a second LED strip and executes animations as per received instructions.

These boards work together to display complex, synchronized animations on addressable ARGB LEDs. The system assumes one-way communication (ESP-NOW) from Main → Receiver.

## 🎯 Core Goals

- Ensure synchronized animations across both boards.
- Treat both LED strips as one continuous strip (e.g., 50 LEDs on Main + 50 LEDs on Receiver = 100 LEDs).
- Allow dynamic color updates via user input (color hex codes).
- No manual pattern selection by the user.
- Include configurable speed/duration variables.
- Preserve special animation on last 16 LEDs of Receiver in every pattern.

## 📦 Global Configuration Variables

To be defined at the top of both ESP32 sketches:

```cpp
// Color configuration
std::vector<String> animationColors = {
  "#0ccebc", "#d9000d", "#16d900", "#d93300"
};

std::vector<std::pair<String, String>> contrastColorPairs = {
  {"#00c4d9", "#00d929"},
  {"#d93300", "#0040ff"},
  {"#ec000e", "#ffffff"},
  {"#000000", "#16d900"}
};

// LED strip configuration
int mainLedCount = 50;
int receiverLedCount = 50; // Set this on Main as known value
int totalLedCount = mainLedCount + receiverLedCount;

// Animation timing variables
int flashSpeed = 50;              // ms delay for fast blink
int fadeDuration = 5000;          // Total fade out time in ms
int stepDelay = 20;               // Delay between each LED in sequential patterns
int colorHoldDuration = 2000;     // Time to hold each full-color frame
int fastColorShiftSpeed = 100;    // For fast red-white-blue loop
int fadeInOutDelay = 15;          // Delay per step in fade animations
int partStepDelay = 400;          // Delay between lighting each part
int staggerDelay = 300;           // Delay between segment steps in patterns

```

## 🌐 Socials:
[![Behance](https://img.shields.io/badge/Behance-1769ff?logo=behance&logoColor=white)](https://behance.net/pipisarchandra1) [![Facebook](https://img.shields.io/badge/Facebook-%231877F2.svg?logo=Facebook&logoColor=white)](https://facebook.com/pipisara.chandrabhanu) [![LinkedIn](https://img.shields.io/badge/LinkedIn-%230077B5.svg?logo=linkedin&logoColor=white)](https://linkedin.com/in/pipisara) 

# 💻 Tech Stack:
![Apache](https://img.shields.io/badge/apache-%23D42029.svg?style=for-the-badge&logo=apache&logoColor=white) ![Nginx](https://img.shields.io/badge/nginx-%23009639.svg?style=for-the-badge&logo=nginx&logoColor=white) ![MicrosoftSQLServer](https://img.shields.io/badge/Microsoft%20SQL%20Server-CC2927?style=for-the-badge&logo=microsoft%20sql%20server&logoColor=white) ![MySQL](https://img.shields.io/badge/mysql-4479A1.svg?style=for-the-badge&logo=mysql&logoColor=white) ![Adobe](https://img.shields.io/badge/adobe-%23FF0000.svg?style=for-the-badge&logo=adobe&logoColor=white) ![Adobe After Effects](https://img.shields.io/badge/Adobe%20After%20Effects-9999FF.svg?style=for-the-badge&logo=Adobe%20After%20Effects&logoColor=white) ![Adobe Dreamweaver](https://img.shields.io/badge/Adobe%20Dreamweaver-FF61F6.svg?style=for-the-badge&logo=Adobe%20Dreamweaver&logoColor=white) ![Adobe Illustrator](https://img.shields.io/badge/adobe%20illustrator-%23FF9A00.svg?style=for-the-badge&logo=adobe%20illustrator&logoColor=white) ![Adobe Lightroom](https://img.shields.io/badge/Adobe%20Lightroom-31A8FF.svg?style=for-the-badge&logo=Adobe%20Lightroom&logoColor=white) ![Adobe Photoshop](https://img.shields.io/badge/adobe%20photoshop-%2331A8FF.svg?style=for-the-badge&logo=adobe%20photoshop&logoColor=white) ![Adobe Premiere Pro](https://img.shields.io/badge/Adobe%20Premiere%20Pro-9999FF.svg?style=for-the-badge&logo=Adobe%20Premiere%20Pro&logoColor=white) ![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white) ![CSS3](https://img.shields.io/badge/css3-%231572B6.svg?style=for-the-badge&logo=css3&logoColor=white) ![HTML5](https://img.shields.io/badge/html5-%23E34F26.svg?style=for-the-badge&logo=html5&logoColor=white) ![JavaScript](https://img.shields.io/badge/javascript-%23323330.svg?style=for-the-badge&logo=javascript&logoColor=%23F7DF1E) ![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54) ![PHP](https://img.shields.io/badge/php-%23777BB4.svg?style=for-the-badge&logo=php&logoColor=white) ![DigitalOcean](https://img.shields.io/badge/DigitalOcean-%230167ff.svg?style=for-the-badge&logo=digitalOcean&logoColor=white) ![AWS](https://img.shields.io/badge/AWS-%23FF9900.svg?style=for-the-badge&logo=amazon-aws&logoColor=white) ![Azure](https://img.shields.io/badge/azure-%230072C6.svg?style=for-the-badge&logo=microsoftazure&logoColor=white) ![WordPress](https://img.shields.io/badge/WordPress-%23117AC9.svg?style=for-the-badge&logo=WordPress&logoColor=white) ![Cisco](https://img.shields.io/badge/cisco-%23049fd9.svg?style=for-the-badge&logo=cisco&logoColor=black)
# 📊 GitHub Stats:
![](https://github-readme-stats.vercel.app/api?username=Pipisara&theme=dark&hide_border=false&include_all_commits=false&count_private=false)<br/>
![](https://github-readme-streak-stats.herokuapp.com/?user=Pipisara&theme=dark&hide_border=false)<br/>
![](https://github-readme-stats.vercel.app/api/top-langs/?username=Pipisara&theme=dark&hide_border=false&include_all_commits=false&count_private=false&layout=compact)

### ✍️ Random Dev Quote
![](https://quotes-github-readme.vercel.app/api?type=horizontal&theme=radical)

---
[![](https://visitcount.itsvg.in/api?id=Pipisara&icon=0&color=0)](https://visitcount.itsvg.in)


