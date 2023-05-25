#ifndef SpotifyAPI_h
#define SpotifyAPI_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

 
#define SPOTIFY_HOST "api.spotify.com"
#define SPOTIFY_ACCOUNTS_HOST "accounts.spotify.com"

#define SPOTIFY_FINGERPRINT "9F 3F 7B C6 26 4C 97 06 A2 D4 D7 B2 35 45 D9 AA 8D BD CD 4D"

#define SPOTIFY_IMAGE_SERVER_FINGERPRINT "8B 24 D0 B7 12 AC DB 03 75 09 45 95 24 FF BE D8 35 E6 EB DF"

#define SPOTIFY_PLAYER_ENDPOINT "/v1/me/player"
#define SPOTIFY_DEVICES_ENDPOINT "/v1/me/player/devices"

#define SPOTIFY_PLAY_ENDPOINT "/v1/me/player/play"
#define SPOTIFY_PAUSE_ENDPOINT "/v1/me/player/pause"
#define SPOTIFY_VOLUME_ENDPOINT "/v1/me/player/volume?volume_percent=%d"

#define SPOTIFY_NEXT_TRACK_ENDPOINT "/v1/me/player/next"
#define SPOTIFY_PREVIOUS_TRACK_ENDPOINT "/v1/me/player/previous"

#define SPOTIFY_TOKEN_ENDPOINT "/api/token"

#define SPOTIFY_SAVE_TRACK_ENDPOINT "/v1/playlists/%s/tracks"

#define SPOTIFY_ACCESS_TOKEN_LENGTH 309

class SpotifyAPI {

    public:
        SpotifyAPI(const char *clientId, const char *clientSecret);

        // Auth Methods
        void setRefreshToken(const char *refreshToken);
        bool refreshAccessToken();
        const char *requestAccessTokens(const char *code, const char *redirectUrl);

        bool getRefreshTokenFromEEPROM();
        void writeRefreshTokenToEEPROM();
        bool checkToken();

        void playPause();
        void getPlaybackState();
        void nextPreviousTrack(bool next);

        void handleRoot();
        void handleCallback();
        void handleNotFound();

        void setSpotifyServer();

        size_t getStringLength(char *);

        bool isPlaying;
        HTTPClient http;
        WiFiClientSecure client;
        ESP8266WebServer server;
        bool gotRefreshToken = false;

    private: 
        char _bearerToken[SPOTIFY_ACCESS_TOKEN_LENGTH + 10]; //10 extra is for "bearer " at the start
        char *_refreshToken;
        const char *_clientId;
        const char *_clientSecret;
        unsigned int timeTokenRefreshed;
        unsigned int tokenTimeToLiveMs;
        const char *requestAccessTokensBody =
            R"(grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&client_secret=%s)";
        const char *refreshAccessTokensBody =
            R"(grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s)";
            const char *_webpageTemplate =
            R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <meta http-equiv="X-UA-Compatible" content="IE=edge">
            <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
        </head>
        <body>
            <div>
            <a href="https://accounts.spotify.com/authorize?client_id=%s&response_type=code&redirect_uri=%s&scope=%s">spotify Auth</a>
            </div>
        </body>
        </html>
        )";
        const char *_callbackURI = R"(http%3A%2F%2Fcbs.local%2Fcallback%2F)";

        const char *_scope = R"(user-read-playback-state%20user-modify-playback-state%20playlist-modify-public%20playlist-modify-private)";
        
};

#endif