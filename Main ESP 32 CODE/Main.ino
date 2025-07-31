/*
 * ESP32 A (Main Board) - RGB LED Controller with ESP-NOW
 * Controls its own LED strip, serves web UI, and wirelessly controls ESP32 B
 * Updated with new block lighting animation
 */

#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// LED Configuration
#define LED_PIN     21        // Data pin for FastLED
#define LED_TYPE    WS2812B   // LED strip type
#define COLOR_ORDER RGB       // RGB order for the LEDs



CRGB savedUserColor = CRGB::Black;  // Stores the single user color
bool hasSavedColor = false;         // Flag to indicate if we have a saved color


// LED Counts
int NUM_LEDS_MAIN = 100;       // Example: 70 LEDs on main board
int NUM_LEDS_RECEIVER = 200;   // Example: 50 LEDs on receiver
int SPECIAL_START_INDEX = NUM_LEDS_RECEIVER - 16;

// Combined LED count and segment config
int TOTAL_LEDS = NUM_LEDS_MAIN + NUM_LEDS_RECEIVER;
int SEGMENT_DELAY = 800; // Adjustable segment lighting delay

// Configure your segments here (example for 120 LEDs: 70 main + 50 receiver)
const int SEGMENTS[6][2] = {
  {0, 49},     // Main board (LEDs 0-49)
  {50, 99},    // Main board (LEDs 50-99)
  {100, 149},  // Receiver board (LEDs 100-149)
  {150, 199},  // Receiver board (LEDs 150-199)
  {200, 283},  // Receiver board (LEDs 200-283)
  {284, 299}   // Receiver board (Special last 16 LEDs)
};

// Animation Timing
int ANIMATION_DURATION_MS = 10000;
int ANIMATION_TRANSITION_MS = 500;

// User Color Settings
int MANUAL_COLOR_DURATION_MS = 10000;
bool USER_COLOR_ACTIVE = false;
unsigned long USER_COLOR_START_TIME = 0;

// Animation Speed Controls
int SPEED_RANDOM_WHITE_BLINK_MS = 200;
int SPEED_FADE_STEPS = 50;
int SPEED_COLOR_WIPE_DELAY_MS = 50;
int SPEED_SPECIAL_16_DELAY_MS = 150;
int SPEED_CHASE_GROUP_DELAY_MS = 150;
int SPEED_FADE_INOUT_STEP_MS = 30;
int SPEED_BLOCK_LIGHTUP_DELAY_MS = 800;
int SPEED_PARTWISE_DELAY_MS = 600;
int SPEED_BREATHING_STEP_MS = 40;

// Colors
const char* COLOR_LIST[] = { "#0ccebc", "#d9000d", "#16d900", "#d93300" };
const char* COLOR_PAIR_LIST[][2] = { 
  {"#ec000e", "#00c4d9"}, 
  {"#00d929", "#d93300"}, 
  {"#ffffff", "#0000ff"} 
};
const char* RGB_CYCLE[] = { "#ff0000", "#ffffff", "#0000ff" };

// Unique color lists for specific animations
const char* COLOR_WIPE_COLORS[] = { "#FF00FF", "#00FFFF", "#FFFF00", "#FF007F" };
const char* SPECIAL_16_COLORS[][2] = {
  {"#FF00FF", "#00FF00"},
  {"#00FFFF", "#FF00FF"},
  {"#FFFF00", "#FF00FF"},
  {"#FF0000", "#00FF00"}
};
const char* FADE_COLORS[] = { "#FF007F", "#7F00FF", "#00FF7F", "#FF7F00" };
const char* SEQUENTIAL_COLORS[] = { "#FF0000", "#00FF00", "#0000FF", "#FF00FF" };

// Animation control
int currentAnimation = 0;
int totalAnimations = 9;
bool animationInitialized = false;

// Color indexes
int colorWipeColorIndex = 0;
int special16ColorIndex = 0;
int fadeColorIndex = 0;
int sequentialColorIndex = 0;
int colorIndex = 0;
int pairIndex = 0;


// Store current LED values
CRGB ledsMain[100];

// WiFi credentials
const char* ssid = "Lantern_Lights";
const char* password = "Lantern123";

// ESP-NOW communication
uint8_t receiverMacAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Replace with ESP32 B's MAC
esp_now_peer_info_t peerInfo;

// Web server
AsyncWebServer server(80);

// Message structure for ESP-NOW
typedef struct {
  uint8_t command;  // 0: Set color, 1-9: Animation patterns
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t specialR;
  uint8_t specialG;
  uint8_t specialB;
  uint16_t delayTime;
  uint8_t param1;
  uint8_t param2;
  uint8_t colorIndex; // Added for synchronization
} LEDCommand;

LEDCommand currentCommand;

// HTML for web interface (same as before)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <title>වල්පොල පොසොන් කලාපය</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: Arial; 
      text-align: center; 
      margin:0px auto; 
      padding: 20px; 
      background-color: #121212;
      color: #e0e0e0;
      display: flex;
      flex-direction: column;
      min-height: 100vh;
    }
    .container { 
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      max-width: 400px; 
      margin: 0 auto; 
      background-color: #1e1e1e; 
      padding: 20px; 
      border-radius: 10px; 
      box-shadow: 0 0 15px rgba(0,0,0,0.3);
      flex-grow: 1;
    }
    .color-picker-container { 
      margin-bottom: 10px; 
    }
    .picker-label {
      display: block;
      margin-bottom: 8px;
      color: #0ccebc;
      font-size: 14px;
    }
    input[type="color"] { 
      width: 150px; 
      height: 50px; 
      margin-bottom: 10px; 
      border: 2px solid #333; 
      border-radius: 5px; 
      background: #333;
      cursor: pointer;
    }
    input[type="text"] { 
      width: 150px; 
      height: 30px; 
      margin: 10px 0; 
      text-align: center; 
      border: 2px solid #333; 
      border-radius: 5px; 
      background: #333;
      color: #fff;
    }
    button { 
      background-color: #0ccebc; 
      color: white; 
      padding: 10px 20px; 
      border: none; 
      border-radius: 5px; 
      cursor: pointer; 
      margin-top: 10px;
      font-weight: bold;
      transition: all 0.3s;
    }
    button:hover {
      background-color: #0aa899;
      transform: scale(1.02);
    }
    .predefined-colors {
      display: grid;
      grid-template-columns: repeat(5, 1fr);
      gap: 10px;
      margin: 20px 0;
      width: 100%;
    }
    .color-box {
      height: 40px;
      border-radius: 5px;
      cursor: pointer;
      border: 2px solid #333;
      transition: all 0.2s;
    }
    .color-box:hover {
      transform: scale(1.05);
      box-shadow: 0 0 10px currentColor;
    }
    .color-input-group {
      margin: 20px 0;
    }
    h1 {
      color: #0ccebc;
      margin-bottom: 20px;
    }
    .status {
      margin-top: 15px;
      padding: 10px;
      border-radius: 5px;
      background-color: #252525;
      min-height: 20px;
      width: 100%;
      transition: all 0.3s;
    }
    .status.active {
      background-color: #0ccebc20;
      border: 1px solid #0ccebc;
    }
    .footer {
      margin-top: 20px;
      padding: 15px;
      color: #888;
      font-size: 12px;
      line-height: 1.5;
    }
    .footer a {
      color: #0ccebc;
      text-decoration: none;
    }
    .footer a:hover {
      text-decoration: underline;
    }
    .instructions {
      margin-top: 20px;
      padding: 15px;
      background-color: #252525;
      border-radius: 5px;
      width: 100%;
      text-align: left;
    }
    .instructions h3 {
      color: #0ccebc;
      margin-top: 0;
      margin-bottom: 10px;
    }
    .instructions p {
      margin: 5px 0;
      font-size: 13px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>පින්බර පොසොන් මංගල්‍යක් වේවා..</h2>
    <h1>පොසොන් පහන් කූඩුව 2025</h1>
  
    <div class="color-picker-container">
      <span class="picker-label">ඔබ කැමති වර්ණය තෝරන්න</span>
      <input type="color" id="colorPicker" onchange="updateAndSendColor(this.value)" value="#0ccebc">
    </div>
    
    <div class="predefined-colors">
      <div class="color-box" style="background-color: #0ccebc;" onclick="selectAndSendColor('#0ccebc')"></div>
      <div class="color-box" style="background-color: #d9000d;" onclick="selectAndSendColor('#d9000d')"></div>
      <div class="color-box" style="background-color: #16d900;" onclick="selectAndSendColor('#16d900')"></div>
      <div class="color-box" style="background-color: #d93300;" onclick="selectAndSendColor('#d93300')"></div>
      <div class="color-box" style="background-color: #FF00FF;" onclick="selectAndSendColor('#FF00FF')"></div>
      <div class="color-box" style="background-color: #00FFFF;" onclick="selectAndSendColor('#00FFFF')"></div>
      <div class="color-box" style="background-color: #FFFF00;" onclick="selectAndSendColor('#FFFF00')"></div>
      <div class="color-box" style="background-color: #FF00A5;" onclick="selectAndSendColor('#FF00A5')"></div>
      <div class="color-box" style="background-color: #00FFA5;" onclick="selectAndSendColor('#00FFA5')"></div>
      <div class="color-box" style="background-color: #A500FF;" onclick="selectAndSendColor('#A500FF')"></div>
    </div>
    
    <div class="color-input-group">
      <input type="text" id="hexInput" placeholder="#RRGGBB" maxlength="7" value="#0ccebc">
      <button onclick="sendColor()">හරි</button>
    </div>
    
    <div class="status" id="status">
      සූදානමින්...
    </div>
    
    <div class="instructions">
      <h3>භාවිතා කරන්නේ කෙසේද:</h3>
      <p>• වර්ණය වහාම යෙදීමට ඕනෑම වර්ණ කොටුවක් ක්ලික් කරන්න</p>
      <p>• ඔබ කැමති වර්ණ තෝරා ගැනීමට ඉහල වර්ණ තෝරකය භාවිතා කරන්න.</p>
      <p>• නැතහොත් අතින් HEX වර්ණ කේතයක් ඇතුළත් කරන්න.</p>
      <p>(උදා: #FF0000 - රතු )</p>
      <p>• "හරි " බොත්තම මගින් ඔබේ තේරීම තහවුරු කරන්න.</p>
    </div>
    
    <div class="instructions">
      <h3>How to Use:</h3>
      <p>• Click any color box to immediately apply that color</p>
      <p>• Use the color picker to select custom colors</p>
      <p>• Or manually enter a HEX color code (e.g. #FF00FF)</p>
      <p>• The "හරි" button confirms your manual selection</p>
    </div>
  </div>
  
  <div class="footer">
    වල්පොල පොසොන් කලාපය
  </div>
  
  <script>
    function updateHexInput(color) {
      document.getElementById('hexInput').value = color.toUpperCase();
    }
    
    function selectAndSendColor(color) {
      document.getElementById('hexInput').value = color;
      document.getElementById('colorPicker').value = color;
      sendColor(color);
    }
    
    function updateAndSendColor(color) {
      updateHexInput(color);
      sendColor(color);
    }
    
    function sendColor(color = null) {
      const colorToSend = color || document.getElementById('hexInput').value;
      
      if(!/^#[0-9A-F]{6}$/i.test(colorToSend)) {
        updateStatus('Invalid color format!', true);
        return;
      }
      
      updateStatus('Sending...');
      
      fetch('/setcolor', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          color: colorToSend
        })
      })
      .then(response => response.text())
      .then(data => {
        console.log(data);
        updateStatus('Color set: ' + colorToSend);
      })
      .catch((error) => {
        console.error('Error:', error);
        updateStatus('Error sending color!', true);
      });
    }
    
    function updateStatus(message, isError = false) {
      const statusElement = document.getElementById('status');
      statusElement.textContent = message;
      statusElement.classList.add('active');
      
      if(isError) {
        statusElement.style.color = '#ff4444';
        statusElement.style.borderColor = '#ff4444';
        statusElement.style.backgroundColor = '#ff444420';
      } else {
        statusElement.style.color = '#0ccebc';
        statusElement.style.borderColor = '#0ccebc';
        statusElement.style.backgroundColor = '#0ccebc20';
      }
      
      setTimeout(() => {
        statusElement.classList.remove('active');
      }, 3000);
    }
  </script>
</body>
</html>

)rawliteral";

void hexToRGB(const char* hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
  String hex = String(hexColor).substring(1);
  long number = strtol(hex.c_str(), NULL, 16);
  r = (number >> 16) & 0xFF;
  g = (number >> 8) & 0xFF;
  b = number & 0xFF;
}

void sendCommand() {
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &currentCommand, sizeof(currentCommand));
  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(r, g, b));
  FastLED.show();
  
  currentCommand.command = 0;
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.specialR = r;
  currentCommand.specialG = g;
  currentCommand.specialB = b;
  currentCommand.delayTime = SPEED_SPECIAL_16_DELAY_MS;
  sendCommand();

  USER_COLOR_ACTIVE = true;
  USER_COLOR_START_TIME = millis();
}

void setUserColor(const char* hexColor) {
  uint8_t r, g, b;
  hexToRGB(hexColor, r, g, b);
  savedUserColor = CRGB(r, g, b);
  hasSavedColor = true;
  
  // Immediately show the color
  setColor(r, g, b);
}

void randomWhiteFlash() {
  if (!animationInitialized) {
    Serial.println("Animation 1: Random White Flash");
    animationInitialized = true;
  }
  
  currentCommand.command = 1;
  currentCommand.r = 255;
  currentCommand.g = 255;
  currentCommand.b = 255;
  currentCommand.delayTime = SPEED_RANDOM_WHITE_BLINK_MS;
  currentCommand.colorIndex = 0;
  sendCommand();
  
  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
    
    for(int j = 0; j < NUM_LEDS_MAIN / 5; j++) {
      int pos = random(NUM_LEDS_MAIN);
      ledsMain[pos] = CRGB::White;
    }
    FastLED.show();
    delay(SPEED_RANDOM_WHITE_BLINK_MS);
  }
}

void fullWhiteFadeOut() {
  if (!animationInitialized) {
    Serial.println("Animation 2: Full White + Fade Out");
    animationInitialized = true;
  }
  
  currentCommand.command = 2;
  currentCommand.r = 255;
  currentCommand.g = 255;
  currentCommand.b = 255;
  currentCommand.delayTime = 5000;
  currentCommand.colorIndex = 0;
  sendCommand();
  
  unsigned long startTime = millis();
  bool fadedOut = false;
  
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    if (!fadedOut) {
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::White);
      FastLED.show();
      
      if (millis() - startTime > 5000) {
        for(int i = 255; i >= 0; i -= (255 / SPEED_FADE_STEPS)) {
          for(int j = 0; j < NUM_LEDS_MAIN; j++) {
            ledsMain[j].fadeToBlackBy(255 / SPEED_FADE_STEPS);
          }
          FastLED.show();
          
          currentCommand.command = 2;
          currentCommand.r = i;
          currentCommand.g = i;
          currentCommand.b = i;
          currentCommand.delayTime = 50;
          currentCommand.param1 = 1;
          sendCommand();
          
          delay(50);
        }
        fadedOut = true;
      }
    }
    delay(10);
  }
}

void colorWipeCombined() {
  if (!animationInitialized) {
    Serial.println("Animation 3: Color Wipe Combined");
    animationInitialized = true;
  }
  
  uint8_t r, g, b;
  hexToRGB(COLOR_WIPE_COLORS[colorWipeColorIndex], r, g, b);
  
  currentCommand.command = 3;
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.delayTime = SPEED_COLOR_WIPE_DELAY_MS;
  currentCommand.param1 = 0;
  currentCommand.colorIndex = colorWipeColorIndex;
  sendCommand();
  
  unsigned long startTime = millis();
  bool wipedMain = false;
  
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    if (!wipedMain) {
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
      FastLED.show();
      
      for(int i = 0; i < NUM_LEDS_MAIN; i++) {
        ledsMain[i] = CRGB(r, g, b);
        FastLED.show();
        delay(SPEED_COLOR_WIPE_DELAY_MS);
      }
      wipedMain = true;
      
      currentCommand.command = 3;
      currentCommand.r = r;
      currentCommand.g = g;
      currentCommand.b = b;
      currentCommand.delayTime = SPEED_COLOR_WIPE_DELAY_MS;
      currentCommand.param1 = 1;
      sendCommand();
    }
    delay(10);
  }
  
  // Move to next color for next time
  colorWipeColorIndex = (colorWipeColorIndex + 1) % (sizeof(COLOR_WIPE_COLORS) / sizeof(COLOR_WIPE_COLORS[0]));
}

void specialColorLast16() {
  if (!animationInitialized) {
    Serial.println("Animation 4: Special Last 16 LEDs");
    animationInitialized = true;
  }
  
  uint8_t mainR, mainG, mainB;
  uint8_t specialR, specialG, specialB;
  hexToRGB(SPECIAL_16_COLORS[special16ColorIndex][0], mainR, mainG, mainB);
  hexToRGB(SPECIAL_16_COLORS[special16ColorIndex][1], specialR, specialG, specialB);
  
  currentCommand.command = 4;
  currentCommand.r = mainR;
  currentCommand.g = mainG;
  currentCommand.b = mainB;
  currentCommand.specialR = specialR;
  currentCommand.specialG = specialG;
  currentCommand.specialB = specialB;
  currentCommand.delayTime = SPEED_SPECIAL_16_DELAY_MS;
  currentCommand.colorIndex = special16ColorIndex;
  sendCommand();
  
  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(mainR, mainG, mainB));
    FastLED.show();
    delay(100);
  }
  
  // Move to next color pair for next time
  special16ColorIndex = (special16ColorIndex + 1) % (sizeof(SPECIAL_16_COLORS) / sizeof(SPECIAL_16_COLORS[0]));
}

void redWhiteBlueChase() {
  if (!animationInitialized) {
    Serial.println("Animation 5: Red-White-Blue Chase");
    animationInitialized = true;
  }
  
  uint8_t rValues[3], gValues[3], bValues[3];
  for(int i = 0; i < 3; i++) {
    hexToRGB(RGB_CYCLE[i], rValues[i], gValues[i], bValues[i]);
  }
  
  currentCommand.command = 5;
  currentCommand.r = rValues[0];
  currentCommand.g = gValues[0];
  currentCommand.b = bValues[0];
  currentCommand.specialR = rValues[1];
  currentCommand.specialG = gValues[1];
  currentCommand.specialB = bValues[1];
  currentCommand.param1 = rValues[2];
  currentCommand.param2 = gValues[2];
  currentCommand.delayTime = SPEED_CHASE_GROUP_DELAY_MS;
  currentCommand.colorIndex = 0;
  sendCommand();
  
  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    for(int colorStep = 0; colorStep < 3; colorStep++) {
      for(int i = 0; i < NUM_LEDS_MAIN; i++) {
        int colorIndex = (i / 10 + colorStep) % 3;
        ledsMain[i] = CRGB(rValues[colorIndex], gValues[colorIndex], bValues[colorIndex]);
      }
      FastLED.show();
      delay(SPEED_CHASE_GROUP_DELAY_MS);
    }
  }
}

void colorFadeInOut() {
  if (!animationInitialized) {
    Serial.println("Animation 6: Color Fade In/Out");
    animationInitialized = true;
  }
  
  uint8_t r, g, b;
  hexToRGB(FADE_COLORS[fadeColorIndex], r, g, b);
  
  currentCommand.command = 6;
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.delayTime = SPEED_FADE_INOUT_STEP_MS;
  currentCommand.colorIndex = fadeColorIndex;
  sendCommand();
  
  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    for(int i = 0; i <= 255; i += 5) {
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(r * i / 255, g * i / 255, b * i / 255));
      FastLED.show();
      delay(SPEED_FADE_INOUT_STEP_MS);
    }
    
    delay(500);
    
    for(int i = 255; i >= 0; i -= 5) {
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(r * i / 255, g * i / 255, b * i / 255));
      FastLED.show();
      delay(SPEED_FADE_INOUT_STEP_MS);
    }
  }
  
  // Move to next color for next time
  fadeColorIndex = (fadeColorIndex + 1) % (sizeof(FADE_COLORS) / sizeof(FADE_COLORS[0]));
}

void blockLighting() {
  uint8_t r, g, b;
  hexToRGB(COLOR_LIST[colorIndex], r, g, b);
  
  currentCommand.command = 7;
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.delayTime = SPEED_BLOCK_LIGHTUP_DELAY_MS;
  
  for(int segment = 0; segment < 6; segment++) {
    int start = SEGMENTS[segment][0];
    int end = SEGMENTS[segment][1];
    
    // Check if segment belongs to main or receiver
    if(end < 100) { // Main board segment
      for(int i = start; i <= end; i++) {
        ledsMain[i] = CRGB(r, g, b);
      }
      FastLED.show();
    } else { // Receiver board segment
      currentCommand.param1 = segment + 1; // Segment number (1-6)
      sendCommand();
    }
    
    delay(SPEED_BLOCK_LIGHTUP_DELAY_MS);
  }
}

void sixPartSequential() {
  if (!animationInitialized) {
    Serial.println("Animation 8: 6-Part Sequential");
    animationInitialized = true;
  }
  
  uint8_t r, g, b;
  hexToRGB(SEQUENTIAL_COLORS[sequentialColorIndex], r, g, b);
  
  currentCommand.command = 8;
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.delayTime = SPEED_PARTWISE_DELAY_MS;
  currentCommand.param1 = 0;
  currentCommand.colorIndex = sequentialColorIndex;
  sendCommand();
  
  unsigned long startTime = millis();
  bool segmentsLit = false;
  
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    if (!segmentsLit) {
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
      FastLED.show();
      
      int totalLEDs = NUM_LEDS_MAIN + NUM_LEDS_RECEIVER;
      int segmentSize = totalLEDs / 6;
      int mainBoardSegments = min(6, (NUM_LEDS_MAIN + segmentSize - 1) / segmentSize);
      
      for(int segment = 0; segment < mainBoardSegments; segment++) {
        fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
        
        int startLed = segment * segmentSize;
        int endLed = min((segment + 1) * segmentSize, NUM_LEDS_MAIN);
        for(int i = startLed; i < endLed; i++) {
          ledsMain[i] = CRGB(r, g, b);
        }
        FastLED.show();
        delay(SPEED_PARTWISE_DELAY_MS);
      }
      
      segmentsLit = true;
      
      // After all segments are lit, light up full strip
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(r, g, b));
      FastLED.show();
      
      // Send command to receiver to light up all
      currentCommand.command = 8;
      currentCommand.param1 = 255; // Special value for full light up
      sendCommand();
      
      // Keep full strip lit for a while
      delay(2000);
    }
    delay(10);
  }
  
  // Move to next color for next time
  sequentialColorIndex = (sequentialColorIndex + 1) % (sizeof(SEQUENTIAL_COLORS) / sizeof(SEQUENTIAL_COLORS[0]));
}

void partwiseColorLighting() {
  if (!animationInitialized) {
    Serial.println("Animation 9: Partwise Color Lighting");
    animationInitialized = true;
  }

  // Define the color order with your specified colors
  const CRGB colorOrder[] = {
    CRGB(0x00, 0x00, 0xFF),    // Blue (#0000FF)
    CRGB(0xFF, 0xFF, 0x00),    // Yellow (#FFFF00)
    CRGB(0xFF, 0x00, 0x00),    // Red (#FF0000)
    CRGB(0xFF, 0xFF, 0xFF),    // White (#FFFFFF)
    CRGB(0xFF, 0x98, 0x00)     // Vivid Gamboge (#FF9800)
  };

  currentCommand.command = 9;
  currentCommand.delayTime = SPEED_PARTWISE_DELAY_MS;

  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    // First clear all LEDs
    fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
    FastLED.show();
    
    currentCommand.param1 = 6; // Clear command
    sendCommand();
    delay(SPEED_PARTWISE_DELAY_MS);

    // Light up segments one by one with different colors
    for (int segment = 0; segment < 6; segment++) {
      CRGB segmentColor = colorOrder[segment % 5]; // Cycle through 5 colors
      
      // Calculate which LEDs on the main board should light up
      int segStart = SEGMENTS[segment][0];
      int segEnd = SEGMENTS[segment][1];
      
      // Only handle segments that are on the main board
      if (segEnd <= NUM_LEDS_MAIN) {
        // Light up slowly within the segment
        for (int i = segStart-1; i < segEnd; i++) { // Convert to 0-based
          ledsMain[i] = segmentColor;
          FastLED.show();
          delay(SPEED_PARTWISE_DELAY_MS / 2); // Half delay for smoother effect
        }
      }
      
      // Tell receiver to light up its portion of this segment
      currentCommand.param1 = segment + 1; // Segment number (1-6)
      currentCommand.r = segmentColor.r;
      currentCommand.g = segmentColor.g;
      currentCommand.b = segmentColor.b;
      sendCommand();
      
      delay(SPEED_PARTWISE_DELAY_MS);
    }

    // After all segments are lit, light up the special 16 LEDs on receiver
    currentCommand.param1 = 10; // Special command for receiver's 16 LEDs
    sendCommand();
    delay(SPEED_PARTWISE_DELAY_MS * 2); // Longer delay for special effect
  }
}
void breathingEffect() {
  uint8_t r, g, b;
  hexToRGB(COLOR_LIST[colorIndex], r, g, b);
  
  currentCommand.command = 9; // Now matches receiver
  currentCommand.r = r;
  currentCommand.g = g;
  currentCommand.b = b;
  currentCommand.delayTime = SPEED_BREATHING_STEP_MS;
  sendCommand();

  unsigned long startTime = millis();
  while (millis() - startTime < ANIMATION_DURATION_MS) {
    for(int i = 0; i < 256; i++) {
      float breath = (exp(sin(i/16.0*PI)) - 0.36787944)*108.0;
      breath = constrain(breath, 0, 255);
      
      fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB(
        r * breath / 255, 
        g * breath / 255, 
        b * breath / 255
      ));
      FastLED.show();
      delay(SPEED_BREATHING_STEP_MS);
    }
  }
}

void runAnimation(int animationNumber) {
  animationInitialized = false;
  switch(animationNumber) {
     case 0: randomWhiteFlash(); break;
    case 1: fullWhiteFadeOut(); break;
    case 2: colorWipeCombined(); break;
    case 3: specialColorLast16(); break;
    case 4: redWhiteBlueChase(); break;
    case 5: colorFadeInOut(); break;
    case 6: blockLighting(); break;
    case 7: sixPartSequential(); break;
    case 8: breathingEffect(); break; // Changed from 10 to 9
  }
}

bool animationColorCycleComplete() {
  switch(currentAnimation) {
    case 2: return (colorWipeColorIndex == 0);
    case 3: return (special16ColorIndex == 0);
    case 5: return (fadeColorIndex == 0);
    case 6: return (colorIndex == 0);
    case 7: return (sequentialColorIndex == 0);
    case 8: return (colorIndex == 0);
    default: return true;
  }
}

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(ledsMain, NUM_LEDS_MAIN).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
  FastLED.show();
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  
  Serial.println("AP Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send_P(200, "text/html; charset=utf-8", index_html);
  });
  
  server.on("/setcolor", HTTP_POST, [](AsyncWebServerRequest *request) {
  request->send(200, "text/plain; charset=utf-8", "Color set");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
      Serial.println("Error parsing JSON");
      return;
    }
    
    const char* hexColor = doc["color"];
    Serial.print("Setting color to: ");
    Serial.println(hexColor);
    setUserColor(hexColor);
    request->send(200, "text/plain", "Color set successfully");
  });
  
  server.begin();

  for(int i = 0; i < 3; i++) {
    fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::White);
    FastLED.show();
    delay(300);
    fill_solid(ledsMain, NUM_LEDS_MAIN, CRGB::Black);
    FastLED.show();
    delay(300);
  }
  
  Serial.println("Setup completed");
}

void loop() {
  // Check if we have a saved color to display
  if (hasSavedColor) {
    setColor(savedUserColor.r, savedUserColor.g, savedUserColor.b);
    hasSavedColor = false;
    USER_COLOR_ACTIVE = true;
    USER_COLOR_START_TIME = millis();
    return;
  }

  if (USER_COLOR_ACTIVE) {
    if (millis() - USER_COLOR_START_TIME > MANUAL_COLOR_DURATION_MS) {
      USER_COLOR_ACTIVE = false;
    } else {
      delay(100);
      return;
    }
  }
  
  runAnimation(currentAnimation);
  
  if (animationColorCycleComplete()) {
    currentAnimation = (currentAnimation + 1) % totalAnimations;
  }
  
  delay(ANIMATION_TRANSITION_MS);
}