#include "Robot.h"

void IRAM_ATTR isr_enc_G() {
  compteurG++;
  totalCompteurG += signeEncodeurG;
}
void IRAM_ATTR isr_enc_D() {
  compteurD++;
  totalCompteurD += signeEncodeurD;
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

