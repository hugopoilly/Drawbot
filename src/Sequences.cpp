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

// ============================================================
// Sequence 3 : fleche dirigee vers le Nord terrestre
// ============================================================
float normaliserAngle180(float angleDeg) {
  while (angleDeg > 180.0f) angleDeg -= 360.0f;
  while (angleDeg < -180.0f) angleDeg += 360.0f;
  return angleDeg;
}

float lireCapMoyenDeg() {
  if (!magOK) {
    Serial.println("ERREUR : magnetometre non detecte, impossible de viser le Nord.");
    return 0.0f;
  }

  float sommeX = 0.0f;
  float sommeY = 0.0f;

  for (int i = 0; i < 25; i++) {
    mettreAJourCapteursEtOdometrie();
    float capRad = capDeg * PI / 180.0f;
    sommeX += cos(capRad);
    sommeY += sin(capRad);
    delay(20);
  }

  float capMoyen = atan2(sommeY, sommeX) * 180.0f / PI;
  if (capMoyen < 0.0f) capMoyen += 360.0f;
  return capMoyen;
}

void orienterRobotVersNord() {
  Serial.println("=== ORIENTATION VERS LE NORD ===");
  Serial.print("Cap cible utilise comme Nord : ");
  Serial.print(capNordCibleDeg, 1);
  Serial.println(" deg");

  unsigned long debut = millis();
  unsigned long dernierLog = 0;

  while (millis() - debut < 12000) {
    mettreAJourCapteursEtOdometrie();
    float erreur = normaliserAngle180(capDeg - capNordCibleDeg);

    if (abs(erreur) <= TOLERANCE_CAP_NORD_DEG) {
      Serial.println("Cap cible atteint.");
      break;
    }

    int vitesse = 112;
    if (abs(erreur) < 3.0f) vitesse = 92;

    float commande = SENS_CORRECTION_NORD * erreur;
    if (commande > 0.0f) {
      setMoteurDroit(vitesse, false);
      setMoteurGauche(vitesse, false);
    } else {
      setMoteurDroit(vitesse, true);
      setMoteurGauche(vitesse, true);
    }

    if (millis() - dernierLog > 250) {
      dernierLog = millis();
      Serial.print("Cap mesure : ");
      Serial.print(capDeg, 2);
      Serial.print(" deg | Erreur : ");
      Serial.print(erreur, 2);
      Serial.print(" deg | Vitesse : ");
      Serial.println(vitesse);
    }

    delay(15);
  }

  arreter();
  freiner();
  float capFinal = lireCapMoyenDeg();
  Serial.print("Cap final mesure : ");
  Serial.print(capFinal, 1);
  Serial.println(" deg");
  Serial.println("=== FIN ORIENTATION NORD ===");
}

void tournerStyloAngle(float angleDeg) {
  int vitesseStylo = 145;
  int nbCycles = max(1, (int)(25.0f * abs(angleDeg) / 90.0f));

  for (int i = 0; i < nbCycles; i++) {
    compteurD = 0;
    compteurG = 0;

    if (angleDeg > 0) {
      setMoteurDroit(vitesseStylo, false);
      setMoteurGauche(vitesseStylo, false);
      while (compteurD < 2 || compteurG < 1) {
        if (compteurD >= 2) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
        if (compteurG >= 1) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
      }
    } else {
      setMoteurGauche(vitesseStylo, true);
      setMoteurDroit(vitesseStylo, true);
      while (compteurG < 2 || compteurD < 1) {
        if (compteurG >= 2) { ledcWrite(CH_IN1_G, 0); ledcWrite(CH_IN2_G, 0); }
        if (compteurD >= 1) { ledcWrite(CH_IN1_D, 0); ledcWrite(CH_IN2_D, 0); }
      }
    }

    arreter();
    delay(4);
  }

  freiner();
}

void avancerTraitCourt(float longueurMm) {
  mouvementRouesMm(longueurMm, longueurMm, 105);
  freiner();
}

void tracerVecteurStylo(float avanceMm, float gaucheMm) {
  float maxMm = max(abs(avanceMm), abs(gaucheMm));
  int steps = max(8, (int)(maxMm / 2.5f));

  for (int i = 0; i < steps; i++) {
    float dAvance = avanceMm / (float)steps;
    float dGauche = gaucheMm / (float)steps;

    // Vitesse laterale du stylo obtenue par petite rotation du robot.
    // dGauche positif = trait vers la gauche du robot.
    float dTheta = dGauche / OFFSET_STYLO_MM;
    float distD = dAvance + (ENTRAXE_REEL_MM / 2.0f) * dTheta;
    float distG = dAvance - (ENTRAXE_REEL_MM / 2.0f) * dTheta;

    mouvementRouesMm(distD, distG, 105);
  }

  freiner();
}

void dessinerContourPointeFleche() {
  Serial.println("--- CONTOUR POINTE FLECHE ---");

  // Depart : haut de la tige, robot oriente vers le Nord.
  // Cote gauche : base gauche puis diagonale vers la pointe.
  tracerVecteurStylo(0.0f, 40.0f);
  tracerVecteurStylo(40.0f, -40.0f);

  // Retour au centre de la base de la pointe.
  reculerCalibrationPID(40.0f);

  // Cote droit : base droite puis diagonale vers la pointe.
  tracerVecteurStylo(0.0f, -40.0f);
  tracerVecteurStylo(40.0f, 40.0f);

  // Retour au centre de la base, pret pour le coloriage.
  reculerCalibrationPID(40.0f);

  Serial.println("--- FIN CONTOUR POINTE ---");
}

void remplirPointeFleche() {
  const int nbTraits = 13;
  const float longueurBaseMm = 25.0f;
  const float longueurPointeMm = 5.0f;
  const float avanceEntreTraitsMm = 1.7f;

  Serial.println("--- REMPLISSAGE POINTE FLECHE ---");

  for (int i = 0; i < nbTraits; i++) {
    float t = (float)i / (float)(nbTraits - 1);
    float longueur = longueurBaseMm + (longueurPointeMm - longueurBaseMm) * t;
    float demiLongueur = longueur / 2.0f;
    bool sensGaucheDroite = (i % 2 == 0);

    // Trait centre sur l'axe de la tige :
    // meme longueur a gauche et a droite, puis retour au milieu.
    if (sensGaucheDroite) {
      tracerVecteurStylo(0.0f, demiLongueur);
      tracerVecteurStylo(0.0f, -longueur);
      tracerVecteurStylo(0.0f, demiLongueur);
    } else {
      tracerVecteurStylo(0.0f, -demiLongueur);
      tracerVecteurStylo(0.0f, longueur);
      tracerVecteurStylo(0.0f, -demiLongueur);
    }

    if (i < nbTraits - 1) {
      tracerVecteurStylo(avanceEntreTraitsMm, 0.0f);
    }
  }

  freiner();
  Serial.println("--- FIN REMPLISSAGE POINTE ---");
}

void dessinerFlecheNord() {
  Serial.println("=== DEBUT SEQUENCE 3 : FLECHE NORD ===");

  if (!magOK) {
    Serial.println("Sequence annulee : magnetometre indisponible.");
    return;
  }

  orienterRobotVersNord();

  Serial.println("[1/2] Trace de la tige : 4 cm");
  avancerCalibrationPID(40.0f);
  delay(200);

  Serial.println("[2/2] Remplissage de la pointe");
  remplirPointeFleche();

  freiner();
  Serial.println("=== FIN SEQUENCE 3 : FLECHE NORD ===");
}
