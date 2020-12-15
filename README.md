## MQTT-ESP8266-Relais
Action d'un relais via MQTT depuis Domoticz

But : Commander un relais MQTT depuis Domoticz.

L'intéret est de pouvoir déclencher un relais à distance par WIFI.

Tout est possible, la consommation de l'appareil commandé ne doit pas dépasser les spécifications inscrites sur le relais commandé.

#Liens utiles

Logiciel  :

- Driver USB CH340G : https://wiki.wemos.cc/downloads 
- Logiciel Arduino IDE : https://www.arduino.cc/en/Main/Software 
- URL à ajouter pour le Board manager : http://arduino.esp8266.com/stable/package_esp8266com_index.json 

Installer la prise en charge des cartes ESP8266

Bibliothéques : 
 - pubsubclient : https://github.com/knolleary/pubsubclient 
 - ArduinoJson v5.13.3 : https://github.com/bblanchon/ArduinoJson 
 
Dans IDE : Faire Croquis / inclure une bibliothéque / ajouter la bibliothèque ZIP. 

Vidéo explicative sur YouTube : https://www.youtube.com/watch?v=6HclvzhEWMg
