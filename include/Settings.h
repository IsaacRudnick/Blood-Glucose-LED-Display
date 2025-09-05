// Brightness of less than 3 will compromise color accuracy. DAY_BRIGHTNESS is also used for boot animations, since they run before the NTP time is set.
#define DAY_BRIGHTNESS 30  // Brightness for daytime 0-255.
#define NIGHT_BRIGHTNESS 3 // Dimmer for night 0-255.

// To disable night mode, set DAY_START_HOUR to 0 NIGHT_START_HOUR to 24.
#define DAY_START_HOUR 11  // 7 AM EST = 11 AM UTC
#define NIGHT_START_HOUR 1 // 9 PM EST = 1 AM UTC

#define SECONDS_BETWEEN_READINGS 300       // Seconds between readings (5 minutes).
#define OLD_DATA_THRESHOLD_SECONDS 15 * 60 // If data is older than this, show error (15 minutes).

// To modify the display colors or logic, Ctrl+F for "Modify color logic here" in src/main.cpp
#define MODERATE_LOW_THRESHOLD 75   // Below this value, show orange alert color.
#define CRITICAL_LOW_THRESHOLD 65   // Below this value, show red alert color.
#define MODERATE_HIGH_THRESHOLD 180 // Above this value, show orange alert color.
#define CRITICAL_HIGH_THRESHOLD 200 // Above this value, show red alert color.