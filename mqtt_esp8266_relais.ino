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
const char* ssid          = "YOUR_WIFI_SSID";       // SSID du réseau Wifi
const char* password      = "YOUR_WIFI_PASSWD";     // Mot de passe du réseau Wifi.

// ------------------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

// SETUP
// *****
void setup() {

  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RELAY_PIN, OUTPUT);
     
  Serial.begin(115200);

  client.setBufferSize(512);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// BOUCLE DE TRAVAIL
// *****************
void loop() {

  while (WiFi.status() != WL_CONNECTED) {
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

// CONNEXION WIFI
// **************
void setup_wifi() {
  
  // Connexion au réseau Wifi

  Serial.println();
  Serial.print("Connexion au point d'accès Wifi '");
  Serial.print(ssid);
  Serial.println("'");
      
  WiFi.mode(WIFI_STA);
  // Serial.printf("Wi-Fi mode réglé sur WIFI_STA %s\n", WiFi.mode(WIFI_STA) ? "" : "Echec!");
  
  WiFi.begin(ssid, password);

  int wifi_status = WiFi.status();
  while ( wifi_status != WL_CONNECTED) {
    // Serial.printf("Connection status: %d\n", wifi_status);
    if( wifi_status == WL_NO_SSID_AVAIL) {
      Serial.println("");
      Serial.println("*** Point d'accès Wifi inaccessible");
      return;
    }
    if( wifi_status == WL_CONNECT_FAILED) {
      Serial.println("");
      Serial.println("*** Echec de la connexion Wifi");
      return;
    }
    /*
    if( wifi_status == WL_CONNECT_WRONG_PASSWORD) {
      Serial.println("");
      Serial.print("Wifi password is incorrect");
      return;
    }
    */
    // Tant que l'on est pas connecté, on boucle.
    delay(500);
    Serial.print(".");
    wifi_status = WiFi.status();
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
      String dev_name = root["name"];
  
      // Activer ou désactiver le relais en fonction de la valeur de nvalue
      if (idx == idxDevice) {
        // Serial.print("Reçu information du Device : ");
        // Serial.println(idx);
        // Serial.println(string);
        if(nvalue == 1) {
          digitalWrite(RELAY_PIN, LOW); 
          Serial.print("'");
          Serial.print(dev_name);
          Serial.println("' === ON ===");
          Serial.println("");
        } else if (nvalue == 0) {
          digitalWrite(RELAY_PIN, HIGH); 
          Serial.print("'");
          Serial.print(dev_name);
          Serial.println("' === OFF ===");
          Serial.println("");
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

// CONNEXION MQTT
// **************
void reconnect() {
  // Connexion MQTT
  if (!client.connected()) {
    
    // Initialise la séquence Random
    randomSeed(micros());

    Serial.print("Connexion au serveur MQTT...");
    
    // Création d'un ID client aléatoire
    String clientId = "RelayIOT-";
    clientId += String(random(0xffff), HEX);
    
    // Tentative de connexion
    if (client.connect(clientId.c_str(), mqtt_login, mqtt_password)) {
      Serial.println(" Connecté");
      
      // Connexion effectuée, publication d'un message...
      
      String message = "Connexion MQTT : '"+ nomModule + "', ID : " + clientId + " -OK-.";
      
      // DynamicJsonDocument root(256);
      // const int capacity = JSON_OBJECT_SIZE(2);
      // StaticJsonDocument<capacity> root;
     StaticJsonDocument<256> root;
      
      // On renseigne les variables.
      root["command"] = "addlogmessage";
      root["message"] = message;
      
      // On sérialise la variable JSON
      String messageOut;
      if (serializeJson(root, messageOut) == 0) {
        Serial.println("*** Erreur lors de la création du message de connexion pour Domoticz");
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
      Serial.println(client.state());
      Serial.println(" prochaine tentative dans 5s");
      // Pause de 5 secondes
      delay(5000);
    }
  }
}
