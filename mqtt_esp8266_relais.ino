/*
 ESP8266 MQTT - Déclenchement d'un relais
 Création Dominique PAUL. / Modifié par Pascal Courtonne
 Dépot Github : https://github.com/DomoticDIY/MQTT-ESP8266-Relais

 Bibliothéques nécessaires :
 - pubsubclient : https://github.com/knolleary/pubsubclient
 - ArduinoJson v5.13.3 : https://github.com/bblanchon/ArduinoJson
Télécharger les bibliothèques, puis dans IDE : Faire Croquis / inclure une bibliothéque / ajouter la bibliothèque ZIP.
Dans le gestionnaire de bibliothéque, charger le module ESP8266Wifi.

Installaer le gestionnaire de carte ESP8266 version 2.5.0 
Si besoin : URL à ajouter pour le Bord manager : http://arduino.esp8266.com/stable/package_esp8266com_index.json

Adaptation pour reconnaissance dans Domoticz :
Dans le fichier PubSubClient.h : La valeur du paramètre doit être augmentée à 512 octets. Cette définition se trouve à la ligne 26 du fichier.
Sinon cela ne fonctionne pas avec Domoticz

Pour prise en compte du matériel :
Installer si besoin le Driver USB CH340G : https://wiki.wemos.cc/downloads
dans Outils -> Type de carte : generic ESP8266 module
  Flash mode 'QIO' (régle générale, suivant votre ESP, si cela ne fonctionne pas, tester un autre mode.
  Flash size : 1M (no SPIFFS)
  Port : Le port COM de votre ESP vu par windows dans le gestionnaire de périphériques.

Vidéo : https://www.youtube.com/watch?v=6HclvzhEWMg&ab_channel=DomoticDIY

*/

// Inclure les librairies.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

String nomModule = "Module relais";     // Nom usuel de ce module.

#define RELAY_PIN 0                     // Pin sur lequel est connectée la commande du relais.

// Définitions liées à Domoticz et MQTT
// ------------------------------------------------------------
const char* mqtt_server   = "BROKER_IP";       // Adresse IP ou DNS du Broker.
const int   mqtt_port     = 1883;              // Port du Brocker MQTT
const char* mqtt_login    = "MQTT_LOGIN";      // Login de connexion à MQTT.
const char* mqtt_password = "MQTT_PASSWD";     // Mot de passe de connexion à MQTT.
char*       topicIn       = "domoticz/out";    // Nom du topic envoyé par Domoticz
char*       topicOut      = "domoticz/in";     // Nom du topic écouté par Domoticz
int         idxDevice     = 42;                // Idx du dispositif dans Domoticz

// -------------------------------------------------------------
// Définitions liées au WIFI
// -------------------------------------------------------------
const char* ssid          = "bobby-wrt";         // SSID du réseau Wifi
const char* password      = "lascap-maison";     // Mot de passe du réseau Wifi.

// ------------------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
     
  Serial.begin(115200);
  
  setup_wifi();

  client.setBufferSize(512);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  
  if (!client.connected()) {
    reconnect();
    if (!client.connected()) {
      // Pause de 5 secondes
      delay(5000);
    }    
  }
  
  client.loop();
}


void setup_wifi() {
  // Connexion au réseau Wifi

  Serial.println();
  Serial.print("Connection au réseau : ");
  Serial.println(ssid);
    
  // delay(10);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    // Tant que l'on est pas connecté, on boucle.
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("Addresse IP : ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Message reçu du Broker.
  String string;
    
  if (strcmp(topic, topicOut)) {  // Topic de Domoticz ?
    for (int i = 0; i < length; i++) {
      string+=((char)payload[i]);
    }
    
    // Parseur Json
    DynamicJsonDocument root(512);
    DeserializationError error = deserializeJson(root, string);
    
    if (! error) {
      int idx = root["idx"];
      int nvalue = root["nvalue"];
      // String dev_name = root["name"];
  
      // Activer la sortie du relais si 1a "nvalue" 1 est reçu.
      if (idx == idxDevice) {
        Serial.print("Reçu information du Device : ");
        Serial.println(idx);
        Serial.println(string);
        if(nvalue == 1) {
          digitalWrite(RELAY_PIN, LOW); 
          Serial.print("Device ");
          Serial.print(idx);
          Serial.println(" sur ON");
        } else if (nvalue == 0) {
          digitalWrite(RELAY_PIN, HIGH); 
          Serial.print("Device ");
          Serial.print(idx);
          Serial.println(" sur OFF");
        }
      }/* else {
        Serial.print("Reçu information du Device : ");
        Serial.print(idx);
        Serial.print(" / ");
        Serial.println(dev_name);
        Serial.println(string);
      }*/
    } else {
        Serial.println("Erreur de lecture du JSON !");
    }
  } /* else {
    Serial.print("Message arrivé d'un autre topic [");
    Serial.print(topic);
    Serial.println("] ");
  } */
}

void reconnect() {
  // Connexion MQTT
  if (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    
    // Initialise la séquence Random
    randomSeed(micros());
    
    // Création d'un ID client aléatoire
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    // Tentative de connexion
    if (client.connect(clientId.c_str(), mqtt_login, mqtt_password)) {
      Serial.println(" Connecté");
      
      // Connexion effectuée, publication d'un message...
      
      String message = "Connexion MQTT : \""+ nomModule + "\", ID : " + clientId + " -OK-.";
      
      DynamicJsonDocument root(256);
      
      // On renseigne les variables.
      root["command"] = "addlogmessage";
      root["message"] = message;
      
      // On sérialise la variable JSON
      String messageOut;
      if (serializeJson(root, messageOut) == 0) {
        Serial.println("Erreur lors de la création du message de connexion pour Domoticz");
      } else  {
        // Convertion du message en Char pour envoi dans les Log Domoticz.
        char messageChar[messageOut.length()+1];
        messageOut.toCharArray(messageChar,messageOut.length()+1);
        client.publish(topicOut, messageChar);
      }
        
      // On souscrit (écoute)
      client.subscribe("#");
    } else {
      Serial.print("Erreur, rc=");
      Serial.print(client.state());
    }
  }
}
