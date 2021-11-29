#include "Serial.h"
enum signalStates {SETUP, GAME, DISPLAY_BOARD, END_ROUND, INERT, RESOLVE};
byte signalState = INERT;
byte gameMode = SETUP;

enum blinkModes {NOT_SET, CLUSTER, PLAYER};
byte blinkMode = NOT_SET;

byte faceValues[6] {0, 1, 2, 3, 4, 5};
bool faceValuesVisible = false;
const Color colorCycle[6] = {RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA};

byte faceNeighbours[6] {0, 0, 0, 0, 0, 0};

byte numNeighbours = 0;
ServicePortSerial sp;

byte faceMatched[6] = { 0, 0, 0, 0, 0, 0};

Timer cycleTimer;

void setup() {
  randomize();
  sp.begin();
}

void loop() {


  numNeighbours = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      numNeighbours++;
    }
  }

  if (signalState < INERT) {
    sendLoop();
  } else if (INERT == signalState ) {
    inertLoop();
  } else if (RESOLVE == signalState) {
    resolveLoop();
  }

  switch (gameMode) {
    case SETUP:
      setupLoop();
      setupDisplayLoop();
      break;
    case GAME:
      gameLoop();
      gameDisplayLoop();
      break;
    case DISPLAY_BOARD:
      displayBoardLoop();
      gameDisplayLoop(); //for now just use game display loop
      break;
    case END_ROUND:
      endRoundLoop();
      endRoundDisplayLoop();
      break;
  }

  //dump button data
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonPressed();

  byte sendData = 0;
  FOREACH_FACE(f) {
    sendData = (signalState << 3);
    if (SETUP == gameMode) {
      sendData = sendData + numNeighbours;
    } else if (DISPLAY_BOARD == gameMode && faceValuesVisible) {
      sendData = sendData + faceValues[f];
    }
    setValueSentOnFace(sendData, f);
  }
}


void inertLoop() {
  bool sendReceived = false;
  byte val = 0;
  FOREACH_FACE(f) {
    if ( !sendReceived) {
      if (!isValueReceivedOnFaceExpired(f) ) {//a neighbor!
        val = getSignalState(getLastValueReceivedOnFace(f));
        if (val < INERT ) {//a neighbor saying SEND!
          sp.print("SEND Received - moving to mode ");
          sp.println(val);
          switch (val) {
            case SETUP:
              signalState = val;
              gameMode = val;
              faceValuesVisible = false;
              break;
            case GAME:
              changeModeToGame();
              break;
            case DISPLAY_BOARD:
              if (PLAYER == blinkMode) {
                sp.println("Acutally, this is a player piece, ignoring DISPLAY_BOARD");
                signalState = RESOLVE;
                return;
              }
              signalState = val;
              gameMode = val;
              faceValuesVisible = true;
              cycleTimer.set(5000);
              break;
            case END_ROUND:
              signalState = val;
              gameMode = val;
              faceValuesVisible = false;
              break;
          }
          sendReceived = true;
        } else {
          if (SETUP == gameMode) {
            faceNeighbours[f] = getPayload(getLastValueReceivedOnFace(f));
          }
        }
      } else {
        if (SETUP == gameMode) {
          faceNeighbours[f] = 0;
        }
      }
    }
  }
}

void sendLoop() {
  bool allSend = true;
  //look for neighbors who have not heard the GO news
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT) {//This neighbor doesn't know it's SEND time. Stay in SEND
        allSend = false;
      }
    }
  }
  if (allSend) {
    sp.println("All Resolve");
    signalState = RESOLVE;
  }
}

void resolveLoop() {
  bool allResolve = true;

  //look for neighbors who have not moved to RESOLVE
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) < INERT) {//This neighbor isn't in RESOLVE. Stay in RESOLVE
        allResolve = false;
      }
    }
  }
  if (allResolve) {
    sp.println("All INERT");
    signalState = INERT;
  }
}

void setupLoop() {

  if (numNeighbours > 1) {
    blinkMode = CLUSTER;
  } else {
    blinkMode = PLAYER;
  }

  if (buttonDoubleClicked()) {
    changeModeToGame();
  }

}

void changeModeToGame() {
  signalState = GAME;
  gameMode = GAME;
  for (byte i = 0; i < 6; i++) {
    faceMatched[i] = 0;
    faceValues[i]=i;
  }
  if (PLAYER == blinkMode) {
    faceValuesVisible = true;   
  } else {
    shuffleArray(faceValues, sizeof(faceValues));
    faceValuesVisible = false;
  }
}

void gameLoop() {
  if (buttonDoubleClicked()) {
    if ( PLAYER == blinkMode || faceValuesVisible) {
      signalState = SETUP;
      gameMode = SETUP;
    } else {
      signalState = DISPLAY_BOARD;
//      if (CLUSTER == blinkMode) {
        gameMode = DISPLAY_BOARD;
//      }
      cycleTimer.set(5000);
      faceValuesVisible = true;
    }
  } else if (GAME == gameMode && PLAYER == blinkMode) {

    byte foundFaces = 0;
    byte wrongFaces = 0;
    byte faceVal = 0;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        faceVal = getPayload(getLastValueReceivedOnFace(f));
        if (faceVal == faceValues[f]) {
          faceMatched[f] = 1;
          foundFaces++;
        } else {
          faceMatched[f] = 2;
          wrongFaces++;
        }
      } else {
        faceMatched[f] = 0;
      }
    }
    if (2 == foundFaces && 0 == wrongFaces) {
      signalState = END_ROUND;
      gameMode = END_ROUND;
    }

  }
}

void displayBoardLoop() {
  if (buttonDoubleClicked()) {
    signalState = SETUP;
    gameMode = SETUP;
  } else if (cycleTimer.isExpired()) {
    shuffleArray(faceValues, sizeof(faceValues));
    cycleTimer.set(5000);
  } 
}
void endRoundLoop() {
  if (buttonDoubleClicked()) {
    signalState = SETUP;
    gameMode = SETUP;
  }
}

void shuffleArray(byte arr[], size_t sizeArr) {

  for (int i = 0; i < sizeArr; i++)
  {
    int j = random(sizeArr - 1);
    byte t = arr[i];
    arr[i] = arr[j];
    arr[j] = t;
  }
}



byte getSignalState(byte data) {
  return ((data >> 3) );
}

byte getPayload(byte data) {
  return (data & 7);
}



void setupDisplayLoop() {
  if (CLUSTER == blinkMode) {
    setColor(BLUE);
  } else {
    setColor(GREEN);
  }
}

void gameDisplayLoop() {


  if (faceValuesVisible) {
    if (numNeighbours != 2 || CLUSTER==blinkMode) {
      drawValuesOnFaces(faceValues);
    } else {
      setColor(OFF);
      for (byte i = 0; i < 6; i++) {
        if (faceMatched[i] == 1) {
          setColorOnFace(GREEN, i);
        } else if (faceMatched[i] == 2) {
          setColorOnFace(RED, i);
        }
      }
    }
  } else {
    if (PLAYER == blinkMode) {
      setColor(YELLOW);
    } else {
      setColor(GREEN);
    }
  }
}




void endRoundDisplayLoop() {
  if (CLUSTER == blinkMode) {
    setColor(MAGENTA);
  } else {
    setColor(OFF);
    for (byte i = 0; i < 6; i++) {
      if (faceMatched[i] == 1) {
        setColorOnFace(GREEN, i);
      } else if (faceMatched[i] == 2) {
        setColorOnFace(RED, i);
      }
    }
  }
}

void drawValuesOnFaces(byte values[]) {
  FOREACH_FACE(f) {
    setColorOnFace(colorCycle[values[f]], f);
  }
}
