#ifndef BOARD320_240_CPP
#define BOARD320_240_CPP

#include <analogWrite.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "BoardInterface.h"
#include "Board320_240.h"

/**
   Init board
*/
void Board320_240::initBoard() {

  // Set button pins for input
  pinMode(pinButtonMiddle, INPUT);
  pinMode(pinButtonLeft, INPUT);
  pinMode(pinButtonRight, INPUT);

  // Init display
  Serial.println("Init tft display");
  tft.begin();
  tft.invertDisplay(invertDisplay);
  tft.setRotation(liveData->settings.displayRotation);
  setBrightness((liveData->settings.lcdBrightness == 0) ? 100 : liveData->settings.lcdBrightness);
  tft.fillScreen(TFT_BLACK);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  spr.setColorDepth((psramUsed) ? 16 : 8);
  spr.createSprite(320, 240);
}

/**
   After setup device
*/
void Board320_240::afterSetup() {
  // Show test data on right button during boot device
  displayScreen = liveData->settings.defaultScreen;
  if (digitalRead(pinButtonRight) == LOW) {
    loadTestData();
  }
}

/**
   Set brightness level
*/
void Board320_240::setBrightness(byte lcdBrightnessPerc) {

  analogWrite(TFT_BL, lcdBrightnessPerc);
}

/**
   Clear screen a display two lines message
*/
void Board320_240::displayMessage(const char* row1, const char* row2) {

  // Must draw directly, withou sprite (due to psramFound check)
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(row1, 0, 240 / 2, GFXFF);
  tft.drawString(row2, 0, (240 / 2) + 30, GFXFF);
}

/**
  Draw cell on dashboard
*/
void Board320_240::drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, uint16_t bgColor, uint16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg color
  spr.setTextSize(1); // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2) {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(tmpStr3, "-%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextDatum(TL_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    posx = ((x + w) * 80) - 8;
    sprintf(tmpStr3, "+%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
    spr.setTextDatum(TR_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    spr.setTextColor(fgColor, bgColor);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.drawString(text, posx, posy, 7);

  } else {

    // All others 1x1 cells
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(fgColor, bgColor);
    spr.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    spr.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
  }
}

/**
  Draw small rect 80x30
*/
void Board320_240::drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t bgColor, int16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 32) + 1;

  spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 32) - 1, h * 32, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg bgColor
  spr.setTextSize(1); // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(fgColor, bgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  spr.drawString(text, posx, posy + 14, 2);
}

/**
  Show tire pressures / temperatures
  Custom field
*/
void Board320_240::showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char* topleft, const char* topright, const char* bottomleft, const char* bottomright, uint16_t color) {

  int32_t posx, posy;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  color);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER, color);
  spr.setTextSize(1);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  spr.drawString(topleft, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomleft, posx, posy, 2);

  spr.setTextDatum(TR_DATUM);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  spr.drawString(topright, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomright, posx, posy, 2);
}

/**
   Main screen (Screen 1)
*/
void Board320_240::drawSceneMain() {

  // Tire pressure
  char pressureStr[4] = "bar";
  char temperatureStr[2] = "C";
  if (liveData->settings.pressureUnit != 'b')
    strcpy(pressureStr, "psi");
  if (liveData->settings.temperatureUnit != 'c')
    strcpy(temperatureStr, "F");
  sprintf(tmpStr1, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireFrontLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireFrontLeftTempC), temperatureStr);
  sprintf(tmpStr2, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireFrontRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireFrontRightPressureBar), pressureStr);
  sprintf(tmpStr3, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireRearLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireRearLeftTempC), temperatureStr);
  sprintf(tmpStr4, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireRearRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireRearRightPressureBar), pressureStr);
  showTires(1, 0, 2, 1, tmpStr1, tmpStr2, tmpStr3, tmpStr4, TFT_BLACK);

  // Added later - kwh total in tires box
  // TODO: refactoring
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_GREEN, TFT_BLACK);
  sprintf(tmpStr1, "C: %01.01f +%01.01fkWh", liveData->params.cumulativeEnergyChargedKWh, liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 30, 2);
  spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  sprintf(tmpStr1, "D: %01.01f -%01.01fkWh", liveData->params.cumulativeEnergyDischargedKWh, liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 44, 2);

  // batPowerKwh100 on roads, else batPowerAmp
  if (liveData->params.speedKmh > 20) {
    sprintf(tmpStr1, "%01.01f", liveData->km2distance(liveData->params.batPowerKwh100));
    drawBigCell(1, 1, 2, 2, tmpStr1, ((liveData->settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (liveData->params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  } else {
    // batPowerAmp on chargers (under 10kmh)
    sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
    drawBigCell(1, 1, 2, 2, tmpStr1, "POWER KW", (liveData->params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }

  // socPerc
  sprintf(tmpStr1, "%01.00f%%", liveData->params.socPerc);
  sprintf(tmpStr2, (liveData->params.sohPerc ==  100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), liveData->params.sohPerc);
  drawBigCell(0, 0, 1, 1, tmpStr1, tmpStr2, (liveData->params.socPerc < 10 || liveData->params.sohPerc < 100 ? TFT_RED : (liveData->params.socPerc  > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);

  // batPowerAmp
  sprintf(tmpStr1, (abs(liveData->params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.batPowerAmp);
  drawBigCell(0, 1, 1, 1, tmpStr1, "CURRENT A", (liveData->params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // batVoltage
  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  drawBigCell(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);

  // batCellMinV
  sprintf(tmpStr1, "%01.02f", liveData->params.batCellMaxV - liveData->params.batCellMinV);
  sprintf(tmpStr2, "CELLS %01.02f", liveData->params.batCellMinV);
  drawBigCell(0, 3, 1, 1, ( liveData->params.batCellMaxV - liveData->params.batCellMinV == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);

  // batTempC
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batMinC));
  sprintf(tmpStr2, ((liveData->settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), liveData->celsius2temperature(liveData->params.batMaxC));
  drawBigCell(1, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  // batHeaterC
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batHeaterC));
  drawBigCell(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);

  // Aux perc
  sprintf(tmpStr1, "%01.00f%%", liveData->params.auxPerc);
  drawBigCell(3, 0, 1, 1, tmpStr1, "AUX BAT.", (liveData->params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);

  // Aux amp
  sprintf(tmpStr1, (abs(liveData->params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.auxCurrentAmp);
  drawBigCell(3, 1, 1, 1, tmpStr1, "AUX AMPS",  (liveData->params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // auxVoltage
  sprintf(tmpStr1, "%01.01f", liveData->params.auxVoltage);
  drawBigCell(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (liveData->params.auxVoltage < 12.1 ? TFT_RED : (liveData->params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);

  // indoorTemperature
  sprintf(tmpStr1, "%01.01f", liveData->celsius2temperature(liveData->params.indoorTemperature));
  sprintf(tmpStr2, "IN/OUT%01.01f", liveData->celsius2temperature(liveData->params.outdoorTemperature));
  drawBigCell(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);
}

/**
   Speed + kwh/100km (Screen 2)
*/
void Board320_240::drawSceneSpeed() {

  int32_t posx, posy;

  // HUD
  if (displayScreenSpeedHud) {

    // Change rotation to vertical & mirror
    if (tft.getRotation() != 6) {
      tft.setRotation(6);
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TR_DATUM); // top-right alignment
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // foreground, background text color

    // Draw speed
    tft.setTextSize((liveData->params.speedKmh > 99) ? 1 : 2);
    sprintf(tmpStr3, "0");
    if (liveData->params.speedKmh > 10)
      sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmh));
    tft.drawString(tmpStr3, 240, 0, 8);

    // Draw power kWh/100km (>25kmh) else kW
    tft.setTextSize(1);
    if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0)
      sprintf(tmpStr3, "%01.01f", liveData->km2distance(liveData->params.batPowerKwh100));
    else
      sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
    tft.drawString(tmpStr3, 240, 150, 8);

    // Draw soc%
    sprintf(tmpStr3, "%01.00f", liveData->params.socPerc);
    tft.drawString(tmpStr3, 240 , 230, 8);

    // Cold gate cirlce
    tft.fillCircle(30, 280, 25, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);

    // Brake lights
    tft.fillRect(0, 310, 240, 10, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

    return;
  }

  //
  spr.fillRect(0, 36, 200, 160, TFT_DARKRED);

  posx = 320 / 2;
  posy = 40;
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_DARKRED);
  spr.setTextSize(2); // Size for small 5cix7 font
  sprintf(tmpStr3, "0");
  if (liveData->params.speedKmh > 10)
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmh));
  spr.drawString(tmpStr3, 200, posy, 7);

  posy = 145;
  spr.setTextDatum(TR_DATUM); // Top center
  spr.setTextSize(1);
  if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0) {
    sprintf(tmpStr3, "%01.01f", liveData->km2distance(liveData->params.batPowerKwh100));
  } else {
    sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
  }
  spr.drawString(tmpStr3, 200, posy, 7);

  // Bottom 2 numbers with charged/discharged kWh from start
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  posx = 5;
  posy = 5;
  spr.setTextDatum(TL_DATUM);
  sprintf(tmpStr3, ((liveData->settings.distanceUnit == 'k') ? "%01.00fkm  " : "%01.00fmi  "), liveData->km2distance(liveData->params.odoKm));
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  if (liveData->params.motorRpm > -1) {
    spr.setTextDatum(TR_DATUM);
    sprintf(tmpStr3, "     %01.00frpm" , liveData->params.motorRpm);
    spr.drawString(tmpStr3, 320 - posx, posy, GFXFF);
  }

  // Bottom info
  // Cummulative regen/power
  posy = 240 - 5;
  sprintf(tmpStr3, "-%01.01f    ", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  spr.setTextDatum(BL_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  posx = 320 - 5;
  sprintf(tmpStr3, "    +%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.setTextDatum(BR_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  // Bat.power
  posx = 320 / 2;
  sprintf(tmpStr3, "   %01.01fkw   ", liveData->params.batPowerKw);
  spr.setTextDatum(BC_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);

  // RIGHT INFO
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  spr.fillCircle(290, 60, 25, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  spr.setTextColor(TFT_WHITE, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextDatum(MC_DATUM);
  sprintf(tmpStr3, "%01.00f", liveData->celsius2temperature(liveData->params.batTempC));
  spr.drawString(tmpStr3, 290, 60, GFXFF);
  // Brake lights
  spr.fillRect(210, 40, 40, 40, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

  // Soc%, bat.kWh
  spr.setFreeFont(&Orbitron_Light_32);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setTextDatum(TR_DATUM);
  sprintf(tmpStr3, " %01.00f%%", liveData->params.socPerc);
  spr.drawString(tmpStr3, 320, 94, GFXFF);
  if (liveData->params.socPerc > 0) {
    float capacity = liveData->params.batteryTotalAvailableKWh * (liveData->params.socPerc / 100);
    // calibration for Niro/Kona, real available capacity is ~66.5kWh, 0-10% ~6.2kWh, 90-100% ~7.2kWh
    if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64) {
      capacity = (liveData->params.socPerc * 0.615) * (1 + (liveData->params.socPerc * 0.0008));
    }
    sprintf(tmpStr3, " %01.01f", capacity);
    spr.drawString(tmpStr3, 320, 129, GFXFF);
    spr.drawString("kWh", 320, 164, GFXFF);
  }
}

/**
   Battery cells (Screen 3)
*/
void Board320_240::drawSceneBatteryCells() {

  int32_t posx, posy;

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batHeaterC));
  drawSmallCell(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batInletC));
  drawSmallCell(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batModuleTempC[0]));
  drawSmallCell(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batModuleTempC[1]));
  drawSmallCell(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batModuleTempC[2]));
  drawSmallCell(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batModuleTempC[3]));
  drawSmallCell(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  // Ioniq (up to 12 cells)
  for (uint16_t i = 4; i < liveData->params.batModuleTempCount; i++) {
    if (liveData->params.batModuleTempC[i] == 0)
      continue;
    posx = (((i - 4) % 8) * 40);
    posy = ((floor((i - 4) / 8)) * 13) + 64;
    //spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
    spr.setTextSize(1); // Size for small 5x7 font
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(((liveData->params.batModuleTempC[i] >= 15) ? ((liveData->params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED), TFT_BLACK);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00fC" : "%01.01fF"), liveData->celsius2temperature(liveData->params.batModuleTempC[i]));
    spr.drawString(tmpStr1, posx + 4, posy, 2);
  }

  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextSize(1); // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < 98; i++) {
    if ((liveData->params.cellVoltage[i] < minVal || minVal == -1) && liveData->params.cellVoltage[i] != -1)
      minVal = liveData->params.cellVoltage[i];
    if ((liveData->params.cellVoltage[i] > maxVal || maxVal == -1) && liveData->params.cellVoltage[i] != -1)
      maxVal = liveData->params.cellVoltage[i];
    if (liveData->params.cellVoltage[i] > 0 && i > liveData->params.cellCount + 1)
      liveData->params.cellCount = i + 1;
  }

  // Draw cell matrix
  for (int i = 0; i < 98; i++) {
    if (liveData->params.cellVoltage[i] == -1)
      continue;
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8) + (liveData->params.cellCount > 96 ? 0 : 1)) * 13) + 68;
    sprintf(tmpStr3, "%01.02f", liveData->params.cellVoltage[i]);
    spr.setTextColor(TFT_NAVY, TFT_BLACK);
    if (liveData->params.cellVoltage[i] == minVal && minVal != maxVal)
      spr.setTextColor(TFT_RED, TFT_BLACK);
    if (liveData->params.cellVoltage[i] == maxVal && minVal != maxVal)
      spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.drawString(tmpStr3, posx, posy, 2);
  }
}

/**
   drawPreDrawnChargingGraphs
   P = U.I
*/
void Board320_240::drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, int mulY) {

  // Rapid gate
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_RAPIDGATE35);
  // Coldgate <5C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE0_5);
  // Coldgate 5-14C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 40 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  // Coldgate 15-24C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  // Optimal
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 51 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 51 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 53 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 53 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 55 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 55 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 71 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 71 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 73 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 73 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 75 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 75 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 100 * mulX), zeroY - (/*I*/ 15 * /*U SOC*/ (100 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  // Triangles
  int x = zeroX;
  int y;
  if (liveData->params.batMaxC >= 35) {
    y = zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 25) {
    y = zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 15) {
    y = zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 5) {
    y = zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else {
    y = zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  spr.fillTriangle(x + 5, y,  x , y - 5, x, y + 5, TFT_ORANGE);
}

/**
   Charging graph (Screen 4)
*/
void Board320_240::drawSceneChargingGraph() {

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3; // 100% = 300px;
  int mulY = 2; // 100kW = 200px
  int maxKw = 80;
  int posy = 0;
  uint16_t color;

  spr.fillSprite(TFT_BLACK);

  sprintf(tmpStr1, "%01.00f", liveData->params.socPerc);
  drawSmallCell(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
  drawSmallCell(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerAmp);
  drawSmallCell(2, 0, 1, 1, tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  drawSmallCell(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batHeaterC));
  drawSmallCell(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batInletC));
  drawSmallCell(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.batMinC));
  drawSmallCell(2, 1, 1, 1, tmpStr1, "BAT.MIN", (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), liveData->celsius2temperature(liveData->params.outdoorTemperature));
  drawSmallCell(3, 1, 1, 1, tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  spr.setTextColor(TFT_SILVER, TFT_TEMP);

  for (int i = 0; i <= 10; i++) {
    color = TFT_DARKRED2;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_DARKRED;
    spr.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    /*if (i != 0 && i != 10) {
      sprintf(tmpStr1, "%d%%", i * 10);
      spr.setTextDatum(BC_DATUM);
      spr.drawString(tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
      }*/
    if (i <= (maxKw / 10)) {
      spr.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0) {
        sprintf(tmpStr1, "%d", i * 10);
        spr.setTextDatum(ML_DATUM);
        spr.drawString(tmpStr1, zeroX + (100 * mulX) + 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }

  // Draw suggested curves
  if (liveData->settings.predrawnChargingGraphs == 1) {
    drawPreDrawnChargingGraphs(zeroX, zeroY, mulX, mulY);
  }

  // Draw realtime values
  for (int i = 0; i <= 100; i++) {
    if (liveData->params.chargingGraphBatMinTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMinTempC[i]*mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphBatMaxTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMaxTempC[i]*mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphWaterCoolantTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphWaterCoolantTempC[i]*mulY), mulX, TFT_PURPLE);
    if (liveData->params.chargingGraphHeaterTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphHeaterTempC[i]*mulY), mulX, TFT_RED);

    if (liveData->params.chargingGraphMinKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMinKw[i]*mulY), mulX, TFT_GREENYELLOW);
    if (liveData->params.chargingGraphMaxKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMaxKw[i]*mulY), mulX, TFT_YELLOW);
  }

  // Bat.module temperatures
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextDatum(BL_DATUM);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "1=%01.00fC " : "1=%01.00fF "), liveData->celsius2temperature(liveData->params.batModuleTempC[0]));
  spr.setTextColor((liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 0,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "2=%01.00fC " : "2=%01.00fF "), liveData->celsius2temperature(liveData->params.batModuleTempC[1]));
  spr.setTextColor((liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 48,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "3=%01.00fC " : "3=%01.00fF "), liveData->celsius2temperature(liveData->params.batModuleTempC[2]));
  spr.setTextColor((liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 96,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "4=%01.00fC " : "4=%01.00fF "), liveData->celsius2temperature(liveData->params.batModuleTempC[3]));
  spr.setTextColor((liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 144,  zeroY - (maxKw * mulY), 2);
  sprintf(tmpStr1, "ir %01.00fkOhm", liveData->params.isolationResistanceKOhm );

  // Bms max.regen/power available
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xC=%01.00fkW ", liveData->params.availableChargePower);
  spr.drawString(tmpStr1, 192,  zeroY - (maxKw * mulY), 2);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xD=%01.00fkW", liveData->params.availableDischargePower);
  spr.drawString(tmpStr1, 256,  zeroY - (maxKw * mulY), 2);

  //
  spr.setTextDatum(TR_DATUM);
  if (liveData->params.coolingWaterTempC != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "W=%01.00fC" : "W=%01.00fF"), liveData->celsius2temperature(liveData->params.coolingWaterTempC));
    spr.setTextColor(TFT_PURPLE, TFT_TEMP);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  spr.setTextColor(TFT_WHITE, TFT_TEMP);
  if (liveData->params.batFanFeedbackHz > 0) {
    sprintf(tmpStr1, "FF=%03.00fHz", liveData->params.batFanFeedbackHz);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.batFanStatus > 0) {
    sprintf(tmpStr1, "FS=%03.00f", liveData->params.batFanStatus);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.coolantTemp1C != -1 && liveData->params.coolantTemp2C != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C1/2:%01.00f/%01.00fC" : "C1/2:%01.00f/%01.00fF"), liveData->celsius2temperature(liveData->params.coolantTemp1C), liveData->celsius2temperature(liveData->params.coolantTemp2C));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempA != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "A=%01.00fC" : "W=%01.00fF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempA));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempB != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "B=%01.00fC" : "W=%01.00fF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempB));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempC != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C=%01.00fC" : "W=%01.00fF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempC));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempD != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "D=%01.00fC" : "W=%01.00fF"), liveData->celsius2temperature(liveData->params.bmsUnknownTempD));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }

  // Print charging time
  time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(tmpStr1, "%02d:%02d:%02d", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(tmpStr1, "%02d:%02d", (diffTime / 60), diffTime % 60);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER, TFT_BLACK);
  spr.drawString(tmpStr1, 0, zeroY - (maxKw * mulY), 2);
}

/**
  SOC 10% table (screen 5)
*/
void Board320_240::drawSceneSoc10Table() {

  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;

  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER, TFT_TEMP);
  spr.setTextDatum(TL_DATUM);
  spr.drawString("CONSUMPTION | DISCH.100%->4% SOC",  2, zeroY, 2);

  spr.setTextDatum(TR_DATUM);

  spr.drawString("dis./char.kWh", 128, zeroY + (1 * 15), 2);
  spr.drawString(((liveData->settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15), 2);
  spr.drawString("kWh100", 224, zeroY + (1 * 15), 2);
  spr.drawString("avg.speed", 310, zeroY + (1 * 15), 2);

  for (int i = 0; i <= 10; i++) {
    sprintf(tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    spr.drawString(tmpStr1, 32, zeroY + ((12 - i) * 15), 2);

    firstCed = (liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : lastCed;
    firstCec = (liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : lastCec;
    firstOdo = (liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : lastOdo;

    if (i != 10) {
      diffCec = (liveData->params.soc10cec[i + 1] != -1 && liveData->params.soc10cec[i] != -1) ? (liveData->params.soc10cec[i] - liveData->params.soc10cec[i + 1]) : 0;
      diffCed = (liveData->params.soc10ced[i + 1] != -1 && liveData->params.soc10ced[i] != -1) ? (liveData->params.soc10ced[i + 1] - liveData->params.soc10ced[i]) : 0;
      diffOdo = (liveData->params.soc10odo[i + 1] != -1 && liveData->params.soc10odo[i] != -1) ? (liveData->params.soc10odo[i] - liveData->params.soc10odo[i + 1]) : -1;
      diffTime = (liveData->params.soc10time[i + 1] != -1 && liveData->params.soc10time[i] != -1) ? (liveData->params.soc10time[i] - liveData->params.soc10time[i + 1]) : -1;
      if (diffCec != 0) {
        sprintf(tmpStr1, "+%01.01f", diffCec);
        spr.drawString(tmpStr1, 128, zeroY + ((12 - i) * 15), 2);
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0) {
        sprintf(tmpStr1, "%01.01f", diffCed);
        spr.drawString(tmpStr1, 80, zeroY + ((12 - i) * 15), 2);
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1) {
        sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
        spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0) {
          sprintf(tmpStr1, "%01.01f", liveData->km2distance(diffOdo) / (diffTime / 3600));
          spr.drawString(tmpStr1, 310, zeroY + ((12 - i) * 15), 2);
        }
      }
      if (diffOdo > 0 && diffCed != 0) {
        sprintf(tmpStr1, "%01.1f", (-diffCed * 100.0 / liveData->km2distance(diffOdo)));
        spr.drawString(tmpStr1, 224, zeroY + ((12 - i) * 15), 2);
      }
    }

    if (diffOdo == -1 && liveData->params.soc10odo[i] != -1) {
      sprintf(tmpStr1, "%01.00f", liveData->km2distance(liveData->params.soc10odo[i]));
      spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
    }
  }

  spr.drawString("0%", 32, zeroY + (13 * 15), 2);
  spr.drawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15), 2);

  spr.drawString("TOT.", 32, zeroY + (14 * 15), 2);
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(tmpStr1, "%01.01f", diffCed);
  spr.drawString(tmpStr1, 80, zeroY + (14 * 15), 2);
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(tmpStr1, "+%01.01f", diffCec);
  spr.drawString(tmpStr1, 128, zeroY + (14 * 15), 2);
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
  spr.drawString(tmpStr1, 160, zeroY + (14 * 15), 2);
  sprintf(tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  spr.drawString(tmpStr1, 310, zeroY + (14 * 15), 2);
}

/**
  DEBUG screen
*/
void Board320_240::drawSceneDebug() {

  int32_t posx, posy;
  String chHex, chHex2;
  uint8_t chByte;

  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER, TFT_TEMP);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(debugAtshRequest, 0, 0, 2);
  spr.drawString(debugCommandRequest, 128, 0, 2);
  spr.drawString(liveData->commandRequest, 256, 0, 2);
  spr.setTextDatum(TR_DATUM);

  for (int i = 0; i < debugLastString.length() / 2; i++) {
    chHex = debugLastString.substring(i * 2, (i * 2) + 2);
    chHex2 = debugPreviousString.substring(i * 2, (i * 2) + 2);
    spr.setTextColor(((chHex.equals(chHex2)) ?  TFT_SILVER : TFT_GREEN), TFT_TEMP);
    chByte = liveData->hexToDec(chHex.c_str(), 1, false);
    posx = (((i) % 10) * 32) + 24;
    posy = ((floor((i) / 10)) * 32) + 24;
    sprintf(tmpStr1, "%03d", chByte);
    spr.drawString(tmpStr1, posx + 4, posy, 2);

    spr.setTextColor(TFT_YELLOW, TFT_TEMP);
    sprintf(tmpStr1, "%c", (char)chByte);
    spr.drawString(tmpStr1, posx + 4, posy + 13, 2);
  }

  debugPreviousString = debugLastString;
}

/**
   Modify caption
*/
String Board320_240::menuItemCaption(int16_t menuItemId, String title) {

  String prefix = "", suffix = "";

  if (menuItemId == 10) // Version
    suffix = APP_VERSION;

  if (menuItemId == 401) // distance
    suffix = (liveData->settings.distanceUnit == 'k') ? "[km]" : "[mi]";
  if (menuItemId == 402) // temperature
    suffix = (liveData->settings.temperatureUnit == 'c') ? "[C]" : "[F]";
  if (menuItemId == 403) // pressure
    suffix = (liveData->settings.pressureUnit == 'b') ? "[bar]" : "[psi]";

  title = ((prefix == "") ? "" : prefix + " ") + title + ((suffix == "") ? "" : " " + suffix);

  return title;
}

/**
  Display menu
*/
void Board320_240::showMenu() {

  uint16_t posY = 0, tmpCurrMenuItem = 0;

  liveData->menuVisible = true;
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(TL_DATUM);
  spr.setFreeFont(&Roboto_Thin_24);

  // Page scroll
  uint8_t visibleCount = (int)(tft.height() / spr.fontHeight());
  if (liveData->menuItemSelected >= liveData->menuItemOffset + visibleCount)
    liveData->menuItemOffset = liveData->menuItemSelected - visibleCount + 1;
  if (liveData->menuItemSelected < liveData->menuItemOffset)
    liveData->menuItemOffset = liveData->menuItemSelected;

  // Print items
  for (uint16_t i = 0; i < liveData->menuItemsCount; ++i) {
    if (liveData->menuCurrent == liveData->menuItems[i].parentId) {
      if (tmpCurrMenuItem >= liveData->menuItemOffset) {
        spr.fillRect(0, posY, 320, spr.fontHeight() + 2, (liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        spr.setTextColor((liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_WHITE : TFT_WHITE, (liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        spr.drawString(menuItemCaption(liveData->menuItems[i].id, liveData->menuItems[i].title), 0, posY + 2, GFXFF);
        posY += spr.fontHeight();
      }
      tmpCurrMenuItem++;
    }
  }

  spr.pushSprite(0, 0);
}

/**
   Hide menu
*/
void Board320_240::hideMenu() {

  liveData->menuVisible = false;
  liveData->menuCurrent = 0;
  liveData->menuItemSelected = 0;
  redrawScreen();
}

/**
  Move in menu with left/right button
*/
void Board320_240::menuMove(bool forward) {

  if (forward) {
    uint16_t tmpCount = 0;
    for (uint16_t i = 0; i < liveData->menuItemsCount; ++i) {
      if (liveData->menuCurrent == liveData->menuItems[i].parentId) {
        tmpCount++;
      }
    }
    liveData->menuItemSelected = (liveData->menuItemSelected >= tmpCount - 1 ) ? tmpCount - 1 : liveData->menuItemSelected + 1;
  } else {
    liveData->menuItemSelected = (liveData->menuItemSelected <= 0) ? 0 : liveData->menuItemSelected - 1;
  }
  showMenu();
}

/**
   Enter menu item
*/
void Board320_240::menuItemClick() {

  // Locate menu item for meta data
  MENU_ITEM tmpMenuItem;
  uint16_t tmpCurrMenuItem = 0;
  for (uint16_t i = 0; i < liveData->menuItemsCount; ++i) {
    if (liveData->menuCurrent == liveData->menuItems[i].parentId) {
      if (liveData->menuItemSelected == tmpCurrMenuItem) {
        tmpMenuItem = liveData->menuItems[i];
        break;
      }
      tmpCurrMenuItem++;
    }
  }

  // Exit menu, parent level menu, open item
  if (liveData->menuItemSelected == 0) {
    // Exit menu
    if (tmpMenuItem.parentId == 0 && tmpMenuItem.id == 0) {
      liveData->menuVisible = false;
      redrawScreen();
    } else {
      // Parent menu
      liveData->menuCurrent = tmpMenuItem.targetParentId;
      showMenu();
    }
    return;
  } else {
    Serial.println(tmpMenuItem.id);
    // Device list
    if (tmpMenuItem.id > 10000 && tmpMenuItem.id < 10100) {
      strlcpy((char*)liveData->settings.obdMacAddress, (char*)tmpMenuItem.obdMacAddress, 20);
      Serial.print("Selected adapter MAC address ");
      Serial.println(liveData->settings.obdMacAddress);
      saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem.id) {
      // Set vehicle type
      case 101: liveData->settings.carType = CAR_KIA_ENIRO_2020_64; break;
      case 102: liveData->settings.carType = CAR_HYUNDAI_KONA_2020_64; break;
      case 103: liveData->settings.carType = CAR_HYUNDAI_IONIQ_2018; break;
      case 104: liveData->settings.carType = CAR_KIA_ENIRO_2020_39; break;
      case 105: liveData->settings.carType = CAR_HYUNDAI_KONA_2020_39; break;
      case 106: liveData->settings.carType = CAR_RENAULT_ZOE; break;
      case 107: liveData->settings.carType = CAR_DEBUG_OBD2_KIA; break;
      // Screen orientation
      case 3011: liveData->settings.displayRotation = 1; tft.setRotation(liveData->settings.displayRotation); break;
      case 3012: liveData->settings.displayRotation = 3; tft.setRotation(liveData->settings.displayRotation); break;
      // Default screen
      case 3021: liveData->settings.defaultScreen = 1; break;
      case 3022: liveData->settings.defaultScreen = 2; break;
      case 3023: liveData->settings.defaultScreen = 3; break;
      case 3024: liveData->settings.defaultScreen = 4; break;
      case 3025: liveData->settings.defaultScreen = 5; break;
      // Debug screen off/on
      case 3031: liveData->settings.debugScreen = 0; break;
      case 3032: liveData->settings.debugScreen = 1; break;
      // Lcd brightness
      case 3041: liveData->settings.lcdBrightness = 0; break;
      case 3042: liveData->settings.lcdBrightness = 20; break;
      case 3043: liveData->settings.lcdBrightness = 50; break;
      case 3044: liveData->settings.lcdBrightness = 100; break;
      // Pre-drawn charg.graphs off/on
      case 3051: liveData->settings.predrawnChargingGraphs = 0; break;
      case 3052: liveData->settings.predrawnChargingGraphs = 1; break;
      // Distance
      case 4011: liveData->settings.distanceUnit = 'k'; break;
      case 4012: liveData->settings.distanceUnit = 'm'; break;
      // Temperature
      case 4021: liveData->settings.temperatureUnit = 'c'; break;
      case 4022: liveData->settings.temperatureUnit = 'f'; break;
      // Pressure
      case 4031: liveData->settings.pressureUnit = 'b'; break;
      case 4032: liveData->settings.pressureUnit = 'p'; break;
      // Pair ble device
      case 2: scanDevices = true; /*startBleScan(); */return;
      // Reset settings
      case 8: resetSettings(); hideMenu(); return;
      // Save settings
      case 9: saveSettings(); break;
      // Version
      case 10: hideMenu(); return;
      // Shutdown
      case 11: shutdownDevice(); return;
      default:
        // Submenu
        liveData->menuCurrent = tmpMenuItem.id;
        liveData->menuItemSelected = 0;
        showMenu();
        return;
    }

    // close menu
    hideMenu();
  }
}

/**
  Redraw screen
*/
void Board320_240::redrawScreen() {

  if (liveData->menuVisible) {
    return;
  }

  // Lights not enabled
  if (!testDataMode && liveData->params.forwardDriveMode && !liveData->params.headLights && !liveData->params.dayLights) {
    spr.fillSprite(TFT_RED);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.setTextColor(TFT_WHITE, TFT_RED);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("! LIGHTS OFF !", 160, 120, GFXFF);
    spr.pushSprite(0, 0);

    return;
  }

  spr.fillSprite(TFT_BLACK);

  // 1. Auto mode = >5kpm Screen 3 - speed, other wise basic Screen2 - Main screen, if charging then Screen 5 Graph
  if (displayScreen == SCREEN_AUTO) {
    if (liveData->params.speedKmh > 5) {
      if (displayScreenAutoMode != 3) {
        displayScreenAutoMode = 3;
      }
      drawSceneSpeed();
    } else if (liveData->params.batPowerKw > 1) {
      if (displayScreenAutoMode != 5) {
        displayScreenAutoMode = 5;
      }
      drawSceneChargingGraph();
    } else {
      if (displayScreenAutoMode != 2) {
        displayScreenAutoMode = 2;
      }
      drawSceneMain();
    }
  }
  // 2. Main screen
  if (displayScreen == SCREEN_DASH) {
    drawSceneMain();
  }
  // 3. Big speed + kwh/100km
  if (displayScreen == SCREEN_SPEED) {
    drawSceneSpeed();
  }
  // 4. Battery cells
  if (displayScreen == SCREEN_CELLS) {
    drawSceneBatteryCells();
  }
  // 5. Charging graph
  if (displayScreen == SCREEN_CHARGING) {
    drawSceneChargingGraph();
  }
  // 6. SOC10% table (CEC-CED)
  if (displayScreen == SCREEN_SOC10) {
    drawSceneSoc10Table();
  }
  // 7. DEBUG SCREEN
  if (displayScreen == SCREEN_DEBUG) {
    drawSceneDebug();
  }

  if (!displayScreenSpeedHud) {
    // BLE not connected
    if (!liveData->bleConnected && liveData->bleConnect) {
      // Print message
      spr.setTextSize(1);
      spr.setTextColor(TFT_WHITE, TFT_BLACK);
      spr.setTextDatum(TL_DATUM);
      spr.drawString("BLE4 OBDII not connected...", 0, 180, 2);
      spr.drawString("Press middle button to menu.", 0, 200, 2);
      spr.drawString(APP_VERSION, 0, 220, 2);
    }

    spr.pushSprite(0, 0);
  }
}

/**
   Parse test data
*/
void Board320_240::loadTestData() {

  testDataMode = true; // skip lights off message
  carInterface->loadTestData();
  redrawScreen();
}

/**
   Board looop
*/
void Board320_240::mainLoop() {

  ///////////////////////////////////////////////////////////////////////
  // Handle buttons
  // MIDDLE - menu select
  if (digitalRead(pinButtonMiddle) == HIGH) {
    btnMiddlePressed = false;
  } else {
    if (!btnMiddlePressed) {
      btnMiddlePressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      if (liveData->menuVisible) {
        menuItemClick();
      } else {
        showMenu();
      }
    }
  }
  // LEFT - screen rotate, menu
  if (digitalRead((liveData->settings.displayRotation == 1) ? pinButtonRight : pinButtonLeft) == HIGH) {
    btnLeftPressed = false;
  } else {
    if (!btnLeftPressed) {
      btnLeftPressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (liveData->menuVisible) {
        menuMove(false);
      } else {
        displayScreen++;
        if (displayScreen > displayScreenCount - (liveData->settings.debugScreen == 0) ? 1 : 0)
          displayScreen = 0; // rotate screens
        // Turn off display on screen 0
        setBrightness((displayScreen == SCREEN_BLANK) ? 0 : (liveData->settings.lcdBrightness == 0) ? 100 : liveData->settings.lcdBrightness);
        redrawScreen();
      }
    }
  }
  // RIGHT - menu, debug screen rotation
  if (digitalRead((liveData->settings.displayRotation == 1) ? pinButtonLeft : pinButtonRight) == HIGH) {
    btnRightPressed = false;
  } else {
    if (!btnRightPressed) {
      btnRightPressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (liveData->menuVisible) {
        menuMove(true);
      } else {
        // doAction
        if (displayScreen == SCREEN_SPEED) {
          displayScreenSpeedHud = !displayScreenSpeedHud;
          redrawScreen();
        }
        if (liveData->settings.debugScreen == 1 && displayScreen == SCREEN_DEBUG) {
          debugCommandIndex = (debugCommandIndex >= liveData->commandQueueCount) ? liveData->commandQueueLoopFrom : debugCommandIndex + 1;
          redrawScreen();
        }

      }
    }
  }

}

/**
 * skipAdapterScan
 */
bool Board320_240::skipAdapterScan() {
   return digitalRead(pinButtonMiddle) == LOW || digitalRead(pinButtonLeft) == LOW || digitalRead(pinButtonRight) == LOW;
}

#endif // BOARD320_240_CPP
