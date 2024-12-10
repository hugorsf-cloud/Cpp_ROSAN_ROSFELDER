//////////////////////////////////////////////////////////////////////////////////BIBLIOTHEQUES//////////////////////////////////////////////////////////////////////////////////////////////////
#include <Wire.h>       // Bibliothèque pour la communication I2C
#include <Servo.h>      // Bibliothèque pour le servomoteur
#include "ADS1X15.h"    // Bibliothèque pour l'ADS1115
#include <string>
#include <iostream>
#include <cstdlib>      // Bibliothèque pour utilisation valeur absolue
#include <deque>        // Utilisation de la librairie STL
using namespace std;

//////////////////////////////////////////////////////////////////////////////////DEFINE/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DUREE_ACQUISITION_MS 1000   // pour la fonction millis()
#define ETAT_BAS_PANNEAU 0
#define ETAT_MOYEN_PANNEAU 90
#define ETAT_HAUT_PANNEAU 180
#define ERREUR_DATA 1   // pour gestion exception

/////////////////////////////////////////////////////////////////////////////////CODE////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADS1115 ADS(0x48);  // Adresse I2C du module ADS1115

// Classe de base pour les composants
class Composants {
  protected:
    int pinutilisee;
  public:
    Composants(int pin) : pinutilisee(pin) { }
};

// Classe pour le capteur de luminosité
class CapteurLuminosite : public Composants {
  private:
    float data;
  public:
    CapteurLuminosite(int pinuse): Composants(pinuse) { 
      // Le capteur n'est pas sur un pin physique, donc on n'utilise pas pinMode ici
    }
    // Fonction pour renvoyer les données du capteur (valeur de luminosité)
    float renvoieData() {
      int rawData = ADS.readADC(pinutilisee); // Lire la valeur brute depuis l'ADS1115 via I2C (lecture sur le canal spécifié)
      float voltage = ADS.toVoltage(rawData);  // La méthode ToVoltage effectue la conversion
      return voltage;
    }
    // Redéfinition opérateur + pour lui donner le rôle de faire une valeur absolue d'une différence de "sensordata"
    friend float operator +(CapteurLuminosite& cap1,CapteurLuminosite& cap2) {
      return fabs(cap1.renvoieData()-cap2.renvoieData());
    }
};

// Classe pour le servomoteur
class ServoMoteur : public Composants {
  private:
    float position;
    Servo myservo;
  public:
    ServoMoteur(int pinuse, int pos): Composants(pinuse), position(pos) {
      myservo.attach(pinuse);
      write(pos);
    }

    float valeurServo() {
      int val = myservo.read();  // Attribue la valeur de position à la variable val
      cout << "Servo moteur à la position : " << val << endl;
      return val;
    }

    void write(float pos) {
      this->position = pos;
      myservo.write(pos);  // Déplacer le servomoteur à la position donnée
    }
};

// Initialisation des composants
CapteurLuminosite moncapteurA0(0);               // Le capteur 1 est connecté sur le canal A0 de l'ADS1115
CapteurLuminosite moncapteurA1(1);               // Le capteur 2 est connecté sur le canal A1 de l'ADS1115
ServoMoteur monservo(D3, 0);                     // Initialisation du servomoteur sur la broche D3
long lasttime = 0;                               // Déclaration pour la fonction millis()

void setup() {
  Serial.begin(9600);                             // Initialiser la communication série
  Serial.printf("bonjour\n");
  Wire.begin();                                   // Utiliser GPIO 5 pour SDA et GPIO 4 pour SCL
  bool ret = ADS.begin();                         // Initialiser l'ADS1115
  Serial.printf("ads=%d\n", (int)ret);
  ADS.setMode(1);                                 // Définir le mode de mesure
}

void loop() {
/////////////////////////////////////////////////////////////////////////////////////////////////////CAPTEURS DE LUMINOSITE /////////////////////////////////////////////////////////////////////////////////
try{
  int sensorData1 = (moncapteurA0.renvoieData()*1000);  // On convertit en entiers (mV)
  int sensorData2 = (moncapteurA1.renvoieData()*1000);  // On convertit en entiers (mV)

  // Utilisation deque (librairie STL)
  deque<int> C1;
  deque<int> C2;
  C1.push_front(sensorData1);
  C2.push_front(sensorData2);
  
  // Fonction millis() pour remplacer les delay()
  auto now = millis();                                     
  if (now - lasttime > DUREE_ACQUISITION_MS){
    lasttime = now;
    Serial.printf("Sensor data1: %.2d mV\n", C1.front());  // Afficher la tension en mV
    Serial.printf("Sensor data2: %.2d mV\n", C2.front());  // Afficher la tension en mV
    if (C1.front() <0 || C2.front()<0){                    // Lancement exception
      throw ERREUR_DATA;
    }

    float lumiere = (moncapteurA0 + moncapteurA1)*1000;  // Utilisation de la surcharge de l'opérateur + et multiplication par 1000 pour augmenter la précision
    Serial.printf("valeur absolue: %f \n", lumiere);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////// MOTEUR /////////////////////////////////////////////////////////////////////////////////////////////////
    if (lumiere < 1000){                                // Si valeur absolue de la différence dans un certain seuil, on considère que le soleil est vers 12h
      monservo.write(ETAT_MOYEN_PANNEAU);
      Serial.printf("Position 2 du servo : %.2f \n", monservo.valeurServo());
    }
  
    else {                                              // Sinon on est soit le matin, soit le soir
      if( C1.front() < C2.front()){
        monservo.write(ETAT_HAUT_PANNEAU);            
        Serial.printf("Position 3 du servo : %.2f \n", monservo.valeurServo());
      }
      else{
        monservo.write(ETAT_BAS_PANNEAU);
        Serial.printf("Position 1 du servo : %.2f \n", monservo.valeurServo());
      }
    }
  
  }
}
catch (int e){                                      // Catch exceptions
  if (e == ERREUR_DATA){
    Serial.printf("ATTENTION DATA NEGATIVE PROBLEME");
  }
}
}
