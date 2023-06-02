# CBS
An ESP8266 based Spotify Control Box

## Layout
![Alt text](./CBS_layout.jpg?raw=true "CBS Layout")

## Installation-Setup
Follow [thεse](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/) steps to get esp8266 support for the arduino ide \
Install the libraries: [ArduinoJson](https://github.com/bblanchon/ArduinoJson) and [MultiButton](https://github.com/poelstra/arduino-multi-button)

### Create a Spotify app
In order for the cbs to work you must create a Spotify app on the Spotify-developers [site](https://developer.spotify.com). After creating the app, copy your client id and client secret to the .ino file. Finally go to the redirect urls and add "cbs.local". 

### Connect to a wifi 
The ESP8266 will try to find the SSID and Password of your wifi in the EEPROM. If it can't find them in the EEPROM then it automatically opens an Access Point called 'CBS'. Go to your prefered device (phone or pc with wifi adapter) and connect to the access point. Then go to 192.168.4.1 address, where you will see a list of the available wifi. Assuming that you can see your wifi in the list, type the ssid and password of your wifi into the input fields and wait for the ESP8266 to restart and connect to your wifi. 

### Connect to your spotify account
After you give the ESP8266 access to the internet, you should give permission to the CBS spotify app to access your Spotify account. For this, type in a web browser, that's connected on the same network with the EPS8266, 'cbs.local'. You should see the hyperlinked text, 'spotify_auth'. Press the link, login to your spotify account and accept the terms of the CBS app. After that , you should be ready to use your CBS box.

## Usage
There are currently 3 buttons:
1. NextPreviousButton: This button goes to the next song with a Signle click and to the previous with a Double click
2. PlayPauseButton: This button starts or stops the playback
3. SaveButton: This button has three functions: \
    a. Single click: Saves the currently playing song to a predefined playlist (specified in the CBS library) \
    b. Double click: Saves the currently playing song to the "Liked Songs" playlist. \
    c. Long click: Clears the EEPROM

## Libraries used/Inspired by
[PinButton - poelstra](https://github.com/poelstra/arduino-multi-button) \
[Spotify-Arduino-API - witnessmenow](https://github.com/witnessmenow/spotify-api-arduino) \
[Wifi manager - Electronics Innovation](https://electronicsinnovation.com/change-esp8266-wifi-credentials-without-uploading-code-from-arduino-ide/) \
[ArduinoJson - bblanchon](https://github.com/bblanchon/ArduinoJson)
