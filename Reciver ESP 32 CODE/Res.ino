/*
 * ESP32 B (Receiver Board) - RGB LED Controller with ESP-NOW
 * Fixed implementation with proper manual color override
 dd*/

#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>

// LED Configuration
#define LED_PIN     21        // Data pin for FastLED
#define LED_TYPE    WS2812B   // LED strip type
#define COLOR_ORDER RGB       // RGB order for the LEDs

// LED Counts
int NUM_LEDS = 200;            // Number of LEDs on this receiver board
int SPECIAL_START_INDEX = 184; // Last 16 LEDs (50-16=34)

CRGB savedUserColor = CRGB::Black;  // Stores the single user color
bool hasSavedColor = false;         // Flag to indicate if we have a saved color
unsigned long colorDisplayEndTime = 0; // When the color display should end


// Manual Color Settings
#define MANUAL_COLOR_DURATION_MS 10000 // 10 seconds
bool MANUAL_COLOR_OVERRIDE = false;
unsigned long MANUAL_COLOR_TIMEOUT = 0;
CRGB manualColor = CRGB::Black;

// Combined LED configuration (must match main board)
const int NUM_LEDS_MAIN = 100; // Main board's LED count
int TOTAL_LEDS = NUM_LEDS_MAIN + NUM_LEDS;
int SEGMENT_DELAY = 800;      // Must match main board

// Segment configuration (must match main board)
const int SEGMENTS[6][2] = {
  {0, 49},     // Main board (LEDs 0-49)
  {50, 99},    // Main board (LEDs 50-99)
  {100, 149},  // Receiver board (LEDs 100-149)
  {150, 199},  // Receiver board (LEDs 150-199)
  {200, 283},  // Receiver board (LEDs 200-283)
  {284, 299}   // Receiver board (Special last 16 LEDs)
};

// Animation Timing (matches controller speeds)
int SPEED_RANDOM_WHITE_BLINK_MS = 200;
int SPEED_FADE_STEPS = 50;
int SPEED_COLOR_WIPE_DELAY_MS = 50;
int SPEED_SPECIAL_16_DELAY_MS = 150;
int SPEED_CHASE_GROUP_DELAY_MS = 150;
int SPEED_FADE_INOUT_STEP_MS = 30;
int SPEED_BLOCK_LIGHTUP_DELAY_MS = 800;
int SPEED_PARTWISE_DELAY_MS = 600;
int SPEED_BREATHING_STEP_MS = 40;

// Store current LED values
CRGB leds[200]; // Adjust size to match NUM_LEDS

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
  uint8_t colorIndex;
} LEDCommand;

void lightUpLast16LEDs(CRGB color) {
  for(int i = SPECIAL_START_INDEX; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    delay(SPEED_SPECIAL_16_DELAY_MS);
  }
}

void handleSetColor(LEDCommand &cmd) {
  if (cmd.param1 == 1) {
    // Override active
    if (cmd.param2 == 1) { // Special flag for our new sequence
      // 1. Clear all LEDs first
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(200); // Small pause for visual effect
      
      // 2. Light up all LEDs except last 16
      for(int i = 0; i < SPECIAL_START_INDEX; i++) {
        leds[i] = CRGB(cmd.r, cmd.g, cmd.b);
      }
      FastLED.show();
      delay(500); // Pause before starting last 16
      
      // 3. Light up last 16 LEDs one by one
      for(int i = SPECIAL_START_INDEX; i < NUM_LEDS; i++) {
        leds[i] = CRGB(cmd.r, cmd.g, cmd.b);
        FastLED.show();
        delay(SPEED_SPECIAL_16_DELAY_MS);
      }
    } else {
      // Normal color fill
      fill_solid(leds, NUM_LEDS, CRGB(cmd.r, cmd.g, cmd.b));
      FastLED.show();
    }
  } else {
    // Override inactive - clear for animations
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }
}

void randomWhiteFlash(LEDCommand &cmd) {
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { // Run for 10 seconds
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    for(int j = 0; j < NUM_LEDS / 5; j++) {
      int pos = random(NUM_LEDS);
      leds[pos] = CRGB::White;
    }
    FastLED.show();
    delay(SPEED_RANDOM_WHITE_BLINK_MS);
  }
}

void fullWhiteFadeOut(LEDCommand &cmd) {
  if (cmd.param1 == 0) {
    fill_solid(leds, NUM_LEDS, CRGB(cmd.r, cmd.g, cmd.b));
    FastLED.show();
  } else {
    for(int j = 0; j < NUM_LEDS; j++) {
      leds[j].fadeToBlackBy(255 / SPEED_FADE_STEPS);
    }
    FastLED.show();
  }
}

void colorWipeCombined(LEDCommand &cmd) {
  CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
  
  if (cmd.param1 == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  } else {
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = color;
      FastLED.show();
      delay(SPEED_COLOR_WIPE_DELAY_MS);
    }
  }
}

void specialColorLast16(LEDCommand &cmd) {
  CRGB mainColor = CRGB(cmd.r, cmd.g, cmd.b);
  CRGB specialColor = CRGB(cmd.specialR, cmd.specialG, cmd.specialB);
  
  fill_solid(leds, NUM_LEDS, mainColor);
  FastLED.show();
  
  for(int i = SPECIAL_START_INDEX; i < NUM_LEDS; i++) {
    leds[i] = specialColor;
    FastLED.show();
    delay(SPEED_SPECIAL_16_DELAY_MS);
  }
}

void redWhiteBlueChase(LEDCommand &cmd) {
  CRGB color1 = CRGB(cmd.r, cmd.g, cmd.b);
  CRGB color2 = CRGB(cmd.specialR, cmd.specialG, cmd.specialB);
  CRGB color3 = CRGB(cmd.param1, cmd.param2, 255); // Blue is fixed
  
  CRGB colors[3] = {color1, color2, color3};
  
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    for(int colorStep = 0; colorStep < 3; colorStep++) {
      for(int i = 0; i < NUM_LEDS; i++) {
        int colorIndex = (i / 10 + colorStep) % 3;
        leds[i] = colors[colorIndex];
      }
      FastLED.show();
      delay(SPEED_CHASE_GROUP_DELAY_MS);
    }
  }
}

void colorFadeInOut(LEDCommand &cmd) {
  CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
  
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    for(int i = 0; i <= 255; i += 5) {
      fill_solid(leds, NUM_LEDS, CRGB(color.r * i / 255, color.g * i / 255, color.b * i / 255));
      FastLED.show();
      delay(SPEED_FADE_INOUT_STEP_MS);
    }
    
    delay(500);
    
    for(int i = 255; i >= 0; i -= 5) {
      fill_solid(leds, NUM_LEDS, CRGB(color.r * i / 255, color.g * i / 255, color.b * i / 255));
      FastLED.show();
      delay(SPEED_FADE_INOUT_STEP_MS);
    }
  }
}
void blockLighting(LEDCommand &cmd) {
  CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
  int segment = cmd.param1 - 1; // Convert to 0-5
  
  if(segment >= 0 && segment < 6) {
    int start = SEGMENTS[segment][0] - 100; // Convert to receiver's index (0-199)
    int end = SEGMENTS[segment][1] - 100;
    
    // Handle special last 16 LEDs differently if needed
    if(segment == 5) { // Special segment
      for(int i = start; i <= end; i++) {
        leds[i] = color;
        FastLED.show();
        delay(SPEED_SPECIAL_16_DELAY_MS); // Special delay for last 16
      }
    } else {
      for(int i = start; i <= end; i++) {
        leds[i] = color;
      }
      FastLED.show();
    }
  }
}

void sixPartSequential(LEDCommand &cmd) {
  CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
  
  if (cmd.param1 == 0) {
    // Clear all LEDs
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  } 
  else if (cmd.param1 == 255) {
    // Light all LEDs (final stage)
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
  } 
  else {
    // Handle specific segment (param1 = segment number 1-6)
    int seg = cmd.param1 - 1;
    if (seg >= 0 && seg < 6) {
      int segStart = SEGMENTS[seg][0] - 1; // Convert to 0-based
      int segEnd = SEGMENTS[seg][1] - 1;   // Convert to 0-based
      
      // Clear all LEDs first if this is the first segment
      if (seg == 0) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
      }
      
      // Light up the current segment
      for (int i = segStart; i <= segEnd; i++) {
        leds[i] = color;
      }
      FastLED.show();
      delay(SPEED_PARTWISE_DELAY_MS);
    }
  }
}


void partwiseColorLighting(LEDCommand &cmd) {
  // Define the color order with your specified colors
  const CRGB colorOrder[] = {
    CRGB(0x00, 0x00, 0xFF),    // Blue (#0000FF)
    CRGB(0xFF, 0xFF, 0x00),    // Yellow (#FFFF00)
    CRGB(0xFF, 0x00, 0x00),    // Red (#FF0000)
    CRGB(0xFF, 0xFF, 0xFF),    // White (#FFFFFF)
    CRGB(0xFF, 0x98, 0x00)     // Vivid Gamboge (#FF9800)
  };

  if (cmd.param1 == 0) {
    // Initial command - do nothing
    return;
  } else if (cmd.param1 == 6) {
    // Clear command
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  } else if (cmd.param1 >= 1 && cmd.param1 <= 6) {
    // Handle specific segment (param1 = segment number 1-6)
    int seg = cmd.param1 - 1;
    if (seg >= 0 && seg < 6) {
      int segStart = SEGMENTS[seg][0];
      int segEnd = SEGMENTS[seg][1];
      
      // Calculate which LEDs on the receiver should light up
      int receiverStart = max(segStart - NUM_LEDS_MAIN - 1, 0);
      int receiverEnd = min(segEnd - NUM_LEDS_MAIN - 1, NUM_LEDS - 1);
      
      if (receiverStart <= receiverEnd) {
        CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
        // Light up slowly within the segment
        for (int i = receiverStart; i <= receiverEnd; i++) {
          leds[i] = color;
          FastLED.show();
          delay(SPEED_PARTWISE_DELAY_MS / 2); // Half delay for smoother effect
        }
      }
    }
  } else if (cmd.param1 == 10) {
    // Handle the special 16 LEDs divided into 5 parts
    for (int part = 0; part < 5; part++) {
      // Calculate the start and end of this part in the special 16 LEDs
      int partStart = SPECIAL_START_INDEX + (16 * part) / 5;
      int partEnd = SPECIAL_START_INDEX + (16 * (part + 1)) / 5;
      
      // Light up this part with the corresponding color
      for (int i = partStart; i < partEnd; i++) {
        leds[i] = colorOrder[part];
        FastLED.show();
        delay(SPEED_PARTWISE_DELAY_MS / 3); // Even slower for special effect
      }
    }
  }
}
void breathingEffect(LEDCommand &cmd) {
  CRGB color = CRGB(cmd.r, cmd.g, cmd.b);
  unsigned long startTime = millis();
  
  while (millis() - startTime < 10000) { // 10 sec duration
    for(int i = 0; i < 256; i++) {
      float breath = (exp(sin(i/16.0*PI)) - 0.36787944)*108.0;
      breath = constrain(breath, 0, 255);
      
      fill_solid(leds, NUM_LEDS, CRGB(
        color.r * breath / 255,
        color.g * breath / 255,
        color.b * breath / 255
      ));
      FastLED.show();
      delay(cmd.delayTime > 0 ? cmd.delayTime : 40);
    }
  }
}

// Updated callback with correct signature
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  const uint8_t *mac = info->src_addr;
  
  if (len == sizeof(LEDCommand)) {
    LEDCommand receivedCommand;
    memcpy(&receivedCommand, incomingData, sizeof(receivedCommand));

    // Always process color commands immediately (command 0)
    if (receivedCommand.command == 0) {
      savedUserColor = CRGB(receivedCommand.r, receivedCommand.g, receivedCommand.b);
      hasSavedColor = true;
      colorDisplayEndTime = millis() + MANUAL_COLOR_DURATION_MS;
      
      // Immediately show the new color
      fill_solid(leds, NUM_LEDS, savedUserColor);
      FastLED.show();
      return;
    }

    // Only process animation commands if not displaying a user color
    if (!hasSavedColor || millis() > colorDisplayEndTime) {
      // Update delay times if they've changed
      if (receivedCommand.delayTime > 0) {
        switch(receivedCommand.command) {
          case 1: SPEED_RANDOM_WHITE_BLINK_MS = receivedCommand.delayTime; break;
          case 2: // Fall through
          case 3: SPEED_COLOR_WIPE_DELAY_MS = receivedCommand.delayTime; break;
          case 4: SPEED_SPECIAL_16_DELAY_MS = receivedCommand.delayTime; break;
          case 5: SPEED_CHASE_GROUP_DELAY_MS = receivedCommand.delayTime; break;
          case 6: SPEED_FADE_INOUT_STEP_MS = receivedCommand.delayTime; break;
          case 7: SPEED_BLOCK_LIGHTUP_DELAY_MS = receivedCommand.delayTime; break;
          case 8: SPEED_PARTWISE_DELAY_MS = receivedCommand.delayTime; break;
          case 9: SPEED_BREATHING_STEP_MS = receivedCommand.delayTime; break;
        }
      }

      // Handle animation commands
      switch(receivedCommand.command) {
        case 1: randomWhiteFlash(receivedCommand); break;
        case 2: fullWhiteFadeOut(receivedCommand); break;
        case 3: colorWipeCombined(receivedCommand); break;
        case 4: specialColorLast16(receivedCommand); break;
        case 5: redWhiteBlueChase(receivedCommand); break;
        case 6: colorFadeInOut(receivedCommand); break;
        case 7: blockLighting(receivedCommand); break;
        case 8: sixPartSequential(receivedCommand); break;
        case 9: breathingEffect(receivedCommand); break;
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  WiFi.mode(WIFI_STA);
  
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  
  // Startup animation
  for(int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    delay(300);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(300);
  }
  
  Serial.println("Receiver board ready");
}

void loop() {
  // Check if we should display the saved color
  if (hasSavedColor) {
    if (millis() < colorDisplayEndTime) {
      // Keep displaying the color until time expires
      delay(100);
      return;
    } else {
      // Color display time expired
      hasSavedColor = false;
      // Clear LEDs when color display ends
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
  }
  
  delay(10); // Small delay to prevent watchdog triggers
}