#include <FastLED.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoHTTPClient.h>
#include <ArduinoJson.h>
#include "font4x6.h"
#include "Secrets.h"
#include "Settings.h"

// LED Matrix Configuration
#define LED_DATA_PIN 26
#define LED_COUNT (8 * 32)
#define WIDTH 32
#define HEIGHT 8

CRGB leds[LED_COUNT];

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60 * 1000); // UTC timezone, update every 60s

WiFiClientSecure wifiClient;
HttpClient client(wifiClient, serverName, port);

bool isDayTime()
{
  int currentHour = timeClient.getHours();
  // Account for the fact that night_start_hour might be after midnight
  if (DAY_START_HOUR < NIGHT_START_HOUR)
  {
    return (currentHour >= DAY_START_HOUR && currentHour < NIGHT_START_HOUR);
  }
  else
  {
    return (currentHour >= DAY_START_HOUR || currentHour < NIGHT_START_HOUR);
  }
}

void setMaxBrightnessFromTime()
{
  if (isDayTime())
  {
    FastLED.setBrightness(DAY_BRIGHTNESS);
  }
  else
  {
    FastLED.setBrightness(NIGHT_BRIGHTNESS); // Nighttime brightness
  }
}

// Map 2D coords to LED index for your matrix layout
// Zero-based indexing: (0,0) is bottom-left, (31,7) is top-right
int mapXYtoIndex(int x, int y)
{
  // Subtract 1 for 0-based indexing
  int colFromRight = WIDTH - 1 - x;
  int colStart = colFromRight * HEIGHT;
  // Serpentine layout, so reverse every other column
  bool isEvenCol = (colFromRight % 2 == 0);
  if (isEvenCol)
  {
    return colStart + y; // Even column: bottom → top
  }
  else
  {
    // Subtract 1 for 0-based indexing
    return colStart + (HEIGHT - 1 - y); // Odd column: top → bottom
  }
}

// Draw a single 4x6 character from font4x6
void drawChar(char c, int x, int yTop, CRGB color)
{
  if (c < 32)
    return; // Only printable chars supported

  int charIndex = (c - 32) * 6; // Each char uses 6 bytes (rows), though the last 3 are empty.
  int width = font4x6[0];
  int height = font4x6[1];

  for (int row = 0; row < height; row++)
  {
    if (yTop + row >= HEIGHT || yTop + row < 0)
      continue;
    uint8_t rowBits = font4x6[3 + charIndex + row]; // byte for this row
    for (int col = 0; col < width; col++)
    {
      if (x + col < 0 || x + col >= WIDTH)
        continue;
      // Bit 7 = leftmost pixel, bit 4 = rightmost pixel (since font bits are left aligned)
      // Our font is stored MSB to LSB left to right, so test bit 7-col
      if (rowBits & (0x80 >> col))
      {
        int px = x + col;
        int py = (HEIGHT - 1) - (yTop + row); // flip vertical for LED indexing

        // Subtract 1 from y to lower the visual by 1 pixel. This is optional.
        py -= 1;
        leds[mapXYtoIndex(px, py)] = color;
      }
    }
  }
}

// Draw a string horizontally with 1 pixel spacing (4 pixels per char)
void drawString(const char *str, int xStart, int yTop, CRGB color)
{
  int x = xStart;
  for (int i = 0; str[i] != '\0'; i++)
  {
    if (str[i] == ' ')
    {
      x += 2; // space width smaller (2 pixels)
      continue;
    }
    drawChar(str[i], x, yTop, color);
    // Move cursor to the right by 4, since font is 4 pixels wide (spaces are built in)
    x += 4;
  }
}

// Draw bottom row progress bar (y=0)
void drawProgressBar(int elapsedTimeSeconds, int maxTimeSeconds)
{
  int barLength = map(elapsedTimeSeconds, 0, maxTimeSeconds, 0, WIDTH - 1);
  barLength = constrain(barLength, 0, WIDTH - 1);

  // Clear bottom row
  for (int x = 0; x < WIDTH; x++)
  {
    leds[mapXYtoIndex(x, 0)] = CRGB::Black;
  }

  // Daytime progress bar is bottom row filling up from left to right in white
  if (isDayTime())
  {
    for (int x = 0; x < barLength; x++)
    {
      leds[mapXYtoIndex(x, 0)] = CRGB::White;
    }
    return;
  }
  // Nighttime progress bar is a single pixel moving from left to right.
  // There's also a start and end pixel in dark blue for visual reference.

  // First and last pixel of bottom row in dark blue
  leds[mapXYtoIndex(0, 0)] = CRGB::DarkBlue;
  leds[mapXYtoIndex(WIDTH - 1, 0)] = CRGB::DarkBlue;
  // Last pixel of progress bar in white
  leds[mapXYtoIndex(barLength - 1, 0)] = CRGB::White;
}

CRGB getColorForValue(int value)
{
  // Modify color logic here
  // This is the default color for in-range values
  CRGB color = CRGB::Green;
  // If value is beyond critical threshold, show red
  if (value < CRITICAL_LOW_THRESHOLD || value > CRITICAL_HIGH_THRESHOLD)
  {
    color = CRGB::Red;
  }
  // If value is beyond moderate threshold, show orange
  else if (value < MODERATE_LOW_THRESHOLD || value > MODERATE_HIGH_THRESHOLD)
  {
    color = CRGB(255, 140, 0); // Orange
  }
  return color;
}

// Display value, delta and progress bar
void displayValueAndDelta(int value, int delta, int elapsedSeconds)
{
  FastLED.clear();

  CRGB color = getColorForValue(value);

  // If value is beyond critical threshold, show red
  if (value < CRITICAL_LOW_THRESHOLD || value > CRITICAL_HIGH_THRESHOLD)
  {
    color = CRGB::Red;
  }
  // If value is beyond moderate threshold, show orange
  else if (value < MODERATE_LOW_THRESHOLD || value > MODERATE_HIGH_THRESHOLD)
  {
    color = CRGB(255, 140, 0); // Orange
  }
  // This is the end of the color function

  char displayStr[12];
  snprintf(displayStr, sizeof(displayStr), " %d %c%d", value, (delta >= 0 ? '+' : '-'), abs(delta));

  drawString(displayStr, 0, 0, color);

  drawProgressBar(elapsedSeconds, SECONDS_BETWEEN_READINGS);

  FastLED.show();
}

// ==================== BOOT ANIMATIONS ====================

// 2. White pixel animation
void bootWhitePixel()
{
  const int tailLength = 8;  // length of the white snake
  const int speedDelay = 30; // delay between frames in ms

  FastLED.clear();

  for (int pos = 0; pos < WIDTH + tailLength; pos++)
  {
    FastLED.clear();

    for (int t = 0; t < tailLength; t++)
    {
      int x = pos - t;
      if (x >= 0 && x < WIDTH)
      {
        // Tail brightness decreases from head to tail
        uint8_t brightness = 255 * (tailLength - t) / tailLength;

        for (int y = 0; y < HEIGHT; y++)
        {
          leds[mapXYtoIndex(x, y)] = CRGB(brightness, brightness, brightness);
        }
      }
    }

    FastLED.show();
    delay(speedDelay);
  }
}

// 3. Rainbow wave
void bootRainbowWave(uint16_t duration_ms = 3000)
{
  uint32_t startTime = millis();
  float offset = 0.0;
  while (millis() - startTime < duration_ms)
  {
    for (uint8_t y = 0; y < HEIGHT; y++)
    {
      for (uint8_t x = 0; x < WIDTH; x++)
      {
        float wave = sinf((x + offset) * 0.3) + cosf((y + offset) * 0.3);
        uint8_t hue = (wave * 40) + 128;
        leds[mapXYtoIndex(x, y)] = CHSV(hue, 255, 255);
      }
    }
    FastLED.show();
    offset += 0.2;
    delay(20);
  }
  FastLED.clear();
  FastLED.show();
}

void runBootSequence()
{
  bootWhitePixel();
  bootRainbowWave();
}

void displayWifiError()
{
  FastLED.clear();
  drawString("WIFI ERR", 1, 1, CRGB::Red);
  FastLED.show();
}

void setup()
{
  Serial.begin(9600);
  delay(100);

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_COUNT);
  setMaxBrightnessFromTime();
  FastLED.clear();
  FastLED.show();
  runBootSequence();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long start_connecting_time = millis();
  int try_for_seconds = 10;
  FastLED.clear();
  drawString("WAITWIFI", 0, 1, CRGB::White);
  FastLED.show();
  // Wait up to 10 seconds for connection
  while (WiFi.status() != WL_CONNECTED && (millis() - start_connecting_time) < try_for_seconds * 1000)
  {
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to Wi-Fi");
    // Print "WIFI ERR" to display
    displayWifiError();
  }
  else
  {
    Serial.println("Wi-Fi connected");
  }

  // Start an NTP client for accurate UNIX time
  timeClient.begin();
  timeClient.update();

  // Skip https certificate validation
  wifiClient.setInsecure();
}

struct Reading
{
  unsigned long epoch = 0;
  int value = 0;
  int delta = 0;
};

Reading lastReading;

bool ensureWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return true;

  WiFi.reconnect();
  delay(500);
  Serial.print("Wifi disconnected. Reconnecting...");
  displayWifiError();
  return false;
}

void displayHTTPError()
{
  FastLED.clear();
  drawString("HTTP ERR", 1, 1, CRGB::Red);
  FastLED.show();
}

// Url is string + apiToken
const String url = String("/api/v2/entries.json?count=2&token=") + apiToken;

// Fetch the latest reading from the API
// Returns true if successful, false otherwise.
bool fetchLatestReading()
{
  Serial.print("Making request: ");
  Serial.println(url);

  client.get(url);
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode != 200)
  {
    Serial.printf("HTTP Error: %d\n", statusCode);
    Serial.println("Response: " + response);
    displayHTTPError();
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, response))
  {
    Serial.println("JSON parse error");
    return false;
  }

  int mostRecentValue = doc[0]["sgv"];
  unsigned long mostRecentDate = doc[0]["date"].as<unsigned long long>() / 1000;
  int previousValue = doc[1]["sgv"];
  int delta = mostRecentValue - previousValue;

  lastReading.epoch = mostRecentDate;
  lastReading.value = mostRecentValue;
  lastReading.delta = delta;

  Serial.printf("Most recent value: %d\n", mostRecentValue);
  Serial.printf("Delta: %d\n", delta);

  return true;
}

void displayOldDataError()
{
  FastLED.clear();
  drawString("OLD DATA", 1, 1, CRGB::Red);
  FastLED.show();
  delay(1000);
}

void loop()
{
  if (!ensureWiFi())
  {
    return;
  }

  timeClient.update();
  setMaxBrightnessFromTime();

  unsigned long currentEpoch = timeClient.getEpochTime();
  // If we have no previous reading, or it's been a long time, fetch a new one.
  int secondsSinceLastReading = (lastReading.epoch > 0)
                                    ? (int)(currentEpoch - lastReading.epoch)
                                    : INT_MAX; // INT_MAX so that we always fetch on the first run

  // If we expect a new reading, fetch it.
  if (secondsSinceLastReading >= SECONDS_BETWEEN_READINGS)
  {
    if (fetchLatestReading())
    {
      secondsSinceLastReading = (int)(currentEpoch - lastReading.epoch);
    }
    else
    {
      // fetchLatestReading will only return false if there was an HTTP error or JSON parse error.
      // Since nightscout API is static, JSON parsing (should) always work. So if we get here, it's an HTTP error.
      displayHTTPError();
      return;
    }
  }

  // If the last reading is too old, show an error.
  if (secondsSinceLastReading > OLD_DATA_THRESHOLD_SECONDS)
  {
    displayOldDataError();
    return;
  }

  // For 10 seconds, display the value, delta and progress bar.
  // This lets us avoid making too many HTTP requests too quickly.
  unsigned long start = millis();
  // This is a loop instead of a single displayValueAndDelta call so that the progress bar updates over time.
  while (millis() - start < 10 * 1000)
  {
    displayValueAndDelta(lastReading.value, lastReading.delta, secondsSinceLastReading);
    delay(100);
  }
}
