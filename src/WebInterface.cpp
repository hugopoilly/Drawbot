#include "Robot.h"

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
  html += "Choix : 14, 16, 18 ou 20 cm<br>";
  html += "Rayon actuel : <b>" + String(rayonCercleCm, 1) + " cm</b>";
  html += "</div>";

  html += "<form action='/seq2/lancer' method='get'>";
  html += "<input type='number' name='r' step='2' min='14' max='20' value='" + String(rayonCercleCm, 1) + "' ";
  html += "style='width:120px;padding:10px;font-size:1.1em;border-radius:8px;border:none;text-align:center;'><br><br>";
  html += "<button class='btn blue' type='submit'>Dessiner le cercle</button>";
  html += "</form>";
  html += "<a class='btn blue' href='/seq2/lancer?r=14'>14 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=16'>16 cm</a>";
  html += "<a class='btn blue' href='/seq2/lancer?r=18'>18 cm</a>";
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
  html += "<meta http-equiv='refresh' content='25;url=/seq2'>";
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


