#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_LSM6DS3.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>

// ============================================================
// PINS
// ============================================================
#define EN_D       23
#define EN_G        4
#define IN_1_D     19
#define IN_2_D     18
#define IN_1_G     17
#define IN_2_G     16
#define ENC_G_CH_A 32
#define ENC_G_CH_B 33
#define ENC_D_CH_A 27
#define ENC_D_CH_B 14

// ============================================================
// PWM
// ============================================================
#define CH_IN1_D 0
#define CH_IN2_D 1
#define CH_IN1_G 2
#define CH_IN2_G 3
#define PWM_FREQ 1000
#define PWM_RES  8

// ============================================================
// PARAMETRES DU ROBOT
// ============================================================
float MM_PAR_IMP_D = 0.1389f;
float MM_PAR_IMP_G = 0.1389f;
#define VITESSE_BASE 80
#define VITESSE_TOURNER 125
float ENTRAXE_MM   = 114.75f;

// Reglages PID pour garder les roues synchronisees
float KP_ROUES = 2.0f;
float KI_ROUES = 0.004f;
float KD_ROUES = 0.45f;

// Position du stylo par rapport aux roues
#define OFFSET_STYLO_ODOMETRIE_MM 130.0f

float rayonCercleCm = 13.5f;
#define OFFSET_STYLO_CERCLE_MM 135.0f
#define ENTRAXE_CERCLE_MM 90.0f
float normaliserRayonCercle(float rayonCm);

// ============================================================
// WIFI
// ============================================================
const char* ssid     = "Drawbot";
const char* password = "drawbot123";
WebServer serveur(80);

// ============================================================
// ENCODEURS
// ============================================================
volatile long compteurG = 0;
volatile long compteurD = 0;

volatile long totalCompteurG = 0;
volatile long totalCompteurD = 0;
volatile int signeEncodeurG = 1;
volatile int signeEncodeurD = 1;

void IRAM_ATTR isr_enc_G() {
  compteurG++;
  totalCompteurG += signeEncodeurG;
}
void IRAM_ATTR isr_enc_D() {
  compteurD++;
  totalCompteurD += signeEncodeurD;
}

// ============================================================
// DONNEES DES CAPTEURS
// ============================================================
Adafruit_LSM6DS3 imu;
Adafruit_LIS3MDL mag;

bool imuOK = false;
bool magOK = false;

float accX = 0, accY = 0, accZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;
float magX = 0, magY = 0, magZ = 0;
float capDeg = 0;
float orientationGyroDeg = 0;

float vitesseRoueG_cm_s = 0;
float vitesseRoueD_cm_s = 0;

float posX_mm = 0;
float posY_mm = 0;
float thetaRad = 0;
float styloX_mm = OFFSET_STYLO_ODOMETRIE_MM;
float styloY_mm = 0;

long dernierTotalG = 0;
long dernierTotalD = 0;
unsigned long dernierTempsOdo = 0;
unsigned long dernierTempsIMU = 0;

float distanceCmdMm = 200.0f;
float angleCmdDeg = 90.0f;

String etatOuiNon(bool etat) {
  return etat ? "OK" : "NON DETECTE";
}

struct PIDSimple {
  float kp;
  float ki;
  float kd;
  float somme;
  float derniereErreur;
  float limiteSomme;
};

void resetPID(PIDSimple &pid) {
  pid.somme = 0.0f;
  pid.derniereErreur = 0.0f;
}

float calculerPID(PIDSimple &pid, float erreur) {
  pid.somme += erreur;
  pid.somme = constrain(pid.somme, -pid.limiteSomme, pid.limiteSomme);

  float derivee = erreur - pid.derniereErreur;
  pid.derniereErreur = erreur;

  return pid.kp * erreur + pid.ki * pid.somme + pid.kd * derivee;
}

void mettreAJourCapteursEtOdometrie() {
  unsigned long maintenant = millis();
  float dtIMU = 0.0f;
  if (dernierTempsIMU == 0) {
    dernierTempsIMU = maintenant;
  } else {
    dtIMU = (maintenant - dernierTempsIMU) / 1000.0f;
    dernierTempsIMU = maintenant;
  }

  if (dernierTempsOdo == 0) {
    dernierTempsOdo = maintenant;
    dernierTotalG = totalCompteurG;
    dernierTotalD = totalCompteurD;
    return;
  }

  float dt = (maintenant - dernierTempsOdo) / 1000.0f;
  if (dt >= 0.05f) {
    long totalGLocal = totalCompteurG;
    long totalDLocal = totalCompteurD;
    long deltaG = totalGLocal - dernierTotalG;
    long deltaD = totalDLocal - dernierTotalD;

    float distG = deltaG * MM_PAR_IMP_G;
    float distD = deltaD * MM_PAR_IMP_D;

    vitesseRoueG_cm_s = (distG / 10.0f) / dt;
    vitesseRoueD_cm_s = (distD / 10.0f) / dt;

    float distCentre = (distD + distG) / 2.0f;
    float dTheta = (distD - distG) / ENTRAXE_MM;
    thetaRad += dTheta;
    posX_mm += distCentre * cos(thetaRad);
    posY_mm += distCentre * sin(thetaRad);
    styloX_mm = posX_mm + OFFSET_STYLO_ODOMETRIE_MM * cos(thetaRad);
    styloY_mm = posY_mm + OFFSET_STYLO_ODOMETRIE_MM * sin(thetaRad);

    dernierTotalG = totalGLocal;
    dernierTotalD = totalDLocal;
    dernierTempsOdo = maintenant;
  }

  if (imuOK) {
    sensors_event_t accel, gyro, temp;
    imu.getEvent(&accel, &gyro, &temp);
    accX = accel.acceleration.x;
    accY = accel.acceleration.y;
    accZ = accel.acceleration.z;
    gyroX = gyro.gyro.x;
    gyroY = gyro.gyro.y;
    gyroZ = gyro.gyro.z;
    orientationGyroDeg += gyroZ * dtIMU * 180.0f / PI;
  }

  if (magOK) {
    sensors_event_t event;
    mag.getEvent(&event);
    magX = event.magnetic.x;
    magY = event.magnetic.y;
    magZ = event.magnetic.z;
    capDeg = atan2(magY, magX) * 180.0f / PI;
    if (capDeg < 0) capDeg += 360.0f;
  }
}

// ============================================================
// ACTIONS A LANCER
// ============================================================
volatile int actionAFaire = 0;
// 0  = rien
// 1  = escalier robot
// 2  = test avancer 20cm
// 3  = test avancer 40cm (2x20cm)
// 4  = test tourner 90 deg gauche
// 5  = test tourner 90 deg droite
// 6  = test avancer 10cm (2x5cm)
// 7  = escalier stylo
// 8  = test tourner stylo 90 deg gauche
// 9  = test tourner stylo 90 deg droite

// ============================================================
// MOTEURS
// ============================================================
void setMoteurDroit(int vitesse, bool avant) {
  signeEncodeurD = avant ? -1 : 1;
  if (avant) { ledcWrite(CH_IN1_D, vitesse); ledcWrite(CH_IN2_D, 0); }
  else        { ledcWrite(CH_IN1_D, 0);       ledcWrite(CH_IN2_D, vitesse); }
}
void setMoteurGauche(int vitesse, bool avant) {
  signeEncodeurG = avant ? 1 : -1;
  if (avant) { ledcWrite(CH_IN1_G, vitesse); ledcWrite(CH_IN2_G, 0); }
  else        { ledcWrite(CH_IN1_G, 0);       ledcWrite(CH_IN2_G, vitesse); }
}
void arreter() {
  ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0);
  ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0);
}
void freiner() {
  arreter();
  delay(80);
}

// ============================================================
// DEPLACEMENT EN LIGNE DROITE
// ============================================================
void avancer(float distanceMm) {
  long ciblesD = (long)(distanceMm / MM_PAR_IMP_D);
  long ciblesG = (long)(distanceMm / MM_PAR_IMP_G);

  float ratio = distanceMm / 200.0f;
  long compensationD = (long)(125.0f + 28.0f * ratio);
  long compensationG = (long)(125.0f + 28.0f * ratio);

  long arretD = ciblesD - compensationD;
  long arretG = ciblesG - compensationG;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- AVANCER ---");
  Serial.print("Distance demandee : "); Serial.print(distanceMm / 10.0f); Serial.println(" cm");
  Serial.print("Impulsions arret D : "); Serial.println(arretD);
  Serial.print("Compensation : "); Serial.println(compensationD);

  setMoteurDroit(VITESSE_BASE, false);
  setMoteurGauche(VITESSE_BASE, true);

  while (true) {
    bool dFini = (compteurD >= arretD);
    bool gFini = (compteurG >= arretG);
    if (dFini && gFini) break;

    if (!dFini && !gFini) {
      long diff = (long)(compteurG * MM_PAR_IMP_G) - (long)(compteurD * MM_PAR_IMP_D);
      int correctionG = VITESSE_BASE - (int)(diff * 2);
      int correctionD = VITESSE_BASE + (int)(diff * 2);
      correctionG = constrain(correctionG, 60, 200);
      correctionD = constrain(correctionD, 60, 200);
      setMoteurGauche(correctionG, true);
      setMoteurDroit(correctionD, false);
    } else {
      if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    }
    delay(5);
  }

  freiner();

  Serial.print("Impulsions reelles D : "); Serial.println(compteurD);
  Serial.print("Impulsions reelles G : "); Serial.println(compteurG);
  Serial.print("Distance reelle D : "); Serial.print(compteurD * MM_PAR_IMP_D / 10.0f); Serial.println(" cm");
  Serial.print("Distance reelle G : "); Serial.print(compteurG * MM_PAR_IMP_G / 10.0f); Serial.println(" cm");
  Serial.println("---------------");
}

// ============================================================
// RECULER
// ============================================================
void reculer(float distanceMm) {
  distanceMm = abs(distanceMm);

  long ciblesD = (long)(distanceMm / MM_PAR_IMP_D);
  long ciblesG = (long)(distanceMm / MM_PAR_IMP_G);
  long arretD = ciblesD - 120;
  long arretG = ciblesG - 120;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- RECULER ---");
  setMoteurDroit(VITESSE_BASE, true);
  setMoteurGauche(VITESSE_BASE, false);

  unsigned long debut = millis();
  while (true) {
    bool dFini = compteurD >= arretD;
    bool gFini = compteurG >= arretG;
    if (dFini && gFini) break;
    if (millis() - debut > 12000) {
      Serial.println("TIMEOUT RECULER");
      break;
    }

    if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
    if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    delay(5);
  }

  freiner();
}

// ============================================================
// Rotation autour du milieu des roues
// angle positif : gauche | angle negatif : droite
// ============================================================
void tourner(float angleDeg) {
  float arcMm = (abs(angleDeg) / 360.0f) * PI * ENTRAXE_MM;
  long ciblesD = (long)(arcMm / MM_PAR_IMP_D);
  long ciblesG = (long)(arcMm / MM_PAR_IMP_G);

  long compensation = 115;
  long arretD = ciblesD - compensation;
  long arretG = ciblesG - compensation;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- TOURNER ---");
  Serial.print("Angle demande : "); Serial.print(angleDeg); Serial.println(" deg");
  Serial.print("Impulsions arret D : "); Serial.println(arretD);

  if (angleDeg > 0) {
    setMoteurDroit(VITESSE_TOURNER, false);
    setMoteurGauche(VITESSE_TOURNER, false);
  } else {
    setMoteurDroit(VITESSE_TOURNER, true);
    setMoteurGauche(VITESSE_TOURNER, true);
  }

  unsigned long debut = millis();
  while (true) {
    bool dFini = (compteurD >= arretD);
    bool gFini = (compteurG >= arretG);
    if (dFini && gFini) break;
    if (millis() - debut > 6000) {
      Serial.println("TIMEOUT TOURNER");
      break;
    }
    if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
    if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    delay(5);
  }

  freiner();

  Serial.print("Impulsions reelles D : "); Serial.println(compteurD);
  Serial.print("Impulsions reelles G : "); Serial.println(compteurG);
  Serial.println("---------------");
}

// ============================================================
// TESTS CALIBRATION AVEC PID
// ============================================================
void avancerCalibrationPID(float distanceMm) {
  long ciblesD = (long)(distanceMm / MM_PAR_IMP_D);
  long ciblesG = (long)(distanceMm / MM_PAR_IMP_G);

  float ratio = distanceMm / 200.0f;
  long compensationD = (long)(125.0f + 28.0f * ratio);
  long compensationG = (long)(125.0f + 28.0f * ratio);

  long arretD = ciblesD - compensationD;
  long arretG = ciblesG - compensationG;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- AVANCER CALIBRATION PID ---");
  Serial.print("Distance demandee : "); Serial.print(distanceMm / 10.0f); Serial.println(" cm");

  setMoteurDroit(VITESSE_BASE, false);
  setMoteurGauche(VITESSE_BASE, true);

  PIDSimple pidRoues = {KP_ROUES, KI_ROUES, KD_ROUES, 0.0f, 0.0f, 250.0f};
  resetPID(pidRoues);

  while (true) {
    bool dFini = compteurD >= arretD;
    bool gFini = compteurG >= arretG;
    if (dFini && gFini) break;

    if (!dFini && !gFini) {
      float distG = compteurG * MM_PAR_IMP_G;
      float distD = compteurD * MM_PAR_IMP_D;
      float erreur = distG - distD;
      int correction = constrain((int)calculerPID(pidRoues, erreur), -45, 45);
      int vitesseG = constrain(VITESSE_BASE - correction, 60, 200);
      int vitesseD = constrain(VITESSE_BASE + correction, 60, 200);

      setMoteurGauche(vitesseG, true);
      setMoteurDroit(vitesseD, false);
    } else {
      if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    }

    delay(5);
  }

  freiner();
}

void reculerCalibrationPID(float distanceMm) {
  distanceMm = abs(distanceMm);

  long ciblesD = (long)(distanceMm / MM_PAR_IMP_D);
  long ciblesG = (long)(distanceMm / MM_PAR_IMP_G);
  long arretD = ciblesD - 120;
  long arretG = ciblesG - 120;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- RECULER CALIBRATION PID ---");
  setMoteurDroit(VITESSE_BASE, true);
  setMoteurGauche(VITESSE_BASE, false);

  PIDSimple pidRoues = {KP_ROUES, KI_ROUES, KD_ROUES, 0.0f, 0.0f, 250.0f};
  resetPID(pidRoues);

  unsigned long debut = millis();
  while (true) {
    bool dFini = compteurD >= arretD;
    bool gFini = compteurG >= arretG;
    if (dFini && gFini) break;
    if (millis() - debut > 12000) {
      Serial.println("TIMEOUT RECULER PID");
      break;
    }

    if (!dFini && !gFini) {
      float distG = compteurG * MM_PAR_IMP_G;
      float distD = compteurD * MM_PAR_IMP_D;
      float erreur = distG - distD;
      int correction = constrain((int)calculerPID(pidRoues, erreur), -45, 45);
      int vitesseG = constrain(VITESSE_BASE - correction, 60, 200);
      int vitesseD = constrain(VITESSE_BASE + correction, 60, 200);

      setMoteurGauche(vitesseG, false);
      setMoteurDroit(vitesseD, true);
    } else {
      if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    }

    delay(5);
  }

  freiner();
}

void tournerCalibrationPID(float angleDeg) {
  float arcMm = (abs(angleDeg) / 360.0f) * PI * ENTRAXE_MM;
  long ciblesD = (long)(arcMm / MM_PAR_IMP_D);
  long ciblesG = (long)(arcMm / MM_PAR_IMP_G);

  long compensation = 115;
  long arretD = ciblesD - compensation;
  long arretG = ciblesG - compensation;
  if (arretD < 0) arretD = ciblesD;
  if (arretG < 0) arretG = ciblesG;

  compteurG = 0;
  compteurD = 0;

  Serial.println("--- TOURNER CALIBRATION PID ---");

  bool sensD = (angleDeg < 0);
  bool sensG = (angleDeg < 0);

  if (angleDeg > 0) {
    sensD = false;
    sensG = false;
  }

  setMoteurDroit(VITESSE_TOURNER, sensD);
  setMoteurGauche(VITESSE_TOURNER, sensG);

  PIDSimple pidRoues = {KP_ROUES, KI_ROUES, KD_ROUES, 0.0f, 0.0f, 250.0f};
  resetPID(pidRoues);

  unsigned long debut = millis();
  while (true) {
    bool dFini = compteurD >= arretD;
    bool gFini = compteurG >= arretG;
    if (dFini && gFini) break;
    if (millis() - debut > 6000) {
      Serial.println("TIMEOUT TOURNER PID");
      break;
    }

    if (!dFini && !gFini) {
      float distG = compteurG * MM_PAR_IMP_G;
      float distD = compteurD * MM_PAR_IMP_D;
      float erreur = distG - distD;
      int correction = constrain((int)calculerPID(pidRoues, erreur), -45, 45);
      int vitesseG = constrain(VITESSE_TOURNER - correction, 70, 200);
      int vitesseD = constrain(VITESSE_TOURNER + correction, 70, 200);

      setMoteurGauche(vitesseG, sensG);
      setMoteurDroit(vitesseD, sensD);
    } else {
      if (dFini) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      if (gFini) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
    }

    delay(5);
  }

  freiner();
}

// ============================================================
// Rotation autour du stylo
// angle positif : gauche | angle negatif : droite
// ============================================================
void tournerStylo(float angleDeg) {
  Serial.println("--- TOURNER STYLO ---");
  int vitesseStylo = 150;
  // Petits pas pour garder le stylo presque fixe.
  // Pour 10cm : 19 * (10/7.5) = 25 cycles
  int nbCycles = 25;

  for (int i = 0; i < nbCycles; i++) {
    compteurD = 0;
    compteurG = 0;

    if (angleDeg > 0) {
      // Roue droite plus rapide, roue gauche plus lente.
      setMoteurDroit(vitesseStylo, false);
      setMoteurGauche(vitesseStylo, false);
      while (compteurD < 2 || compteurG < 1) {
        if (compteurD >= 2) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
        if (compteurG >= 1) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
      }
    } else {
      // Roue gauche plus rapide, roue droite plus lente.
      setMoteurGauche(vitesseStylo, true);
      setMoteurDroit(vitesseStylo, true);
      while (compteurG < 2 || compteurD < 1) {
        if (compteurG >= 2) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
        if (compteurD >= 1) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      }
    }

    arreter();
    delay(5);
  }

  freiner();
  Serial.println("--- FIN TOURNER STYLO ---");
}
// ============================================================
// CSS COMMUN
// ============================================================
String cssCommun() {
  String css = "<style>";
  css += "body{font-family:Arial,sans-serif;text-align:center;background:#1a1a2e;color:white;margin:0;padding:30px;}";
  css += "h1{color:#00d4aa;font-size:2em;margin-bottom:5px;}";
  css += "h2{color:#00d4aa;font-size:1.4em;margin-bottom:20px;}";
  css += "p{color:#aaa;margin-bottom:20px;}";
  css += ".btn{display:block;width:80%;max-width:400px;margin:12px auto;padding:18px;";
  css += "font-size:1.1em;border:none;border-radius:12px;cursor:pointer;";
  css += "color:white;font-weight:bold;text-decoration:none;transition:opacity 0.2s;}";
  css += ".btn:hover{opacity:0.85;}";
  css += ".green{background:linear-gradient(135deg,#228B22,#32CD32);}";
  css += ".blue{background:linear-gradient(135deg,#1a6fc4,#4169E1);}";
  css += ".orange{background:linear-gradient(135deg,#c47a1a,#FF8C00);}";
  css += ".red{background:linear-gradient(135deg,#8B0000,#DC143C);}";
  css += ".grey{background:linear-gradient(135deg,#444,#666);}";
  css += ".purple{background:linear-gradient(135deg,#6a0dad,#9b30ff);}";
  css += ".info{margin:20px auto;padding:15px;background:#0d0d1a;border-radius:10px;";
  css += "color:#00d4aa;max-width:400px;font-size:0.95em;}";
  css += ".warn{color:#FF8C00;font-size:0.9em;margin-top:10px;}";
  css += "</style>";
  return css;
}

// ============================================================
// PAGE PRINCIPALE
// ============================================================
void afficher_page() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Drawbot</title>";
  html += cssCommun();
  html += "</head><body>";
  html += "<h1>Drawbot</h1>";
  html += "<p>Choisissez une sequence</p>";
  html += "<a class='btn green' href='/seq1'>Sequence 1 - Escalier</a>";
  html += "<a class='btn blue' href='/seq2'>Sequence 2 - Cercle</a>";
  html += "<a class='btn orange' href='/seq3'>Sequence 3 - Rose des vents</a>";
  html += "<a class='btn purple' href='/calibration'>Calibration</a>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "<div class='info'>Robot connecte et pret !<br>";
  html += "MM/imp D: " + String(MM_PAR_IMP_D, 4);
  html += " | MM/imp G: " + String(MM_PAR_IMP_G, 4);
  html += "<br>Entraxe: " + String(ENTRAXE_MM, 1) + " mm</div>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

// ============================================================
// SOUS-MENU SEQUENCE 1
// ============================================================
void afficher_seq1() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Escalier</title>";
  html += cssCommun();
  html += "</head><body>";
  html += "<h1>Drawbot</h1>";
  html += "<h2>Sequence 1 - Escalier</h2>";
  html += "<div class='info'>";
  html += "Trajet : 20cm, 90&deg; gauche, 10cm, 90&deg; droite, 40cm<br><br>";
  html += "<b>Robot</b> : le robot suit le chemin en escalier<br>";
  html += "<b>Stylo</b> : le stylo trace des angles droits nets<br><br>";
  html += "<span class='warn'>Posez le robot sur la feuille avant de lancer</span>";
  html += "</div>";
  html += "<a class='btn green' href='/seq1/robot'>Escalier Robot</a>";
  html += "<a class='btn blue' href='/seq1/stylo'>Escalier Stylo</a>";
  html += "<a class='btn grey' href='/'>Retour</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_seq1_robot() {
  actionAFaire = 1;
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='15;url=/seq1'>";
  html += cssCommun();
  html += "</head><body>";
  html += "<h2>Escalier Robot en cours...</h2>";
  html += "<div class='info'>Ne pas bouger le robot !<br>";
  html += "<span class='warn'>Retour dans 15s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP urgence</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_seq1_stylo() {
  actionAFaire = 7;
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='15;url=/seq1'>";
  html += cssCommun();
  html += "</head><body>";
  html += "<h2>Escalier Stylo en cours...</h2>";
  html += "<div class='info'>Le stylo trace les angles droits !<br>Ne pas bouger le robot !<br>";
  html += "<span class='warn'>Retour dans 15s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP urgence</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

// ============================================================
// PAGE CALIBRATION
// ============================================================
void afficher_calibration() {
  mettreAJourCapteursEtOdometrie();

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Calibration</title>";
  html += cssCommun();
  html += "<style>";
  html += "input[type=number]{width:120px;padding:8px;font-size:1.1em;border-radius:8px;border:none;text-align:center;}";
  html += ".section{background:#0d0d1a;border-radius:12px;padding:20px;max-width:520px;margin:15px auto;}";
  html += ".section h3{color:#00d4aa;margin-top:0;}";
  html += "table{margin:auto;border-collapse:collapse;width:100%;max-width:480px;}";
  html += "td{border-bottom:1px solid #333;padding:7px;text-align:left;}";
  html += "td:nth-child(2){text-align:right;color:#00d4aa;font-weight:bold;}";
  html += "</style></head><body>";
  html += "<h1>Calibration</h1>";

  html += "<div class='section'><h3>Commandes simples</h3>";
  html += "<form action='/calibration/cmd_avancer' method='get'>Distance cm : <input type='number' name='d' step='1' min='1' max='100' value='20'><button class='btn green' type='submit'>Avancer</button></form>";
  html += "<form action='/calibration/cmd_reculer' method='get'>Distance cm : <input type='number' name='d' step='1' min='1' max='100' value='10'><button class='btn orange' type='submit'>Reculer</button></form>";
  html += "<form action='/calibration/cmd_tourner_gauche' method='get'>Angle deg : <input type='number' name='a' step='5' min='5' max='360' value='90'><button class='btn blue' type='submit'>Tourner gauche</button></form>";
  html += "<form action='/calibration/cmd_tourner_droite' method='get'>Angle deg : <input type='number' name='a' step='5' min='5' max='360' value='90'><button class='btn blue' type='submit'>Tourner droite</button></form>";
  html += "<a class='btn red' href='/stop'>STOP</a></div>";

  html += "<div class='section'><h3>Encodeurs</h3><table>";
  html += "<tr><td>Compteur total gauche</td><td id='encG'>" + String(totalCompteurG) + "</td></tr>";
  html += "<tr><td>Compteur total droite</td><td id='encD'>" + String(totalCompteurD) + "</td></tr>";
  html += "<tr><td>Distance gauche</td><td id='distG'>" + String(totalCompteurG * MM_PAR_IMP_G / 10.0f, 1) + " cm</td></tr>";
  html += "<tr><td>Distance droite</td><td id='distD'>" + String(totalCompteurD * MM_PAR_IMP_D / 10.0f, 1) + " cm</td></tr>";
  html += "<tr><td>Vitesse roue gauche</td><td id='vitG'>" + String(vitesseRoueG_cm_s, 2) + " cm/s</td></tr>";
  html += "<tr><td>Vitesse roue droite</td><td id='vitD'>" + String(vitesseRoueD_cm_s, 2) + " cm/s</td></tr>";
  html += "</table></div>";

  html += "<div class='section'><h3>Position encodeurs</h3><table>";
  html += "<tr><td>X robot</td><td id='posX'>" + String(posX_mm / 10.0f, 1) + " cm</td></tr>";
  html += "<tr><td>Y robot</td><td id='posY'>" + String(posY_mm / 10.0f, 1) + " cm</td></tr>";
  html += "<tr><td>Orientation theta</td><td id='theta'>" + String(thetaRad * 180.0f / PI, 1) + " deg</td></tr>";
  html += "<tr><td>X stylo</td><td id='styloX'>" + String(styloX_mm / 10.0f, 1) + " cm</td></tr>";
  html += "<tr><td>Y stylo</td><td id='styloY'>" + String(styloY_mm / 10.0f, 1) + " cm</td></tr>";
  html += "</table><a class='btn grey' href='/calibration/reset_odo'>Reset odometrie</a></div>";

  html += "<div class='section'><h3>IMU LSM6DS3</h3>Etat : <b id='imuEtat'>" + etatOuiNon(imuOK) + "</b><br><br><table>";
  html += "<tr><td>Accel X</td><td id='accX'>" + String(accX, 2) + " m/s2</td></tr>";
  html += "<tr><td>Accel Y</td><td id='accY'>" + String(accY, 2) + " m/s2</td></tr>";
  html += "<tr><td>Accel Z</td><td id='accZ'>" + String(accZ, 2) + " m/s2</td></tr>";
  html += "<tr><td>Gyro X</td><td id='gyroX'>" + String(gyroX, 3) + " rad/s</td></tr>";
  html += "<tr><td>Gyro Y</td><td id='gyroY'>" + String(gyroY, 3) + " rad/s</td></tr>";
  html += "<tr><td>Gyro Z</td><td id='gyroZ'>" + String(gyroZ, 3) + " rad/s</td></tr>";
  html += "<tr><td>Orientation gyro Z</td><td id='orientationGyro'>" + String(orientationGyroDeg, 1) + " deg</td></tr>";
  html += "</table></div>";

  html += "<div class='section'><h3>Magnetometre LIS3MDL</h3>Etat : <b id='magEtat'>" + etatOuiNon(magOK) + "</b><br><br><table>";
  html += "<tr><td>Mag X</td><td id='magX'>" + String(magX, 2) + " uT</td></tr>";
  html += "<tr><td>Mag Y</td><td id='magY'>" + String(magY, 2) + " uT</td></tr>";
  html += "<tr><td>Mag Z</td><td id='magZ'>" + String(magZ, 2) + " uT</td></tr>";
  html += "<tr><td>Cap estime</td><td id='cap'>" + String(capDeg, 1) + " deg</td></tr>";
  html += "</table></div>";

  html += "<div class='section'><h3>Parametres actuels</h3>";
  html += "MM/imp Droite : <b>" + String(MM_PAR_IMP_D, 4) + " mm</b><br><br>";
  html += "MM/imp Gauche : <b>" + String(MM_PAR_IMP_G, 4) + " mm</b><br><br>";
  html += "Entraxe : <b>" + String(ENTRAXE_MM, 1) + " mm</b><br><br>";
  html += "Vitesse rotation : <b>" + String(VITESSE_TOURNER) + "</b><br><br>";

  html += "<form action='/calibration/setmmd' method='get'>";
  html += "<label>Nouveau MM/imp Droite :</label><br>";
  html += "<input type='number' name='val' step='0.0001' min='0.05' max='1.0' value='" + String(MM_PAR_IMP_D, 4) + "'><br><br>";
  html += "<button class='btn green' type='submit'>Mettre a jour MM/imp D</button>";
  html += "</form><br>";

  html += "<form action='/calibration/setmmg' method='get'>";
  html += "<label>Nouveau MM/imp Gauche :</label><br>";
  html += "<input type='number' name='val' step='0.0001' min='0.05' max='1.0' value='" + String(MM_PAR_IMP_G, 4) + "'><br><br>";
  html += "<button class='btn green' type='submit'>Mettre a jour MM/imp G</button>";
  html += "</form><br>";

  html += "<form action='/calibration/setentraxe' method='get'>";
  html += "<label>Nouvel entraxe (mm) :</label><br>";
  html += "<input type='number' name='val' step='0.5' min='50' max='150' value='" + String(ENTRAXE_MM, 1) + "'><br><br>";
  html += "<button class='btn blue' type='submit'>Mettre a jour Entraxe</button>";
  html += "</form></div>";

  html += "<div class='section'><h3>Tests mouvement normal</h3>";
  html += "<a class='btn green' href='/calibration/avancer20'>Test 20cm</a>";
  html += "<a class='btn green' href='/calibration/avancer40'>Test 40cm (2x20cm)</a>";
  html += "<a class='btn green' href='/calibration/avancer10'>Test 10cm (2x5cm)</a>";
  html += "<a class='btn blue' href='/calibration/tourner90g'>Test 90&deg; gauche</a>";
  html += "<a class='btn blue' href='/calibration/tourner90d'>Test 90&deg; droite</a>";
  html += "</div>";

  html += "<div class='section'><h3>Tests rotation STYLO</h3>";
  html += "<p style='color:#aaa;font-size:0.85em'>Verifiez que le stylo reste fixe !</p>";
  html += "<a class='btn orange' href='/calibration/tourner90g_stylo'>Test 90&deg; gauche STYLO</a>";
  html += "<a class='btn orange' href='/calibration/tourner90d_stylo'>Test 90&deg; droite STYLO</a>";
  html += "</div>";

  html += "<div class='section'><h3>Aide au calcul</h3>";
  html += "<p style='color:#aaa;font-size:0.85em'>";
  html += "Si roue D avance X cm au lieu de 20cm :<br>";
  html += "<b>MM/imp D = " + String(MM_PAR_IMP_D, 4) + " x (X/20)</b><br><br>";
  html += "Si roue G avance X cm au lieu de 20cm :<br>";
  html += "<b>MM/imp G = " + String(MM_PAR_IMP_G, 4) + " x (X/20)</b><br><br>";
  html += "Si tourne Y&deg; au lieu de 90&deg; :<br>";
  html += "<b>Entraxe = " + String(ENTRAXE_MM, 1) + " x (Y/90)</b>";
  html += "</p></div>";

  html += "<a class='btn grey' href='/'>Retour</a>";
  html += "<script>";
  html += "async function majCalibration(){try{const r=await fetch('/calibration/data',{cache:'no-store'});const d=await r.json();Object.keys(d).forEach(k=>{const e=document.getElementById(k);if(e)e.textContent=d[k];});}catch(e){}}";
  html += "setInterval(majCalibration,1000);majCalibration();";
  html += "</script>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calibration_data() {
  mettreAJourCapteursEtOdometrie();

  String json = "{";
  json += "\"encG\":\"" + String(totalCompteurG) + "\",";
  json += "\"encD\":\"" + String(totalCompteurD) + "\",";
  json += "\"distG\":\"" + String(totalCompteurG * MM_PAR_IMP_G / 10.0f, 1) + " cm\",";
  json += "\"distD\":\"" + String(totalCompteurD * MM_PAR_IMP_D / 10.0f, 1) + " cm\",";
  json += "\"vitG\":\"" + String(vitesseRoueG_cm_s, 2) + " cm/s\",";
  json += "\"vitD\":\"" + String(vitesseRoueD_cm_s, 2) + " cm/s\",";
  json += "\"posX\":\"" + String(posX_mm / 10.0f, 1) + " cm\",";
  json += "\"posY\":\"" + String(posY_mm / 10.0f, 1) + " cm\",";
  json += "\"theta\":\"" + String(thetaRad * 180.0f / PI, 1) + " deg\",";
  json += "\"styloX\":\"" + String(styloX_mm / 10.0f, 1) + " cm\",";
  json += "\"styloY\":\"" + String(styloY_mm / 10.0f, 1) + " cm\",";
  json += "\"imuEtat\":\"" + etatOuiNon(imuOK) + "\",";
  json += "\"accX\":\"" + String(accX, 2) + " m/s2\",";
  json += "\"accY\":\"" + String(accY, 2) + " m/s2\",";
  json += "\"accZ\":\"" + String(accZ, 2) + " m/s2\",";
  json += "\"gyroX\":\"" + String(gyroX, 3) + " rad/s\",";
  json += "\"gyroY\":\"" + String(gyroY, 3) + " rad/s\",";
  json += "\"gyroZ\":\"" + String(gyroZ, 3) + " rad/s\",";
  json += "\"orientationGyro\":\"" + String(orientationGyroDeg, 1) + " deg\",";
  json += "\"magEtat\":\"" + etatOuiNon(magOK) + "\",";
  json += "\"magX\":\"" + String(magX, 2) + " uT\",";
  json += "\"magY\":\"" + String(magY, 2) + " uT\",";
  json += "\"magZ\":\"" + String(magZ, 2) + " uT\",";
  json += "\"cap\":\"" + String(capDeg, 1) + " deg\"";
  json += "}";

  serveur.send(200, "application/json", json);
}

void handle_set_mmd() {
  if (serveur.hasArg("val")) {
    float val = serveur.arg("val").toFloat();
    if (val > 0.05f && val < 1.0f) {
      MM_PAR_IMP_D = val;
      Serial.print("MM_PAR_IMP_D : "); Serial.println(MM_PAR_IMP_D, 4);
    }
  }
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_set_mmg() {
  if (serveur.hasArg("val")) {
    float val = serveur.arg("val").toFloat();
    if (val > 0.05f && val < 1.0f) {
      MM_PAR_IMP_G = val;
      Serial.print("MM_PAR_IMP_G : "); Serial.println(MM_PAR_IMP_G, 4);
    }
  }
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_set_entraxe() {
  if (serveur.hasArg("val")) {
    float val = serveur.arg("val").toFloat();
    if (val > 50.0f && val < 150.0f) {
      ENTRAXE_MM = val;
      Serial.print("ENTRAXE_MM : "); Serial.println(ENTRAXE_MM, 1);
    }
  }
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_calib_avancer20() {
  actionAFaire = 2;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='6;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 20cm...</h2>";
  html += "<div class='info'>Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 6s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_avancer40() {
  actionAFaire = 3;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='10;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 40cm...</h2>";
  html += "<div class='info'>Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 10s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_avancer10() {
  actionAFaire = 6;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='6;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 10cm...</h2>";
  html += "<div class='info'>Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 6s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_tourner90g() {
  actionAFaire = 4;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 90&deg; gauche...</h2>";
  html += "<div class='info'>Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 5s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_tourner90d() {
  actionAFaire = 5;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 90&deg; droite...</h2>";
  html += "<div class='info'>Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 5s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_tourner90g_stylo() {
  actionAFaire = 8;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='8;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 90&deg; gauche STYLO...</h2>";
  html += "<div class='info'>Verifiez que le stylo reste fixe !<br>";
  html += "Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 8s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_calib_tourner90d_stylo() {
  actionAFaire = 9;
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='8;url=/calibration'>";
  html += cssCommun();
  html += "</head><body><h2>Test 90&deg; droite STYLO...</h2>";
  html += "<div class='info'>Verifiez que le stylo reste fixe !<br>";
  html += "Regardez le moniteur serie !<br>";
  html += "<span class='warn'>Retour dans 8s</span></div>";
  html += "<a class='btn red' href='/stop'>STOP</a>";
  html += "</body></html>";
  serveur.send(200, "text/html", html);
}

void handle_cmd_avancer() {
  if (serveur.hasArg("d")) {
    distanceCmdMm = serveur.arg("d").toFloat() * 10.0f;
    distanceCmdMm = constrain(distanceCmdMm, 10.0f, 1000.0f);
  }
  actionAFaire = 11;
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_cmd_reculer() {
  if (serveur.hasArg("d")) {
    distanceCmdMm = serveur.arg("d").toFloat() * 10.0f;
    distanceCmdMm = constrain(distanceCmdMm, 10.0f, 1000.0f);
  }
  actionAFaire = 12;
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_cmd_tourner_gauche() {
  if (serveur.hasArg("a")) {
    angleCmdDeg = serveur.arg("a").toFloat();
    angleCmdDeg = constrain(angleCmdDeg, 5.0f, 360.0f);
  }
  actionAFaire = 13;
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_cmd_tourner_droite() {
  if (serveur.hasArg("a")) {
    angleCmdDeg = serveur.arg("a").toFloat();
    angleCmdDeg = constrain(angleCmdDeg, 5.0f, 360.0f);
  }
  actionAFaire = 14;
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void handle_reset_odometrie() {
  actionAFaire = 15;
  serveur.sendHeader("Location", "/calibration", true);
  serveur.send(302, "text/plain", "");
}

void afficher_seq2() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Cercle</title>";
  html += cssCommun();
  html += "</head><body>";

  html += "<h1>Drawbot</h1>";
  html += "<h2>Sequence 2 - Cercle</h2>";

  html += "<div class='info'>";
  html += "Choix : 2 cm, 8 cm, ou 13.5 cm a 20 cm<br>";
  html += "Rayon actuel : <b>" + String(rayonCercleCm, 1) + " cm</b>";
  html += "</div>";

  html += "<form action='/seq2/lancer' method='get'>";
  html += "<input type='number' name='r' step='0.5' min='13.5' max='20' value='" + String(rayonCercleCm, 1) + "' ";
  html += "style='width:120px;padding:10px;font-size:1.1em;border-radius:8px;border:none;text-align:center;'><br><br>";
  html += "<button class='btn blue' type='submit'>Dessiner le cercle</button>";
  html += "</form>";
  html += "<a class='btn orange' href='/seq2/lancer?r=2'>Test cercle 2 cm</a>";
  html += "<a class='btn orange' href='/seq2/lancer?r=8'>Test cercle 8 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=13.5'>13.5 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=14'>14 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=15'>15 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=16'>16 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=17'>17 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=18'>18 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=19'>19 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=20'>20 cm</a>";

  html += "<a class='btn grey' href='/'>Retour</a>";
  html += "</body></html>";

  serveur.send(200, "text/html", html);
}

void handle_seq2_lancer() {
  if (serveur.hasArg("r")) {
    rayonCercleCm = serveur.arg("r").toFloat();

    rayonCercleCm = normaliserRayonCercle(rayonCercleCm);
  }

  actionAFaire = 10;

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='15;url=/seq2'>";
  html += cssCommun();
  html += "</head><body>";
  html += "<h2>Cercle en cours...</h2>";
  html += "<div class='info'>Rayon : " + String(rayonCercleCm, 1) + " cm<br>";
  html += "Ne pas bouger le robot.</div>";
  html += "<a class='btn red' href='/stop'>STOP urgence</a>";
  html += "</body></html>";

  serveur.send(200, "text/html", html);
}

void afficher_seq3() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='2;url=/'>";
  html += cssCommun();
  html += "</head><body><h2>Sequence 3 - Rose des vents (a venir)</h2></body></html>";
  serveur.send(200, "text/html", html);
}

void handle_stop() {
  arreter();
  actionAFaire = 0;
  Serial.println("STOP force !");
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='1;url=/'>";
  html += cssCommun();
  html += "</head><body><h2>STOP !</h2></body></html>";
  serveur.send(200, "text/html", html);
}

// ============================================================
// SEQUENCES DE DESSIN
// ============================================================
void sequenceEscalier() {
  Serial.println("=== DEBUT ESCALIER ROBOT ===");

  Serial.println("[1/5] Avance 20cm");
  avancer(200.0f);
  delay(200);

  Serial.println("[2/5] Tourne 90 gauche");
  tourner(90.0f);
  delay(200);

  Serial.println("[3/5] Avance 10cm (2x5cm)");
  avancer(50.0f);
  avancer(50.0f);
  delay(200);

  Serial.println("[4/5] Tourne 90 droite");
  tourner(-90.0f);
  delay(200);

  Serial.println("[5/5] Avance 40cm (2x20cm)");
  avancer(200.0f);
  avancer(200.0f);

  Serial.println("=== FIN ESCALIER ROBOT ===");
}

// ============================================================
// Escalier dessine avec le stylo
// Le stylo trace les trois segments.
// ============================================================
#define ENTRAXE_REEL_MM 90.0f
#define OFFSET_STYLO_MM 130.0f

void mouvementRouesMm(float distD_mm, float distG_mm, int vitesse) {
  long cibleD = abs(distD_mm) / MM_PAR_IMP_D;
  long cibleG = abs(distG_mm) / MM_PAR_IMP_G;

  compteurD = 0;
  compteurG = 0;

  if (distD_mm > 0) setMoteurDroit(vitesse, false); // droite avance
  if (distD_mm < 0) setMoteurDroit(vitesse, true);  // droite recule

  if (distG_mm > 0) setMoteurGauche(vitesse, true);  // gauche avance
  if (distG_mm < 0) setMoteurGauche(vitesse, false); // gauche recule

  while (compteurD < cibleD || compteurG < cibleG) {
    if (compteurD >= cibleD) {
      ledcWrite(CH_IN1_D, 0);
      ledcWrite(CH_IN2_D, 0);
    }
    if (compteurG >= cibleG) {
      ledcWrite(CH_IN1_G, 0);
      ledcWrite(CH_IN2_G, 0);
    }
  }

  arreter();
  delay(5);
}

void traitStyloGauche(float longueur_mm) {
  Serial.println("--- TRAIT STYLO GAUCHE ---");

  int vitesse = 120;
  int steps = 53; // reglage pour obtenir environ 10 cm

  float thetaFinal = asin(longueur_mm / OFFSET_STYLO_MM);
  float dtheta = thetaFinal / 80.0f;

  for (int i = 0; i < steps; i++) {
    float theta = i * dtheta + dtheta / 2.0f;

    float dsCentre = OFFSET_STYLO_MM * sin(theta) * dtheta;

    float dsD = dsCentre + (ENTRAXE_REEL_MM / 2.0f) * dtheta;
    float dsG = dsCentre - (ENTRAXE_REEL_MM / 2.0f) * dtheta;

    mouvementRouesMm(dsD, dsG, vitesse);
  }

  freiner();
}

void traitStyloDroite(float longueur_mm) {
  Serial.println("--- TRAIT DROITE STYLO 40 CM ATTAQUE + ATTENUATION ---");

  int vitesse = 115;

  // Depart du trait vers la droite.
  for (int i = 0; i < 26; i++) {
    mouvementRouesMm(
      0.55f,  // roue droite avance peu
      2.35f,  // roue gauche avance fort
      vitesse
    );
  }

  // Transition progressive.
  for (int i = 0; i < 35; i++) {
    float t = (float)i / 34.0f;

    float distD = 0.55f + (1.45f - 0.55f) * t;
    float distG = 2.35f + (1.90f - 2.35f) * t;

    mouvementRouesMm(distD, distG, vitesse);
  }

  // Fin du trait.
  int nbCycles = 88;

  for (int i = 0; i < nbCycles; i++) {
    float t = (float)i / (float)(nbCycles - 1);

    float distD = 1.45f + 0.30f * t;
    float distG = 1.90f - 0.05f * t;

    mouvementRouesMm(distD, distG, vitesse);
  }

  freiner();
  Serial.println("--- FIN TRAIT DROITE STYLO ---");
}
void sequenceEscalierStylo() {
  Serial.println("=== DEBUT ESCALIER STYLO ===");

  // Trait 1 : 20 cm tout droit
  avancer(200.0f);
  delay(300);

  // Trait 2 : 10 cm vers la gauche
  traitStyloGauche(100.0f);
  delay(300);

  // Trait 3 : 40 cm vers la droite
  traitStyloDroite(400.0f);

  freiner();
  Serial.println("=== FIN ESCALIER STYLO ===");
}


// ============================================================
// CERCLE
// ============================================================
float normaliserRayonCercle(float rayonCm) {
  if (rayonCm < 5.0f) return 2.0f;
  if (rayonCm < 13.5f) return 8.0f;
  if (rayonCm <= 13.5f) return 13.5f;
  if (rayonCm > 20.0f) return 20.0f;
  return ceil(rayonCm);
}

void mouvementRouesCercle(float distD_mm, float distG_mm, int vitesse, float compensationMm = 0.0f, bool arretFin = true) {
  float absD = fabsf(distD_mm);
  float absG = fabsf(distG_mm);

  if (absD > compensationMm) absD -= compensationMm;
  if (absG > compensationMm) absG -= compensationMm;

  long cibleD = (long)(absD / MM_PAR_IMP_D + 0.5f);
  long cibleG = (long)(absG / MM_PAR_IMP_G + 0.5f);

  if (cibleD == 0 && cibleG == 0) return;

  compteurD = 0;
  compteurG = 0;

  float distMax = max(absD, absG);
  int vitesseD = (cibleD == 0) ? 0 : constrain((int)(vitesse * absD / distMax), 75, vitesse);
  int vitesseG = (cibleG == 0) ? 0 : constrain((int)(vitesse * absG / distMax), 75, vitesse);

  if (cibleD > 0) setMoteurDroit(vitesseD, distD_mm < 0);
  if (cibleG > 0) setMoteurGauche(vitesseG, distG_mm > 0);
  if (cibleD == 0) {
    ledcWrite(CH_IN1_D, 0);
    ledcWrite(CH_IN2_D, 0);
  }
  if (cibleG == 0) {
    ledcWrite(CH_IN1_G, 0);
    ledcWrite(CH_IN2_G, 0);
  }

  while (compteurD < cibleD || compteurG < cibleG) {
    bool dFini = compteurD >= cibleD;
    bool gFini = compteurG >= cibleG;

    if (dFini) {
      ledcWrite(CH_IN1_D, 0);
      ledcWrite(CH_IN2_D, 0);
    }
    if (gFini) {
      ledcWrite(CH_IN1_G, 0);
      ledcWrite(CH_IN2_G, 0);
    }

    if (!dFini && !gFini && cibleD > 0 && cibleG > 0) {
      float progD = (float)compteurD / (float)cibleD;
      float progG = (float)compteurG / (float)cibleG;
      int correction = constrain((int)((progD - progG) * 80.0f), -22, 22);

      setMoteurDroit(constrain(vitesseD - correction, 70, vitesse), distD_mm < 0);
      setMoteurGauche(constrain(vitesseG + correction, 70, vitesse), distG_mm > 0);
    }

    delay(2);
  }

  arreter();
  if (arretFin) delay(2);
  else delay(1);
}

void cerclePetitDoux(float rayonCm) {
  Serial.println("--- CERCLE PETIT DOUX ---");

  float R = rayonCm * 10.0f;
  float Rc = R - OFFSET_STYLO_CERCLE_MM;
  float angle = 2.0f * PI;
  float distD = angle * (Rc + ENTRAXE_CERCLE_MM / 2.0f);
  float distG = angle * (Rc - ENTRAXE_CERCLE_MM / 2.0f);

  if (rayonCm <= 3.0f) {
    mouvementRouesCercle(distD, distG, 130, 0.0f);
  } else {
    int steps = 8;
    for (int i = 0; i < steps; i++) {
      mouvementRouesCercle(distD / steps, distG / steps, 135, 0.0f, false);
    }
    arreter();
  }

  freiner();
}

void cercleGrandPropre(float rayonCm) {
  Serial.println("--- CERCLE GRAND PROPRE ---");

  rayonCm = normaliserRayonCercle(rayonCm);
  if (rayonCm < 13.5f) rayonCm = 13.5f;

  float R = rayonCm * 10.0f;
  float Rc = R - OFFSET_STYLO_CERCLE_MM;
  if (Rc < 0.0f) Rc = 0.0f;

  float distDTotal = 2.0f * PI * (Rc + ENTRAXE_CERCLE_MM / 2.0f);
  float distGTotal = 2.0f * PI * (Rc - ENTRAXE_CERCLE_MM / 2.0f);
  if (rayonCm <= 13.5f) {
    distDTotal *= 0.985f;
    distGTotal *= 0.985f;
  }

  Serial.print("Rayon preset : ");
  Serial.print(rayonCm);
  Serial.println(" cm");
  Serial.print("Rc robot : ");
  Serial.print(Rc / 10.0f);
  Serial.println(" cm");
  Serial.print("Distance D/G : ");
  Serial.print(distDTotal);
  Serial.print(" / ");
  Serial.println(distGTotal);

  float compensation = (rayonCm <= 13.5f) ? 4.0f : 0.0f;

  if (rayonCm < 18.0f) {
    int steps = (rayonCm <= 14.0f) ? 48 : 40;
    for (int i = 0; i < steps; i++) {
      mouvementRouesCercle(distDTotal / steps, distGTotal / steps, 130, 0.0f, false);
    }
    arreter();
  } else {
    mouvementRouesCercle(distDTotal, distGTotal, 125, compensation);
  }
  freiner();
}

// Fonction principale des cercles
void dessinerCercle(float rayonCm) {
  Serial.println("=== DEBUT CERCLE ===");
  Serial.print("Rayon demande : ");
  Serial.println(rayonCm);

  rayonCm = normaliserRayonCercle(rayonCm);

  if (rayonCm < 5.0f) cerclePetitDoux(2.0f);
  else if (rayonCm < 13.5f) cerclePetitDoux(8.0f);
  else cercleGrandPropre(rayonCm);

  Serial.println("=== FIN CERCLE ===");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Demarrage Drawbot...");

  Wire.begin(21, 22);

  if (imu.begin_I2C(0x6B)) {
    imuOK = true;
    Serial.println("IMU LSM6DS3 OK");
  } else {
    imuOK = false;
    Serial.println("IMU LSM6DS3 NON DETECTEE");
  }

  if (mag.begin_I2C(0x1E)) {
    magOK = true;
    mag.setPerformanceMode(LIS3MDL_HIGHMODE);
    mag.setOperationMode(LIS3MDL_CONTINUOUSMODE);
    mag.setDataRate(LIS3MDL_DATARATE_155_HZ);
    mag.setRange(LIS3MDL_RANGE_4_GAUSS);
    Serial.println("Magnetometre LIS3MDL OK");
  } else {
    magOK = false;
    Serial.println("Magnetometre LIS3MDL NON DETECTE");
  }

  ledcSetup(CH_IN1_D, PWM_FREQ, PWM_RES);
  ledcSetup(CH_IN2_D, PWM_FREQ, PWM_RES);
  ledcSetup(CH_IN1_G, PWM_FREQ, PWM_RES);
  ledcSetup(CH_IN2_G, PWM_FREQ, PWM_RES);
  ledcAttachPin(IN_1_D, CH_IN1_D);
  ledcAttachPin(IN_2_D, CH_IN2_D);
  ledcAttachPin(IN_1_G, CH_IN1_G);
  ledcAttachPin(IN_2_G, CH_IN2_G);

  pinMode(EN_D, OUTPUT); digitalWrite(EN_D, HIGH);
  pinMode(EN_G, OUTPUT); digitalWrite(EN_G, HIGH);

  pinMode(ENC_G_CH_A, INPUT_PULLUP);
  pinMode(ENC_G_CH_B, INPUT_PULLUP);
  pinMode(ENC_D_CH_A, INPUT_PULLUP);
  pinMode(ENC_D_CH_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_G_CH_A), isr_enc_G, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_D_CH_A), isr_enc_D, CHANGE);

  WiFi.softAP(ssid, password);
  Serial.print("IP : ");
  Serial.println(WiFi.softAPIP());

  serveur.on("/",                              afficher_page);
  serveur.on("/seq1",                          afficher_seq1);
  serveur.on("/seq1/robot",                    handle_seq1_robot);
  serveur.on("/seq1/stylo",                    handle_seq1_stylo);
  serveur.on("/seq2",                          afficher_seq2);
  serveur.on("/seq3",                          afficher_seq3);
  serveur.on("/stop",                          handle_stop);
  serveur.on("/calibration",                   afficher_calibration);
  serveur.on("/calibration/data",              handle_calibration_data);
  serveur.on("/calibration/setmmd",            handle_set_mmd);
  serveur.on("/calibration/setmmg",            handle_set_mmg);
  serveur.on("/calibration/setentraxe",        handle_set_entraxe);
  serveur.on("/calibration/avancer20",         handle_calib_avancer20);
  serveur.on("/calibration/avancer40",         handle_calib_avancer40);
  serveur.on("/calibration/avancer10",         handle_calib_avancer10);
  serveur.on("/calibration/tourner90g",        handle_calib_tourner90g);
  serveur.on("/calibration/tourner90d",        handle_calib_tourner90d);
  serveur.on("/calibration/tourner90g_stylo",  handle_calib_tourner90g_stylo);
  serveur.on("/calibration/tourner90d_stylo",  handle_calib_tourner90d_stylo);
  serveur.on("/calibration/cmd_avancer",       handle_cmd_avancer);
  serveur.on("/calibration/cmd_reculer",       handle_cmd_reculer);
  serveur.on("/calibration/cmd_tourner_gauche", handle_cmd_tourner_gauche);
  serveur.on("/calibration/cmd_tourner_droite", handle_cmd_tourner_droite);
  serveur.on("/calibration/reset_odo",         handle_reset_odometrie);
  serveur.on("/seq2/lancer", handle_seq2_lancer);
  serveur.begin();

  Serial.println("Drawbot pret ! WiFi: Drawbot | 192.168.4.1");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  serveur.handleClient();
  mettreAJourCapteursEtOdometrie();

  switch (actionAFaire) {
    case 1:
      actionAFaire = 0;
      sequenceEscalier();
      break;
    case 2:
      actionAFaire = 0;
      avancerCalibrationPID(200.0f);
      break;
    case 3:
      actionAFaire = 0;
      Serial.println("=== TEST 40cm (2x20cm) ===");
      avancerCalibrationPID(200.0f);
      avancerCalibrationPID(200.0f);
      break;
    case 4:
      actionAFaire = 0;
      tournerCalibrationPID(90.0f);
      break;
    case 5:
      actionAFaire = 0;
      tournerCalibrationPID(-90.0f);
      break;
    case 6:
      actionAFaire = 0;
      Serial.println("=== TEST 10cm (2x5cm) ===");
      avancerCalibrationPID(50.0f);
      avancerCalibrationPID(50.0f);
      break;
    case 7:
      actionAFaire = 0;
      sequenceEscalierStylo();
      break;
    case 8:
      actionAFaire = 0;
      tournerStylo(90.0f);
      break;
    case 9:
      actionAFaire = 0;
      tournerStylo(-90.0f);
      break;
    case 10:
      actionAFaire = 0;
      dessinerCercle(rayonCercleCm);
      break;  
    case 11:
      actionAFaire = 0;
      avancerCalibrationPID(distanceCmdMm);
      break;
    case 12:
      actionAFaire = 0;
      reculerCalibrationPID(distanceCmdMm);
      break;
    case 13:
      actionAFaire = 0;
      tournerCalibrationPID(angleCmdDeg);
      break;
    case 14:
      actionAFaire = 0;
      tournerCalibrationPID(-angleCmdDeg);
      break;
    case 15:
      actionAFaire = 0;
      posX_mm = 0;
      posY_mm = 0;
      thetaRad = 0;
      styloX_mm = OFFSET_STYLO_ODOMETRIE_MM;
      styloY_mm = 0;
      orientationGyroDeg = 0;
      totalCompteurG = 0;
      totalCompteurD = 0;
      dernierTotalG = 0;
      dernierTotalD = 0;
      Serial.println("Odometrie remise a zero");
      break;
    default:
      break;
  }
}
