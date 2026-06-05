#include "Robot.h"

float normaliserRayonCercle(float rayonCm) {
  // Pour la demonstration, seuls les rayons calibres et propres sont proposes.
  if (rayonCm < 15.0f) return 14.0f;
  if (rayonCm < 17.0f) return 16.0f;
  if (rayonCm < 19.0f) return 18.0f;
  return 20.0f;
}

float rayonRotationCalibre(float rayonCm) {
  // Table finale des 4 cercles calibres. Ne pas modifier sans nouveau test.
  if (rayonCm <= 14.0f) return 40.0f;
  if (rayonCm <= 16.0f) return 49.2f;
  if (rayonCm <= 18.0f) return 57.0f;
  return 68.0f;
}

float gainFermetureCalibre(float rayonCm) {
  // Table finale des gains de fermeture pour les 4 cercles calibres.
  if (rayonCm <= 14.0f) return 0.9447f;
  if (rayonCm <= 16.0f) return 1.428f;
  if (rayonCm <= 18.0f) return 1.635f;
  return 1.80f;
}

void mouvementRouesCercle(float distD_mm, float distG_mm, int vitesse, float compensationMm, bool arretFin) {
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
  int vitesseMin = 65;
  int vitesseD = (cibleD == 0) ? 0 : constrain((int)(vitesse * absD / distMax), vitesseMin, vitesse);
  int vitesseG = (cibleG == 0) ? 0 : constrain((int)(vitesse * absG / distMax), vitesseMin, vitesse);

  // PID de synchronisation : on compare les progressions relatives,
  // car les deux roues ne parcourent pas toujours la meme distance.
  PIDSimple pidCercle = {1.4f, 0.015f, 0.65f, 0.0f, 0.0f, 300.0f};
  resetPID(pidCercle);

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
      float erreurProgression = (progD - progG) * 100.0f;
      int correction = constrain((int)calculerPID(pidCercle, erreurProgression), -28, 28);

      setMoteurDroit(constrain(vitesseD - correction, vitesseMin, vitesse), distD_mm < 0);
      setMoteurGauche(constrain(vitesseG + correction, vitesseMin, vitesse), distG_mm > 0);
    }

    delay(2);
    mettreAJourCapteursEtOdometrie();
  }

  if (arretFin) {
    arreter();
    delay(20);
  }
}

void cercleGrandPropre(float rayonCm) {
  Serial.println("--- CERCLE PID ---");

  rayonCm = normaliserRayonCercle(rayonCm);

  float R = rayonCm * 10.0f;

  float RcGeometrique = sqrtf(max(0.0f, R * R - OFFSET_STYLO_CERCLE_MM * OFFSET_STYLO_CERCLE_MM));
  float RcCalibre = rayonRotationCalibre(rayonCm);
  // Les autres rayons restent sur le comportement valide precedent.
  // Le 14 cm a besoin d'un petit rayon de rotation dedie pour atteindre 28 cm.
  float Rc = (rayonCm <= 14.0f) ? RcCalibre : min(RcGeometrique, RcCalibre);

  float gainFermeture = gainFermetureCalibre(rayonCm);

  float distDTotal = gainFermeture * 2.0f * PI * (Rc + ENTRAXE_CERCLE_MM / 2.0f);
  float distGTotal = gainFermeture * 2.0f * PI * (Rc - ENTRAXE_CERCLE_MM / 2.0f);

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
  Serial.print("Gain fermeture : ");
  Serial.println(gainFermeture, 2);

  // Segmentation sur tous les rayons : le PID corrige souvent, les erreurs
  // s'accumulent moins, et le point de depart/fin reste plus propre.
  int steps = (int)(72.0f * gainFermeture);
  int vitesse = 115;

  for (int i = 0; i < steps; i++) {
    bool dernierSegment = (i == steps - 1);
    mouvementRouesCercle(distDTotal / steps, distGTotal / steps, vitesse, 0.0f, dernierSegment);
  }

  freiner();
}

// Fonction principale des cercles
void dessinerCercle(float rayonCm) {
  Serial.println("=== DEBUT CERCLE ===");
  Serial.print("Rayon demande : ");
  Serial.println(rayonCm);

  rayonCm = normaliserRayonCercle(rayonCm);

  cercleGrandPropre(rayonCm);

  Serial.println("=== FIN CERCLE ===");
}
