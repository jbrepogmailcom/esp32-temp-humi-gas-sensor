# Air Sensor JB4

This project is designed to run on a Wemos D1 board and performs several functions related to environmental monitoring and control. It includes functionalities for reading sensor data, connecting to WiFi, publishing data to an MQTT server, sending events to IFTTT, and controlling a Toshiba air conditioner via IR commands.

## Features

- **Sensor Initialization and Reading**: Initializes and reads data from various sensors including MiCS-6814 (for gas detection) and BME280 (for temperature, humidity, and pressure).
- **WiFi and MQTT**: Connects to WiFi using WiFiManager and publishes sensor data to an MQTT server.
- **IFTTT Integration**: Sends events to IFTTT based on temperature and humidity thresholds.
- **Telegram Bot**: Uses a Telegram bot to send messages and receive commands, and controls a Toshiba air conditioner via IR commands.
- **OTA Updates**: Checks for firmware updates and performs OTA updates if a new version is available.
- **Miscellaneous**: Controls a humidifier based on humidity levels, logs data to ThingSpeak, and handles various hardware configurations and resets.

## Getting Started

### Prerequisites

- Arduino IDE
- Wemos D1 board
- MiCS-6814 sensor
- BME280 sensor
- Toshiba air conditioner (for IR control)
- WiFi network

### Libraries

Ensure you have the following libraries installed in your Arduino IDE:

- Ticker
- WiFiManager
- ESP8266HTTPClient
- ESP8266httpUpdate
- CTBot
- DHTesp
- MiCS6814-I2C
- ESP8266WiFi
- ESP8266WiFiMulti
- WiFiClientSecureBearSSL
- BME280I2C
- Wire
- ESP8266mDNS
- WiFiUdp
- Arduino
- PubSubClient
- IRremoteESP8266
- IRsend
- ir_Toshiba

### Installation

1. Clone this repository to your local machine.
2. Open the `air_sensor-jb4-230314-commented.ino` file in the Arduino IDE.
3. Update the following placeholders with your actual values:
   - `YOUR_IFTTT_KEY`
   - `YOUR_API_KEY`
   - `YOUR_API_KEY_GASSES`
   - `YOUR_MQTT_SERVER`
   - `YOUR_MQTT_USER`
   - `YOUR_MQTT_PASSWORD`
   - `YOUR_TELEGRAM_BOT_TOKEN`
   - `SERVER IP`
   - `DIRECTORY`
4. Connect your Wemos D1 board to your computer.
5. Select the appropriate board and port in the Arduino IDE.
6. Upload the sketch to the board.

### Usage

Once the sketch is uploaded, the device will:

1. Connect to the WiFi network.
2. Initialize the sensors and start reading data.
3. Publish sensor data to the MQTT server.
4. Send events to IFTTT based on temperature and humidity thresholds.
5. Control the Toshiba air conditioner via IR commands received from the Telegram bot.
6. Check for firmware updates and perform OTA updates if a new version is available.

### Contributing

Contributions are welcome! Please fork this repository and submit pull requests.

### License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Acknowledgments

- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)
- [ThingSpeak](https://thingspeak.com/)
- [IFTTT](https://ifttt.com/)
- [Telegram](https://telegram.org/)
