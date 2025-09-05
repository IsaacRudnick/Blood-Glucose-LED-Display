// Wi-Fi Configuration
const char *ssid = "MyWifi";
const char *password = "Password123";

// Server Configuration
const char *serverName = "mynightscoutsite.com";
// 443 if HTTPS, 80 if HTTP. If you don't know, it's probably 443.
const int port = 443;
// To check if you need this, go to
// {yoursite}/api/v2/entries.json?count=2 in a private/incognito browser window.
// If it asks for an API key or token, you need one.
// See https://nightscout.github.io/nightscout/admin_tools/ for details.
// Ensure the token has "readable" permissions.
// If you don't need one, just set this to an empty string "".
const char *apiToken = "somethinglikethis-1234567890abcdef";