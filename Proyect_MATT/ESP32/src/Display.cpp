#include "Display.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
static unsigned long ultimaReinicializacionLcd = 0;

static String prepararLineaDisplay(String texto) {
  texto = texto.substring(0, 16);

  for (int i = 0; i < texto.length(); i++) {
    char c = texto[i];
    if (c < 32 || c > 126) texto.setCharAt(i, '?');
  }

  while (texto.length() < 16) texto += " ";
  return texto;
}

void iniciarDisplay() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  ultimaReinicializacionLcd = millis();
  delay(100);
}

void mostrarDisplay(String linea1, String linea2) {
  if (millis() - ultimaReinicializacionLcd > 15000) {
    lcd.init();
    lcd.backlight();
    ultimaReinicializacionLcd = millis();
  }

  linea1 = prepararLineaDisplay(linea1);
  linea2 = prepararLineaDisplay(linea2);

  lcd.setCursor(0, 0);
  lcd.print(linea1);

  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

void mostrarInicioDisplay() {
  static int puntos = 0;

  String linea2 = "Esperando";
  for (int i = 0; i < puntos; i++) linea2 += ".";

  puntos++;
  if (puntos > 3) puntos = 0;

  mostrarDisplay("MATT listo", linea2);
}

void mostrarErrorDisplay() {
  mostrarDisplay("ERROR", "Sin OK KL25Z");
}

void mostrarTerminadoDisplay() {
  mostrarDisplay("Comando listo", "Texto enviado");
}
