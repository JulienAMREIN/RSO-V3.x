// Routeur Solaire - V3 - Julien AMREIN, Licence GNU GPL V3

// La résistance testée sur le dispositif doit etre au minimum de [33 000 ohms] pour le photo-régulateur DIY
// Jour nuit sur digital 7

#include <Wire.h>                                         // Lib pour protocol I2C
#include <LiquidCrystal_I2C.h>                            // Lib pour écran LCD
#include "EmonLib.h"                                      // Lib pour la gestion de la pince ampèremétrique
#include "OneWire.h"                                      // Lib pour la gestion de la sonde DS18B20
#include "DallasTemperature.h"                            // Lib pour la gestion de la sonde DS18B20

OneWire oneWire(A3);                                      // Broche de connexion DATA de la sonde
DallasTemperature ds(&oneWire);

LiquidCrystal_I2C lcd(0x27,16,2);                         // Adresse et format de l'écran
EnergyMonitor emon1;

byte ledPin = 9;                                          // Variable pour déterminer la pin de branchement de la led en PWM
byte valeurLedDimmer = 0;                                 // Variable de la valeur de sortie sur la pin "ledPin" en PWM pour faire varier l'intensité lumineuse et piloter le dimmer de 0 à 255
byte statusCourantLed = 0;                                // 0= initial   1=était en conso EDF   2=était en injection EDF

int delayChangementEtat = 1000;                           // Temps de maintient de la luminosité de la led lord d'une bascule injection/conso et conso/injection
byte valeurMaximumLed = 60;                               // Variable pour définir l'amplitude maximum de la lumière de la led qui pilote le dimmer
byte valeurIncrementationLed = 1;                         // Le pas d'incrémentation pour augmenter la luminosité de la LED et se rapprocher du seuil consomation depuis EDF
byte valeurDecrementationLed = 1;                         // Le pas de décrémentation pour diminuer la luminosité de la LED et stopper rapidement la consomation depuis EDF

byte maxTemp = 65;                                        // Température de sécurité max pour couper la chauffe
byte medTemp = 30;                                        // Température de chauffe à atteindre en heure creuse au minimum en mode "complément HC"
int tSTORE = 0;                                           // Variable de stockage de la température précédente enregistrée pour comparer au nouveau relevé
int t = 0;                                                // Variable de stockage de la température relevée

byte brocheBoutonAssist = 7;                              // Broche utilisée pour détecter le sélecteur de marche Hiver, jour/nuit
int etatBoutonAssist = HIGH;                              // Etat du sélecteur de marche Hiver, jour/nuit

void setup()
{

  pinMode(brocheBoutonAssist, INPUT);                     // Déclaration de la broche du sélecteur de marche Hiver, jour/nuit

  lcd.init();                       
  lcd.backlight();

//----------------------------------------------------------- Affichage ECRAN 1
  affichageVersion();                                      // Ligne du haut et du bas
  delay(3000);                                             // Durée de l'affichage

  affichageDeveloppeur();                                  // Ligne du bas
  delay(3000);                                             // Durée de l'affichage

//----------------------------------------------------------- Affichage ECRAN 2
  affichageConfiguration();                                // Ligne du haut et du bas
  delay(3000);                                             // Durée de l'affichage

//----------------------------------------------------------- Affichage ECRAN 3
  lcd.setCursor(0,0);                                      // Bloc d'affichage initial avant apparition de "consommation ou injection"
  lcd.print("Initialisation..");
  lcd.setCursor(0,1);                                      // Affichage initial de PWR et TMP
  lcd.print("Pwr:    Tmp:    ");                           // Affichage initial de PWR et TMP

  emon1.voltage(2, 300, 1.7);                              // Tension: input pin, calibration, phase_shift
  emon1.current(1, 57);                                    // Courrant: input pin, calibration.

  ds.begin();                                              // sonde DS18B20 activée

}

//----------------------------------------------------------- Début de la boucle principale
void loop()
{
//----------------------------------------------------------- Calcul de température
  ds.requestTemperatures();
  t = ds.getTempCByIndex(0);                              // Affecter à t la valeur de la température relevée

//----------------------------------------------------------- TRAITEMENT DU RETOUR DE SONDE
  traitementErreurSonde();                                // Erreur sonde NO SENSOR
  majAffichageTemperature();                              // MAJ affichage de la température si changement de sa valeur

  // TRAITEMENT 3: Température trop élevée
  if(t >= maxTemp)                                        // Si la température est supérieur à maxTemp degrés alors
    {                                                     // afficher sur lcd MAX et valeur
    lcd.setCursor(8,1);
    lcd.print("MAX:    ");
    lcd.setCursor(13,1);
    lcd.print(t);

    valeurLedDimmer = 0;
    analogWrite(ledPin, valeurLedDimmer);                 // Puissance led dans le dimmer à 0 pour arret de chauffe
    lcd.setCursor(5,1);
    lcd.print("   ");                                     // Effacer la valeur de power sur l'écran LCD
    lcd.setCursor(5,1);
    lcd.print(valeurLedDimmer);                           // Ecrire 0 pour la valeur de power sur l'écran LCD

      while(t >= maxTemp)                                 // Boucle de sécurité pour mettre en pause la chauffe et relever la température
        {
        ds.requestTemperatures();                         // Reprise de la température pour confirmer
        t = ds.getTempCByIndex(0);                        // Reprise de la température pour confirmer

          if(t != tSTORE)                                 // Si la température relevée à évoluée
            {
            lcd.setCursor(12,1);
            lcd.print("    ");                            // Mettre à jour la valeur affichée sur l'écran LCD
            lcd.setCursor(13,1);
            lcd.print(t);                                 
            tSTORE = t;                                   // Mettre à jour la valeur de t dans tSTORE
            }

//----------------------------------------------------------- Temporisation avec affichage sur écran LCD

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print("   Surchauffe   ");

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print("  *Surchauffe*  ");

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print(" * Surchauffe * ");

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print("*  Surchauffe  *");

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print(" * Surchauffe * ");

        delay(1000);                                      // Boucle de pause en attendant une redescente de la température
        lcd.setCursor(0,0);                               // Bloc d'affichage de la sécurité de chauffe
        lcd.print("  *Surchauffe*  ");        
        }
        
    lcd.setCursor(0,0);                                   // Bloc d'affichage de la sécurité de chauffe
    lcd.print("Sortie surchauf.");
    lcd.setCursor(0,1);                                   // Affichage initial de PWR et TMP pour la  sortie de boucle
    lcd.print("Pwr:    Tmp:    ");                        // Affichage initial de PWR et TMP pour la  sortie de boucle
    }

  // TRAITEMENT 4: CHAUFFE A LA TEMPERATURE MINIMUM EN HEURE CREUSE

  etatBoutonAssist = digitalRead(brocheBoutonAssist);     // ici lire l'état du bouton sélecteur de mode hiver

  if((t < medTemp) && (t > 0) && (etatBoutonAssist == LOW))// Si une sonde est connectée et si la température est inférieur au mini et si mode hiver enclenché alors
    {
    analogWrite(ledPin, valeurMaximumLed);                // Puissance de chauffe maximum
    lcd.setCursor(5,1);                                   // Affichage sur ecran LCD de power au maximum
    lcd.print("   ");
    lcd.setCursor(5,1);
    lcd.print(valeurMaximumLed);

    while((t < medTemp) && (t > 0) && (etatBoutonAssist == LOW))
      {
        //----------------------------------------------------------- Temporisation avec affichage sur écran LCD

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print("   Mode Hiver   ");

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print("  *Mode Hiver*  ");

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print(" * Mode Hiver * ");

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print("*  Mode Hiver  *");

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print(" * Mode Hiver * ");

      delay(1000);                                         // Boucle de pause en attendant une remontée de la température
      lcd.setCursor(0,0);                                  // Bloc d'affichage de la sécurité de chauffe
      lcd.print("  *Mode Hiver*  ");

      etatBoutonAssist = digitalRead(brocheBoutonAssist);  // ici lire l'état du bouton sélecteur de mode hiver
      ds.requestTemperatures();                            // Reprise de la température pour confirmer
      t = ds.getTempCByIndex(0);                           // Reprise de la température pour confirmer

          if(t != tSTORE)                                  // Si la température relevée à évoluée
            {
            lcd.setCursor(12,1);
            lcd.print("    ");                             // Mettre à jour la valeur affichée sur l'écran LCD
            lcd.setCursor(13,1);
            lcd.print(t);                                 
            tSTORE = t;                                    // Mettre à jour la valeur de t dans tSTORE
            }

      }
    valeurLedDimmer = 0;
    analogWrite(ledPin, valeurLedDimmer);               // Puissance de chauffe à 0 pour fin de mode hiver
    lcd.setCursor(5,1);                                 // Affichage sur ecran LCD de power à 0
    lcd.print("   ");
    lcd.setCursor(5,1);
    lcd.print(valeurLedDimmer);

    lcd.setCursor(0,0);                                // Bloc d'affichage du hautde sortie du mode hiver
    lcd.print("Mode Hiver FINI ");
  }

//----------------------------------------------------------- Calculs EmonLib

  emon1.calcVI(20,500);                                   // Calculate all. No.of half wavelengths (crossings), time-out

  float realPower       = emon1.realPower;                //extract Real Power into variable
  float apparentPower   = emon1.apparentPower;            //extract Apparent Power into variable
  float powerFActor     = emon1.powerFactor;              //extract Power Factor into Variable
  float supplyVoltage   = emon1.Vrms;                     //extract Vrms into Variable
  float Irms            = emon1.Irms;                     //extract Irms into Variable

  // On Consomme de l'électricité------------------------------------------------------------------------------------------------------------

  if (realPower > 0)                                     // Si on consomme du courant depuis EDF
  {

    if(statusCourantLed == 2)                            // Et si avant on injectai de l'électricité depuis EDF
    {                                                    // Alors
      lcd.setCursor(0,0);
      lcd.print(">>> CONSOMMATION");
    }
    
    if(valeurLedDimmer > 0)                              // Si on consomme du courant depuis EDF Et si la LED du dimmer est allumée
    {                                                    // ALORS
      valeurLedDimmer -= valeurDecrementationLed;        // On diminue la valeur de la variable de luminosité de la led qui controle le dimmer d'une valeur de variable décrémentation

      if(valeurLedDimmer < 0)                            // On refuse une valeur négative pour la variable "valeurLedDimmer" en attribuant la valeur 0 le cas echeant.
      {
        valeurLedDimmer = 0;
      }
      analogWrite(ledPin, valeurLedDimmer);              // Et donc on fait diminuer la puissance autorisée dans le dimmer en baissant la lumière émise par la LED
      lcd.setCursor(5,1);
      lcd.print("   ");
      lcd.setCursor(5,1);
      lcd.print(valeurLedDimmer);
    }

    if(valeurLedDimmer == 0)                             // Si on consomme du courant depuis EDF Et si la LED du dimmer est éteinte
    {                                                    // ALORS
      lcd.setCursor(5,1);
      lcd.print("   ");
      lcd.setCursor(5,1);
      lcd.print(valeurLedDimmer);
      
      delay(2000);                                       // on attends X secondes avant de continuer dans le code
    }

    statusCourantLed = 1;                                // Attribution de la valeur d'état précédent     0= initial   1=était en conso EDF   2=était en injection EDF
  }

  // On Injecte de l'électricité------------------------------------------------------------------------------------------------------------

  if (realPower < 0)                                     // Si on injecte du courant vers EDF
  {

    if(statusCourantLed == 1)                            // Et si avant on consommait de l'électricité depuis EDF
    {                                                    // Alors
      lcd.setCursor(0,0);
      lcd.print("INJECTION >>>   ");

      delay(delayChangementEtat);                        // on reste X secondes avant de continuer dans le code
    }

    if(valeurLedDimmer == valeurMaximumLed)              // Si on injecte du courant vers EDF Et si on est arrivé à la valeurMaximumLed
    {                                                    // Alors
      delay(2000);                                       // on attends X secondes avant de continuer dans le code
    } 
    
    if(valeurLedDimmer < valeurMaximumLed)               // Si on injecte du courant vers EDF Et si on est inferieur à la valeurMaximumLed
    {
      valeurLedDimmer += valeurIncrementationLed;        // On augmente la valeur de la variable de luminosité de la led qui controle le dimmer d'une valeur de variable incrémentation
      analogWrite(ledPin, valeurLedDimmer);              // Et donc on fait augmenter la puissance autorisée dans le dimmer en montant la lumière émise par la LED
      lcd.setCursor(5,1);
      lcd.print("   ");
      lcd.setCursor(5,1);
      lcd.print(valeurLedDimmer);
    }

    statusCourantLed = 2;                                // Attribution de la valeur d'état précédent     0= initial   1=était en conso EDF   2=était en injection EDF
  }

  // On est neutre en conso. ------------------------------------------------------------------------------------------------------------

  if (realPower == 0)                                    // Si on est neutre sur la consommation EDF
  {                                                      // cela signifie qu'on est au point d'équilibre injection / consomation
    lcd.setCursor(1,0);
    lcd.print(" >>> NEUTRE <<< ");
    valeurLedDimmer -= valeurDecrementationLed;          // On diminue la luminosité de la led qui controle le dimmer de X crans pour temporiser les variations densoleillement
    if(valeurLedDimmer < 0)                              // On refuse une valeur négative,en attribuant la valeur 0 le cas echeant.
    {
      valeurLedDimmer = 0;
    }
    analogWrite(ledPin, valeurLedDimmer);                // Et donc on fait diminuer la puissance autorisée dans le dimmer en baissant la lumière émise par la LED
    lcd.setCursor(5,1);
    lcd.print("   ");
    lcd.setCursor(0,5);
    lcd.print(valeurLedDimmer);
    delay(2000);                                         // Puis on fige le système pendant X secondes pour limiter les variations intempestives (A tester pour validation)
  }

}


  //------------------------------------------------------------------------------------------------------------
  //                              Les fonctions isolées du code sont ci-dessous
  //------------------------------------------------------------------------------------------------------------
  /*
        Liste des fonctions:

          1- affichageVersion() - Affichage initial
          2- Affichage Mode hiver
          3- Affichage Surchauffe
          4-
  */

//----------------------------------------------------------------------------------
void affichageVersion(){
  lcd.setCursor(0,0);
  lcd.print("Code V11-11-2024");
  lcd.setCursor(0,1);
  lcd.print("PCB Mod.   BLACK");
}

//----------------------------------------------------------------------------------
void affichageDeveloppeur() {
    const char* message = "By Julien AMREIN";

    // Efface la ligne entière avant l'affichage
    lcd.setCursor(0, 1);
    lcd.print("                ");

    // Affiche chaque caractère avec un délai entre eux
    for (int i = 0; message[i] != '\0'; i++) {
        lcd.setCursor(i, 1);
        lcd.print(message[i]);
        delay(150);
    }
}

//----------------------------------------------------------------------------------
void affichageConfiguration(){
  lcd.setCursor(0,0);                                      // Bloc d'affichage de la config
  lcd.print("Configuration : ");
  lcd.setCursor(0,1);
  lcd.print("Max:    Min:    ");
  lcd.setCursor(5,1);
  lcd.print(maxTemp);                                      // Affichage de la température maximum programmée
  lcd.setCursor(13,1);
  lcd.print(medTemp);                                      // Affichage de la température minimum heure creuse programmée
  }
//----------------------------------------------------------------------------------
  void  traitementErreurSonde(){
    if(t == -127)                                           // Si la SONDE EST ABSENTE, (sans sonde il s'affiche -127)
    {
      lcd.setCursor(8,1);                                   // afficher un message de défaut sur lcd 
      lcd.print("NoSensor");
      tSTORE = 0;                                           // Mettre à jour la valeur de t dans tSTORE
    }
  }
//----------------------------------------------------------------------------------
  void majAffichageTemperature(){
    if((t > 0) && (t < 110) && (tSTORE != t))               // Si la SONDE RETOURNE UNE TEMPERATURE COHERENTE et DIFFERENTE de l'affichage précédent, afficher la température sur lcd
    {
     lcd.setCursor(8,1);
     lcd.print("Tmp:    ");
     lcd.setCursor(13,1);
     lcd.print(t);
     tSTORE = t;                                           // Mettre à jour la valeur de t dans tSTORE
    }
  }








