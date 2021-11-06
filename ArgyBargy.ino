#define MODE_SETUP 1
#define MODE_BOARD 2
#define MODE_BOARD_READY 3
#define MODE_PLAYER_SETUP 4
#define MODE_PLAYER 5
#define MODE_GAMEOVER 6
#define MODE_RESET 7

#define MATCH_SEED 32
#define START_RESET 40
#define END_RESET 41
#define NEIGHBOUR_SEED 42

byte mode = MODE_SETUP;
// byte neighbours = -1;

byte faceValues[] {0,1,2,3,4,5};

const Color colorCycle[6] = {RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA};
/*
const Color colorMidCycles[1] = {makeColorRGB( 255,   0,   0)};
makeColorRGB( 255,   127,   0),
makeColorRGB( 255,   255,   0),
makeColorRGB( 127,   255,   0),
makeColorRGB( 0,   255,   0),
makeColorRGB( 0,   255,   127),
makeColorRGB( 0,   255,   255),
makeColorRGB( 0,   127,   255),
makeColorRGB( 0,   0,   255),
makeColorRGB( 127,   0,   255),
makeColorRGB( 255,   0,   255),
makeColorRGB( 255,   0,   127)};*/



void setup() {
  // put your setup code here, to run once:
  randomize();
  setColor(YELLOW);
  setValueSentOnAllFaces(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (MODE_SETUP==mode) {
    setColor(YELLOW);
    bool neighboursReady = true;
    bool singlePair = false;
    bool setOfFour = false;
    bool validFormation = true;
    byte neighbours = 0;
    byte maxNeighbours = 0;
    FOREACH_FACE(f) {//check every face
      if (!isValueReceivedOnFaceExpired(f)) {
        neighbours++;
        byte val = getLastValueReceivedOnFace(f);
        if (NEIGHBOUR_SEED<=val) {
          val = val - NEIGHBOUR_SEED;
        }
        if(validFormation && neighboursReady) {
          if (0==val) {
            neighboursReady = false;
          }
          if (0<val && singlePair) { //ie we've already seen a neighbour with 1, and this is a different neighbour
              validFormation = false;          
          }
          if (1==val) {
            singlePair = true;
            maxNeighbours = 1;
          }
          if (2==val) { // 2 should be followed by a 3 and then a 2 (or preceeded by!)
            byte nextFace = (f+1) % FACE_COUNT;
            byte nextNextFace = (f+2) % FACE_COUNT;
            byte prevFace = (f+5) % FACE_COUNT;
            byte prevPrevFace = (f+4) % FACE_COUNT;
            if(
                (checkFaceIsValue(nextFace,3+NEIGHBOUR_SEED) && checkFaceIsValue(nextNextFace,2+NEIGHBOUR_SEED)) 
                || (checkFaceIsValue(prevFace,3+NEIGHBOUR_SEED) && checkFaceIsValue(prevPrevFace,2+NEIGHBOUR_SEED)) 
              ) {
              setOfFour = true;
              maxNeighbours = 3;
            } else {
              validFormation = false;
            }
          }
          if (3==val && !setOfFour) {//could be middle of 2-3-2, or a 3-3
            byte nextFace = (f+1) % FACE_COUNT;
            byte prevFace = (f+5) % FACE_COUNT;
            if(checkFaceIsValue(nextFace,2+NEIGHBOUR_SEED) && checkFaceIsValue(prevFace,2+NEIGHBOUR_SEED)) {
              setOfFour = true;
              maxNeighbours = 3;
            } else if(checkFaceIsValue(nextFace,3+NEIGHBOUR_SEED) || checkFaceIsValue(prevFace,3+NEIGHBOUR_SEED)) {
              setOfFour = true;
              maxNeighbours = 2;
            } else {
              validFormation = false;
            }            
          }
          if (3<val) {
            validFormation = false;
          }
        }
      }
    }
    if (maxNeighbours<neighbours) {
      validFormation = false;
    }
    setValueSentOnAllFaces(neighbours + NEIGHBOUR_SEED);
    if (validFormation && neighboursReady) {
      setColor(GREEN);
      if(0==neighbours) {
        mode=MODE_PLAYER_SETUP;
      } else {
        mode=MODE_BOARD;
      }
    }
  }
  if (MODE_BOARD==mode) {
    if (isAlone()) {
      mode=MODE_PLAYER_SETUP;
    } else {
      if(buttonSingleClicked()) {
        setupBoard();
      } else {
        bool neighbourSetup = false;
        FOREACH_FACE(f) {
          if (!isValueReceivedOnFaceExpired(f)) {
            byte val = getLastValueReceivedOnFace(f);
            if (val>=MATCH_SEED && val<MATCH_SEED+FACE_COUNT) {
              neighbourSetup = true;
            }
          }
        }
        if (neighbourSetup) {
          setupBoard();
        }
      }
    }    
  }
  if (MODE_PLAYER_SETUP==mode) {
//    setColor(BLUE);
    if (buttonSingleClicked()) {
      shuffleArray(faceValues); //random for now
      drawValuesOnFaces(faceValues);
    }
    //protoype
    byte matches = 0;
    byte firstMatch = -1;
    byte secondMatch = -1;
    bool failed = false;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        byte val = getLastValueReceivedOnFace(f);
        if (MATCH_SEED + faceValues[f] == val) {
          matches++;
        } else if (val>=MATCH_SEED && val<MATCH_SEED+FACE_COUNT) {
          failed = true;
        } else if (val>=START_RESET) {
          mode = MODE_SETUP;
          return;
        }
        
      }
    }
    if (2==matches && !failed) {
      mode=MODE_GAMEOVER;
      FOREACH_FACE(f) {
        if (isValueReceivedOnFaceExpired(f)) {
          setColorOnFace(GREEN, f);
        } else {
          setValueSentOnFace(0,f);
        }
      }
    }
    if (failed || 1==matches) {
      mode=MODE_GAMEOVER;
      FOREACH_FACE(f) {
        if (isValueReceivedOnFaceExpired(f)) {
          setColorOnFace(RED, f);
        } else {
          setValueSentOnFace(0,f);
        }
      }      
    }
  }
  if (MODE_GAMEOVER==mode || MODE_BOARD_READY==mode) {
    if (buttonSingleClicked()) {
      //discard
    }
    if (buttonDoubleClicked()) {
      setValueSentOnAllFaces(START_RESET);
    } else {
      bool allReset = true;
      bool anyReset = false;
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {
          byte val = getLastValueReceivedOnFace(f);
          if (START_RESET == val || END_RESET == val) {
            anyReset = true;
          } else {
            allReset = false;
          }
        }
      }
      if (allReset) {
        mode=MODE_SETUP;
        setValueSentOnAllFaces(END_RESET);        
      } else if (anyReset) {
        setValueSentOnAllFaces(START_RESET);      
      }
    }
  }

}

boolean checkFaceIsValue(byte face, byte value) {
  if(!isValueReceivedOnFaceExpired(face) && value==getLastValueReceivedOnFace(face)) {
    return true;
  }
  return false;
}

void drawColors() {
  FOREACH_FACE(f) {
    setColorOnFace(colorCycle[f],f);
  }
}

void drawValuesOnFaces(byte values[]) {
  FOREACH_FACE(f) {
    setColorOnFace(colorCycle[values[f]],f);
  }  
}

void shuffleArray(byte arr[]) {
  
  for (int i = 0; i < 5; i++)
  {
      int j = random(5);
  
      byte t = arr[i];
      arr[i] = arr[j];
      arr[j] = t;
  }
}

void setupBoard() {
  mode = MODE_BOARD_READY;
  shuffleArray(faceValues); //random for now
  drawValuesOnFaces(faceValues);
  FOREACH_FACE(f) {
    setValueSentOnFace(MATCH_SEED + faceValues[f],f);
  }
}
