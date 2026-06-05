#include "Robot.h"

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

