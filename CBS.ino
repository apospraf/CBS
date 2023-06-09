#include <CBS.h>
#include <PinButton.h>

char clientId[] = "";     // Your client ID of your spotify APP
char clientSecret[] = ""; // Your client Secret of your spotify APP (Do Not share this!)

char scope[] = "user-read-playback-state%20user-modify-playback-state%20playlist-modify-public%20playlist-modify-private";

const int wifiPin = 5;
const int spotifyPin = 4;

unsigned long lastTime = 0;

PinButton playPauseButton(12);
PinButton nextPreviousButton(14);
PinButton saveButton(13);

ServerUtilities serverUtilities;
SpotifyAPI spotifyAPI(clientId, clientSecret);
CBS cbs;

void setup(){
  Serial.begin(115200);
  EEPROM.begin(512);
  
  pinMode(wifiPin, OUTPUT);
  pinMode(spotifyPin, OUTPUT);
  WiFi.mode(WIFI_STA);
  serverUtilities.getSSIDPASS();
  WiFi.begin(serverUtilities.esid.c_str(), serverUtilities.epass.c_str());
  Serial.println("");

  if (serverUtilities.testWifi()){
    Serial.println("Succesfully Connected!!!");
  } else {
    serverUtilities.launchWeb();
    serverUtilities.setupAP();
  }
  
  Serial.println();
  Serial.println("Waiting.");
  
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(500);
    serverUtilities.server.handleClient();
    cbs.ledBlink(wifiPin);
  }
  EEPROM.end();

  digitalWrite(wifiPin, HIGH);

  if (spotifyAPI.getRefreshTokenFromEEPROM()){
    Serial.println("Found refresh token in eeprom");
    spotifyAPI.refreshAccessToken();
  } else {
    
    if (MDNS.begin("cbs")) {
      Serial.println("MDNS responder started");
    }
    Serial.println("HTTP server started");
    spotifyAPI.setSpotifyServer();
  }

  while (!spotifyAPI.gotRefreshToken){
    delay(500);
    cbs.ledBlink(spotifyPin);
    MDNS.update();
    spotifyAPI.server.handleClient();
  }
  digitalWrite(spotifyPin, HIGH);

}

void loop() {
  playPauseButton.update();
  nextPreviousButton.update();
  saveButton.update();
  
  if (millis() - lastTime >= 30000) {
    if ( spotifyAPI.checkToken() ){
      Serial.println("Token refreshed");
    }
    lastTime = millis();
  }

  if (nextPreviousButton.isSingleClick()){
    spotifyAPI.nextPreviousTrack(true);
  }
  if (nextPreviousButton.isDoubleClick()){
    spotifyAPI.nextPreviousTrack(false);
  }

  if (playPauseButton.isSingleClick()){
    spotifyAPI.getPlaybackState();
    spotifyAPI.playPause();
    
  }

  if (saveButton.isLongClick()){
//    cbs.clearEEPROM(); 
//   spotifyAPI.transferPlayback();
    spotifyAPI.findCBSPlaylist();
  }
  if (saveButton.isSingleClick()){
    spotifyAPI.saveCurrentTrack(1);
  }
  if (saveButton.isDoubleClick()){
    spotifyAPI.saveCurrentTrack(2);
  }
  

}