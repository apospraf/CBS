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
            Serial.println(accessToken);
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
    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
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
    http.addHeader("Authorization", _bearerToken);

    int httpCode = http.sendRequest("PUT");
    if(httpCode == -1) {
        Serial.printf("http error: %s\n", http.errorToString(httpCode).c_str());
    } else {
        Serial.printf("HTTP status code %d\n", httpCode);
    }
    http.end();
}

String SpotifyAPI::getCurrentTrack(){
    String url = "https://api.spotify.com/v1/me/player/currently-playing";
    String currentTrackId = "";

    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", _bearerToken);

    int httpCode = http.GET();
    if (httpCode == 200){
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        StaticJsonDocument<64> filter;
        JsonObject filter_item = filter.createNestedObject("item");
        filter_item["id"] = true;
        deserializeJson(doc, payload, DeserializationOption::Filter(filter));
        const char* tempValue = doc["item"]["id"];
        currentTrackId = String(tempValue);

    } else {
        Serial.printf("http error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    return currentTrackId;
}

void SpotifyAPI::saveCurrentTrack(int playlistInt){
    String trackId = getCurrentTrack();
    int httpResponseCode = 0;
    
    
    if (playlistInt == 1){

        String url = "https://api.spotify.com/v1/playlists/" + cbsPlaylistId + "/tracks";
        String payload = "{\"uris\": [\"spotify:track:" + trackId + "\"], \"position\": 0}";
        
        http.begin(client, url);
        http.addHeader("Accept", "application/json");
        http.addHeader("Authorization", _bearerToken);
        http.addHeader("Content-Type", "application/json");
        
        httpResponseCode = http.POST(payload);
    } else {

        String url = "https://api.spotify.com/v1/me/tracks";
        String payload = "{\"ids\": [\"" + trackId + "\"]}";

        http.begin(client, url);
        http.addHeader("Accept", "application/json");
        http.addHeader("Authorization", _bearerToken);
        http.addHeader("Content-Type", "application/json");

        httpResponseCode = http.PUT(payload);
    }

     if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String responseBody = http.getString();
        Serial.println(responseBody);
    } else {
        Serial.print("Error in HTTP request: ");
        Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
}

void SpotifyAPI::findCBSPlaylist() {

  // Prepare the request URL
  String url = "https://api.spotify.com/v1/me/playlists";
  
  // Set the request headers
  http.begin(client, url);
  http.addHeader("Authorization", _bearerToken);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");

  // Send the GET request
  int httpResponseCode = http.GET();
  
  // Check the response
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("Printing payload");
    Serial.println(payload.c_str());
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.println("Error parsing JSON response");
      Serial.println(error.c_str());
      http.end();
      return;
    }

    // Extract playlist information and search for the desired playlist by name
    JsonArray playlists = doc["items"].as<JsonArray>();
    int numPlaylists = playlists.size();

    for (int i = 0; i < numPlaylists; i++) {
      String playlistId = playlists[i]["id"].as<String>();
      String playlistName = playlists[i]["name"].as<String>();

      // Check if the current playlist name matches the desired name
      if (playlistName.equals("CBS")) {
        Serial.println("Playlist found!");
        Serial.print("Playlist ID: ");
        Serial.println(playlistId);
        cbsPlaylistId = playlistId;
        http.end();
        return;
      }
    }

    // Playlist not found
    Serial.println("Playlist not found!");
  } else {
    Serial.print("Error in HTTP request: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  
  // Cleanup
  http.end();
}

void SpotifyAPI::transferPlayback(){
    String url = "https://api.spotify.com/v1/me/player/devices";

    http.begin(client, url);
    http.addHeader("Authorization", _bearerToken);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("Printing payload");
        Serial.println(payload.c_str());
    } else {
        Serial.print("Error in HTTP request: ");
        Serial.println(http.errorToString(httpResponseCode));
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

bool SpotifyAPI::checkToken(){
    if (tokenTimeToLiveMs + timeTokenRefreshed < millis()){
        refreshAccessToken();
        return true;
    } else {
        return false;
    }
}