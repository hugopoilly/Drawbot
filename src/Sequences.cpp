#include "Robot.h"

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

