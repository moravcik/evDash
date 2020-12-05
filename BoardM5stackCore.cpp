#ifndef BOARDM5STACKCORE_CPP
#define BOARDM5STACKCORE_CPP

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore.h"

/**
  Init board
*/
void BoardM5stackCore::initBoard() {

  invertDisplay = true;
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;
  pinSpeaker = SPEAKER_PIN;
  pinBrightness = TFT_BL;

  // Mute speaker
  //ledcWriteTone(TONE_PIN_CHANNEL, 0);
  digitalWrite(SPEAKER_PIN, 0);

  //
  Board320_240::initBoard();
}

void BoardM5stackCore::mainLoop() {

  Board320_240::mainLoop();
}

#endif // BOARDM5STACKCORE_CPP
