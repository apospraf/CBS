#include "SpotifyAPI.h"

SpotifyAPI::SpotifyAPI(const char *clientId, const char *clientSecret){
    client.setInsecure();
    this->_clientId = clientId;
    this->_clientSecret = clientSecret;
}

void SpotifyAPI::setRefreshToken(const char *refreshToken)
{
    int newRefreshTokenLen = strlen(refreshToken);
    if (_refreshToken == NULL || strlen(_refreshToken) < newRefreshTokenLen)
    {
        delete _refreshToken;
        _refreshToken = new char[newRefreshTokenLen + 1]();
    }

    strncpy(_refreshToken, refreshToken, newRefreshTokenLen + 1);
}

const char *SpotifyAPI::requestAccessTokens(const char *code, const char *redirectUrl){
    char body[500];
    sprintf(body, requestAccessTokensBody, code, redirectUrl, _clientId, _clientSecret);
    
    http.begin(client, "https://accounts.spotify.com/api/token");
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String payload = "grant_type=authorization_code&code=" + String(code) + "&redirect_uri=" + String(redirectUrl) + "&client_id=" + String(_clientId) + "&client_secret=" + String(_clientSecret);
    int statusCode = http.POST(payload);
    unsigned long now = millis();
    if (statusCode == 200) {
        DynamicJsonDocument doc(1000);
        DeserializationError error = deserializeJson(doc, client);
        if (!error)
        {
            sprintf(this->_bearerToken, "Bearer %s", doc["access_token"].as<const char *>());
            setRefreshToken(doc["refresh_token"].as<const char *>());
            int tokenTtl = doc["expires_in"];             // Usually 3600 (1 hour)
            tokenTimeToLiveMs = (tokenTtl * 1000) - 10000; // The 2000 is just to force the token expiry to check if its very close
            timeTokenRefreshed = now;
        }
    } else {
        char message[50];
        sprintf(message, "Request failed, code %d", statusCode);
        Serial.println(message);
    }
    http.end();
    return _refreshToken;
}

bool SpotifyAPI::refreshAccessToken()
{
    http.begin(client, "https://accounts.spotify.com/api/token");
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    
    String payload = "grant_type=refresh_token&refresh_token=" + String(_refreshToken) + "&client_id=" + String(_clientId) + "&client_secret=" + String(_clientSecret);
    // Set up the request data
    int statusCode = http.POST(payload);

    bool refreshed = false;
    unsigned long now = millis();
    if (statusCode == 200)
    {
        StaticJsonDocument<48> filter;
        filter["access_token"] = true;
        filter["token_type"] = true;
        filter["expires_in"] = true;

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

        const char *accessToken = doc["access_token"].as<const char *>();
        if (accessToken != NULL && (SPOTIFY_ACCESS_TOKEN_LENGTH >= strlen(accessToken)))
        {
            sprintf(this->_bearerToken, "Bearer %s", accessToken);
            int tokenTtl = doc["expires_in"];             // Usually 3600 (1 hour)
            tokenTimeToLiveMs = (tokenTtl * 1000) - 2000; // The 2000 is just to force the token expiry to check if its very close
            timeTokenRefreshed = now;
            refreshed = true;
        }
    } else {
        char message[50];
        sprintf(message, "Request failed, code %d", statusCode);
        Serial.println(message);
    }
    http.end();
    return refreshed;
}

void SpotifyAPI::nextPreviousTrack(bool next){
    String url = next? "https://api.spotify.com/v1/me/player/next" : "https://api.spotify.com/v1/me/player/previous";

    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", "0");
    http.addHeader("Authorization", _bearerToken);

    int statusCode = http.sendRequest("POST");

    if (statusCode == HTTP_CODE_OK){
        Serial.println("All good");
    } else {
        char message[50];
        sprintf(message, "Request failed, code %d", statusCode);
        Serial.println(message);
    }
    http.end();
}

void SpotifyAPI::getPlaybackState(){
    String url = "https://api.spotify.com/v1/me/player";
    http.begin(this->client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", "0");
    http.addHeader("Authorization", _bearerToken);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Got playback state");
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        StaticJsonDocument<64> filter;
        filter["is_playing"] = true;
        deserializeJson(doc, payload, DeserializationOption::Filter(filter));
        isPlaying = doc["is_playing"];
    } else {
        char message[50];
        sprintf(message, "HTTP request failed %s", http.errorToString(httpCode));
        Serial.println(message);
    }
    http.end();
}

void SpotifyAPI::playPause(){
    String url = isPlaying? "https://api.spotify.com/v1/me/player/pause" : "https://api.spotify.com/v1/me/player/play";

    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", "0");
    http.addHeader("Authorization", _bearerToken);

    int httpCode = http.sendRequest("PUT");
    if(httpCode == -1) {
        Serial.printf("http error: %s\n", http.errorToString(httpCode).c_str());
    } else {
        Serial.printf("HTTP status code %d\n", httpCode);
    }
    http.end();
}


void SpotifyAPI::handleRoot()
{
  char webpage[800];
  sprintf(webpage, _webpageTemplate, _clientId, _callbackURI, _scope);
  server.send(200, "text/html", webpage);
}

void SpotifyAPI::handleCallback()
{
  String code = "";
  const char *refreshToken = NULL;
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "code")
    {
      code = server.arg(i);
      refreshToken = requestAccessTokens(code.c_str(), _callbackURI);
      writeRefreshTokenToEEPROM();
      gotRefreshToken = true;
    }
  }

  if (refreshToken != NULL)
  {
    server.send(200, "text/plain", refreshToken);
  }
  else
  {
    server.send(404, "text/plain", "Failed to load token, check serial monitor");
  }
}

void SpotifyAPI::handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  Serial.print(message);
  server.send(404, "text/plain", message);
}


bool SpotifyAPI::getRefreshTokenFromEEPROM(){
    int length = 0;
    String refreshToken;

    EEPROM.begin(512);
    for (int i = 96; i < 512; ++i) {
        if (EEPROM.read(i)!=0){
            length ++;
            refreshToken += char(EEPROM.read(i));

        } else {
            break;
        }
    }
    EEPROM.end();
    if (length > 2) {
        setRefreshToken(refreshToken.c_str());
        gotRefreshToken = true;
    } else {
        gotRefreshToken = false;
    }
    return gotRefreshToken;
}

void SpotifyAPI::writeRefreshTokenToEEPROM(){
    EEPROM.begin(512);
    for (int i = 0; i < strlen(_refreshToken); i++){
        EEPROM.write(i+96, _refreshToken[i]);
    }
    EEPROM.end();
}

void SpotifyAPI::setSpotifyServer(){
    server.on("/", [this]() {
        this->handleRoot();
    });
    server.on("/callback/", [this](){
        this->handleCallback();
    });
    server.onNotFound([this](){
        this->handleNotFound();
    });
    server.begin(); 
}

size_t SpotifyAPI::getStringLength(char* str) {
  size_t length = 0;
  while (str[length] != '\0') {
    length++;
  }
  return length;
}

bool SpotifyAPI::checkToken(){
    if (tokenTimeToLiveMs + timeTokenRefreshed < millis()){
        refreshAccessToken();
        return true;
    } else {
        return false;
    }
}