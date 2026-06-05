#include "Robot.h"

// ============================================================
// Initialisation materielle
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

  // Routes principales de l'interface web du robot.
  serveur.on("/",                               afficher_page);
  serveur.on("/seq1",                           afficher_seq1);
  serveur.on("/seq1/robot",                     handle_seq1_robot);
  serveur.on("/seq1/stylo",                     handle_seq1_stylo);
  serveur.on("/seq2",                           afficher_seq2);
  serveur.on("/seq2/lancer",                    handle_seq2_lancer);
  serveur.on("/seq3",                           afficher_seq3);
  serveur.on("/stop",                           handle_stop);

  // Routes de calibration et de commandes manuelles.
  serveur.on("/calibration",                    afficher_calibration);
  serveur.on("/calibration/data",               handle_calibration_data);
  serveur.on("/calibration/setmmd",             handle_set_mmd);
  serveur.on("/calibration/setmmg",             handle_set_mmg);
  serveur.on("/calibration/setentraxe",         handle_set_entraxe);
  serveur.on("/calibration/avancer20",          handle_calib_avancer20);
  serveur.on("/calibration/avancer40",          handle_calib_avancer40);
  serveur.on("/calibration/avancer10",          handle_calib_avancer10);
  serveur.on("/calibration/tourner90g",         handle_calib_tourner90g);
  serveur.on("/calibration/tourner90d",         handle_calib_tourner90d);
  serveur.on("/calibration/tourner90g_stylo",   handle_calib_tourner90g_stylo);
  serveur.on("/calibration/tourner90d_stylo",   handle_calib_tourner90d_stylo);
  serveur.on("/calibration/cmd_avancer",        handle_cmd_avancer);
  serveur.on("/calibration/cmd_reculer",        handle_cmd_reculer);
  serveur.on("/calibration/cmd_tourner_gauche", handle_cmd_tourner_gauche);
  serveur.on("/calibration/cmd_tourner_droite", handle_cmd_tourner_droite);
  serveur.on("/calibration/reset_odo",          handle_reset_odometrie);

  serveur.begin();

  Serial.println("Drawbot pret !");
  Serial.println("WiFi : Drawbot");
  Serial.println("Adresse interface : http://192.168.4.1");
}

// ============================================================
// Boucle principale
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
