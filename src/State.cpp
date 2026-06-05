#include "Robot.h"

// ============================================================
// Variables partagees par les modules
// ============================================================
float MM_PAR_IMP_D = 0.1389f;
float MM_PAR_IMP_G = 0.1389f;
float ENTRAXE_MM = 114.75f;

float KP_ROUES = 2.0f;
float KI_ROUES = 0.004f;
float KD_ROUES = 0.45f;

float rayonCercleCm = 14.0f;
float distanceCmdMm = 200.0f;
float angleCmdDeg = 90.0f;

const char* ssid = "Drawbot";
const char* password = "drawbot123";
WebServer serveur(80);

volatile long compteurG = 0;
volatile long compteurD = 0;
volatile long totalCompteurG = 0;
volatile long totalCompteurD = 0;
volatile int signeEncodeurG = 1;
volatile int signeEncodeurD = 1;
volatile int actionAFaire = 0;

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

String etatOuiNon(bool etat) {
  return etat ? "OK" : "NON DETECTE";
}

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
