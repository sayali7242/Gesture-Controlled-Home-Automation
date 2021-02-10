The following steps need to be followed to run the system on an Arduino Nano 33 IoT board

1) Install relevant libraries and register Arduino board on AWS IoT using the crypto element and the RSA key pair. The following tutorial can be followed for this: https://create.arduino.cc/projecthub/Arduino_Genuino/securely-connecting-an-arduino-mkr-wifi-1010-to-aws-iot-core-a9f365

2) In the secrets.h file, enter the WiFi SSID and password, as well as the endpoint and secret certificate downloaded from AWS IoT (this is also described in the above tutorial).

3) Install the ArduinoJSON library for parsing messages

4) Connect the Nano to the computer, select the correct COM port and board type, and upload the program. A flashing on-board LED signals connection to WiFi and a solid LED indicates connection to AWS via MQTT.

5) Create the button circuit by connecting a button to Pin 2 of the Nano, with a 10K resistor connecting Pin 2 to GND (can be seen in the project report).

6) Once the circuit is created, gestures can be tested on the arduino/outgoing-gyroscope topic using the Test functionality on AWS IoT

7) Follow the steps in readme_aws.txt for AWS setup and IFTTT setup to connect to smart devices