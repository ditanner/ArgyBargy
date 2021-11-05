#define MODE_SETUP 1
#define MODE_BOARD 2
#define MODE_PLAYER_SETUP 3
#define MODE_PLAYER 4

#define MATCH_SEED 32
byte mode = MODE_SETUP;
byte neighbours = -1;

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
  setColor(GREEN);
  setValueSentOnAllFaces(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (MODE_SETUP==mode) {
  setColor(GREEN);
    bool neighboursReady = true;
    bool singlePair = false;
    bool setOfFour = false;
    bool validFormation = true;
    neighbours = 0;
    byte maxNeighbours = 0;
    FOREACH_FACE(f) {//check every face
      if (!isValueReceivedOnFaceExpired(f)) {
        neighbours++;
        byte val = getLastValueReceivedOnFace(f);
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
            byte nextFace = (f+1)%6;
            byte nextNextFace = (f+2)%6;
            byte prevFace = (f+5)%6;
            byte prevPrevFace = (f+4)%6;
            if(
                (checkFaceIsValue(nextFace,3) && checkFaceIsValue(nextNextFace,2)) 
                || (checkFaceIsValue(prevFace,3) && checkFaceIsValue(prevPrevFace,2)) 
              ) {
              setOfFour = true;
              maxNeighbours = 3;
            } else {
              validFormation = false;
            }
          }
          if (3==val && !setOfFour) {//could be middle of 2-3-2, or a 3-3
            byte nextFace = (f+1)%6;
            byte prevFace = (f+5)%6;
            if(checkFaceIsValue(nextFace,2) && checkFaceIsValue(prevFace,2)) {
              setOfFour = true;
              maxNeighbours = 3;
            } else if(checkFaceIsValue(nextFace,3) || checkFaceIsValue(prevFace,3)) {
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
    setValueSentOnAllFaces(neighbours);
    if (validFormation && neighboursReady) {
      drawColors();
      if(0==neighbours) {
        mode=MODE_PLAYER_SETUP;
      } else {
        shuffleArray(faceValues); //random for now
        drawValuesOnFaces(faceValues);
        FOREACH_FACE(f) {
          setValueSentOnFace(MATCH_SEED + faceValues[f],f);
        }
        mode=MODE_BOARD;
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
        if (getLastValueReceivedOnFace(f) == MATCH_SEED + faceValues[f]) {
          matches++;
        } else {
          failed = true;
        }
      }
    }
    if (2==matches && !failed) {
      FOREACH_FACE(f) {
        if (isValueReceivedOnFaceExpired(f)) {
          setColorOnFace(GREEN, f);
        }
      }
    }
    if (failed || 1==matches) {
      FOREACH_FACE(f) {
        if (isValueReceivedOnFaceExpired(f)) {
          setColorOnFace(RED, f);
        }
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
