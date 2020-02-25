# homeAutomation
HomeAutomation with Node-red Dashboard.
This code works on ESP-01 and ESP8266 Nodemcu.
----------------------------------------------------------------------------------------------------------------------------------------
Components and Connection.

1. ESP8266

2. DHT11

3. RELAY

4. PUSH BUTTON

Data Pin of DHT11 is connected to pin D3 of ESP and then VCC and GND are given to 3v and G(ground) respectively. The purpose of DHT is to give Temerature and Humidity of the Room or environment.

The IN pin of the relay is connected to D2 of ESP and then VCC and GND are given to 3v and G(ground) respectively, Note if you are using a 5v relay, it will have to be connected to the 5v pin of ESP. The purpose of the relay is to unable us to switch on light, fans, AC's and many more remotely.

One pin of the push button is connected to D5 and the other to G(ground), the purpose of the push button is to help with connecting to different networks(WiFi) and changing configuration parameters such as Mqtt ports and node-red address.



Steps to making the code work for you 
1. Clone the repository.
2. if you are using platformIO, open the whole folder with your prefered code editor, i used visual studio code.
3. if you are using arduino ide, copy the code inside the main.cpp file into your arduino ide file.
3. install the necessary header files(libraries) "you wont be able to compile without them".
4. compile and then upload.
5. start node-red from terminal if not installed alredy install from https://nodered.org/docs/getting-started/local 
6. install mqtt from http://www.bytesofgigabytes.com/mqtt/installing-mqtt-broker-on-windows/, this is responsible for communication        between  ESP Device and Node-red.
7. Open the MQTT port by doing http://www.bytesofgigabytes.com/networking/how-to-open-port-in-windows/
8. On node-red click on the Humburger menu and then click on manage palette, it will open some sort of a side menu.
9. Click on install and search for node-red-dashboard and then install.
10. from the same hunburger menu click on import, here you will need to upload the node-red file in the node-red folder.
 

