#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_LSM6DS3.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_Sensor.h>

// ============================================================
// Brochage du robot
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
// PWM moteurs
// ============================================================
#define CH_IN1_D 0
#define CH_IN2_D 1
#define CH_IN1_G 2
#define CH_IN2_G 3
#define PWM_FREQ 1000
#define PWM_RES  8

// ============================================================
// Parametres mecaniques et de calibration
// ============================================================
#define VITESSE_BASE 80
#define VITESSE_TOURNER 125
#define OFFSET_STYLO_ODOMETRIE_MM 130.0f
#define OFFSET_STYLO_CERCLE_MM 135.0f
#define ENTRAXE_CERCLE_MM 90.0f
#define ENTRAXE_REEL_MM 90.0f
#define OFFSET_STYLO_MM 130.0f

extern float MM_PAR_IMP_D;
extern float MM_PAR_IMP_G;
extern float ENTRAXE_MM;
extern float KP_ROUES;
extern float KI_ROUES;
extern float KD_ROUES;
extern float rayonCercleCm;
extern float distanceCmdMm;
extern float angleCmdDeg;

// ============================================================
// WiFi et serveur web
// ============================================================
extern const char* ssid;
extern const char* password;
extern WebServer serveur;

// ============================================================
// Encodeurs et etat global
// ============================================================
extern volatile long compteurG;
extern volatile long compteurD;
extern volatile long totalCompteurG;
extern volatile long totalCompteurD;
extern volatile int signeEncodeurG;
extern volatile int signeEncodeurD;
extern volatile int actionAFaire;

// Codes d'action lances depuis l'interface web.
// 0 = rien | 1 = escalier robot | 2-6 = tests calibration
// 7 = escalier stylo | 8-9 = rotations stylo | 10 = cercle
// 11-14 = commandes libres | 15 = remise a zero odometrie

// ============================================================
// Capteurs et odometrie
// ============================================================
extern Adafruit_LSM6DS3 imu;
extern Adafruit_LIS3MDL mag;
extern bool imuOK;
extern bool magOK;
extern float accX, accY, accZ;
extern float gyroX, gyroY, gyroZ;
extern float magX, magY, magZ;
extern float capDeg;
extern float orientationGyroDeg;
extern float vitesseRoueG_cm_s;
extern float vitesseRoueD_cm_s;
extern float posX_mm;
extern float posY_mm;
extern float thetaRad;
extern float styloX_mm;
extern float styloY_mm;
extern long dernierTotalG;
extern long dernierTotalD;
extern unsigned long dernierTempsOdo;
extern unsigned long dernierTempsIMU;

struct PIDSimple {
  float kp;
  float ki;
  float kd;
  float somme;
  float derniereErreur;
  float limiteSomme;
};

String etatOuiNon(bool etat);
void resetPID(PIDSimple &pid);
float calculerPID(PIDSimple &pid, float erreur);

void IRAM_ATTR isr_enc_G();
void IRAM_ATTR isr_enc_D();
void mettreAJourCapteursEtOdometrie();

// ============================================================
// Moteurs et mouvements
// ============================================================
void setMoteurDroit(int vitesse, bool avant);
void setMoteurGauche(int vitesse, bool avant);
void arreter();
void freiner();

void avancer(float distanceMm);
void reculer(float distanceMm);
void tourner(float angleDeg);
void avancerCalibrationPID(float distanceMm);
void reculerCalibrationPID(float distanceMm);
void tournerCalibrationPID(float angleDeg);
void tournerStylo(float angleDeg);
void mouvementRouesMm(float distD_mm, float distG_mm, int vitesse);

// ============================================================
// Interface web
// ============================================================
String cssCommun();
void afficher_page();
void afficher_seq1();
void afficher_calibration();
void afficher_seq2();
void afficher_seq3();
void handle_seq1_robot();
void handle_seq1_stylo();
void handle_calibration_data();
void handle_set_mmd();
void handle_set_mmg();
void handle_set_entraxe();
void handle_calib_avancer20();
void handle_calib_avancer40();
void handle_calib_avancer10();
void handle_calib_tourner90g();
void handle_calib_tourner90d();
void handle_calib_tourner90g_stylo();
void handle_calib_tourner90d_stylo();
void handle_cmd_avancer();
void handle_cmd_reculer();
void handle_cmd_tourner_gauche();
void handle_cmd_tourner_droite();
void handle_reset_odometrie();
void handle_seq2_lancer();
void handle_stop();

// ============================================================
// Sequences de dessin
// ============================================================
void sequenceEscalier();
void traitStyloGauche(float longueur_mm);
void traitStyloDroite(float longueur_mm);
void sequenceEscalierStylo();

// ============================================================
// Cercle
// ============================================================
float normaliserRayonCercle(float rayonCm);
void mouvementRouesCercle(float distD_mm, float distG_mm, int vitesse, float compensationMm = 0.0f, bool arretFin = true);
void cercleGrandPropre(float rayonCm);
void dessinerCercle(float rayonCm);
