// Brightness of less than 3 will compromise color accuracy.
#define DAY_BRIGHTNESS 30  // Brightness for daytime 0-255.
#define NIGHT_BRIGHTNESS 3 // Dimmer for night 0-255.

#define DAY_START_HOUR 11   // 7 AM EST = 11 AM UTC
#define NIGHT_START_HOUR 23 // 7 PM EST = 11 PM UTC

#define SECONDS_BETWEEN_READINGS 300       // Seconds between readings (5 minutes).
#define OLD_DATA_THRESHOLD_SECONDS 15 * 60 // If data is older than this, show error (15 minutes).