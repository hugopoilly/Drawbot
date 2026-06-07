#include "Robot.h"

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

  Serial.println("--------- VALEURS MESUREES ---------");
  Serial.print("Impulsions mesurees roue droite : ");
  Serial.println(compteurD);
  Serial.print("Impulsions mesurees roue gauche : ");
  Serial.println(compteurG);
  Serial.print("Distance mesuree roue droite : ");
  Serial.print(compteurD * MM_PAR_IMP_D / 10.0f, 2);
  Serial.println(" cm");
  Serial.print("Distance mesuree roue gauche : ");
  Serial.print(compteurG * MM_PAR_IMP_G / 10.0f, 2);
  Serial.println(" cm");
  Serial.print("Ecart mesure D-G : ");
  Serial.print((compteurD * MM_PAR_IMP_D - compteurG * MM_PAR_IMP_G) / 10.0f, 2);
  Serial.println(" cm");
  Serial.println("FIN AVANCER CALIBRATION PID");
  Serial.println("------------------------------------");
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

  Serial.println("--------- VALEURS MESUREES ---------");
  Serial.print("Angle demande : ");
  Serial.print(angleDeg);
  Serial.println(" deg");
  Serial.print("Impulsions mesurees roue droite : ");
  Serial.println(compteurD);
  Serial.print("Impulsions mesurees roue gauche : ");
  Serial.println(compteurG);
  Serial.print("Distance mesuree roue droite : ");
  Serial.print(compteurD * MM_PAR_IMP_D / 10.0f, 2);
  Serial.println(" cm");
  Serial.print("Distance mesuree roue gauche : ");
  Serial.print(compteurG * MM_PAR_IMP_G / 10.0f, 2);
  Serial.println(" cm");
  Serial.print("Ecart mesure D-G : ");
  Serial.print((compteurD * MM_PAR_IMP_D - compteurG * MM_PAR_IMP_G) / 10.0f, 2);
  Serial.println(" cm");
  Serial.println("FIN TOURNER CALIBRATION PID");
  Serial.println("------------------------------------");
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

