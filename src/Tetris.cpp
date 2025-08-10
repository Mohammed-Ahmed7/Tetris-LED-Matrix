#if 0

#include <Arduino.h>


// Hier werden die Pins definiert, an denen die LED-Matrix angeschlossen ist.
// Außerdem gibt es noch einen Joystick mit VRX (x-Richtung), VRY (y-Richtung)
// und einen Taster SW.

#define LEDARRAY_D   PB0
#define LEDARRAY_C   PB1
#define LEDARRAY_B   PB2
#define LEDARRAY_A   PB3
#define LEDARRAY_G   PB4
#define LEDARRAY_DI  PB5
#define LEDARRAY_CLK PB6
#define LEDARRAY_LAT PB7

// Joystick und Taster
#define VRX PA4
#define VRY PA6
#define SW  PA7

// Dies ist eine Zustandsmaschine, die entweder das Spiel ausführt
// oder den Score anzeigt, wenn das Spiel vorbei ist.
enum GameState {
  STATE_PLAYING,
  STATE_SHOWSCORE
};

// Wir starten mit dem SPIEL-Zustand
static GameState gameState = STATE_PLAYING;

// Das Spielfeld hat 16 Zeilen und 16 Spalten
// 0 bedeutet aus, 1 bedeutet an
static byte board[16][16];

// Dieser Wert speichert die Punkte
// Er kann maximal 999 werden
static int score = 0;


// Die Variablen currentX und currentY sagen wo sich der aktuelle Stein gerade befindet
// currentRotation ist eine Zahl von 0 bis 3 und steht für die Rotationsrichtung des Steins
static int currentX = 0;
static int currentY = 0;
static int currentRotation = 0; 

// Das sind unsere sieben Tetris-Steine als Typen in einem enum
enum TetrominoType {
  TETROMINO_I,
  TETROMINO_O,
  TETROMINO_T,
  TETROMINO_S,
  TETROMINO_Z,
  TETROMINO_J,
  TETROMINO_L,
  TETROMINO_COUNT
};

// Wir merken uns, welcher Stein gerade im Spiel ist
static TetrominoType currentPiece = TETROMINO_I;


// Im Tetris gibt es 7 verschiedene Steine. Jeder Stein hat 4 Rotationszustände (0°, 90°, 180°, 270°)
// Wir beschreiben jeden Stein in einem 4x4-Array mit 1 (Block vorhanden) oder 0 (kein Block)
// Zum Beispiel beim I-Stein sind die vier möglichen Ausrichtungen eingetragen

// I (Gerade)
static const byte SHAPE_I[4][4][4] = {
  {{1,0,0,0},{1,0,0,0},{1,0,0,0},{1,0,0,0},},
  {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0},},
  { {1,0,0,0}, {1,0,0,0},{1,0,0,0},{1,0,0,0},},
  {  {0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0},},};
static const byte SHAPE_O[4][4][4] = {
  {{1,1,0,0}, {1,1,0,0},{0,0,0,0},{0,0,0,0},},
  {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0},},
  { {1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}, },
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0},{0,0,0,0},},};
// T
static const byte SHAPE_T[4][4][4] = {
  { {1,1,1,0}, {0,1,0,0},{0,0,0,0}, {0,0,0,0},},
  {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0},},
  { {0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0},},
  {{1,0,0,0},{1,1,0,0}, {1,0,0,0},{0,0,0,0},},};
// S
static const byte SHAPE_S[4][4][4] = {
  {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0},},
  {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0},},
  {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0},},
  { {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0},},
};
//Z
static const byte SHAPE_Z[4][4][4] = {
  { {1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0},},
  {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0},},
  {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0},},
  {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0},},};
// J
static const byte SHAPE_J[4][4][4] = {
  { {1,0,0,0},{1,0,0,0},{1,1,0,0},{0,0,0,0},},
  { {1,1,1,0},{1,0,0,0},{0,0,0,0},{0,0,0,0},},
  {  {1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0},},
  { {0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0},},
};
// L
static const byte SHAPE_L[4][4][4] = {
  { {1,0,0,0},{1,0,0,0},{1,1,0,0},{0,0,0,0},},
  { {1,1,1,0},{1,0,0,0},{0,0,0,0},{0,0,0,0},},
  { {1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0},},
  { {0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0},},};

// In dieses Array TETROMINOS kommen alle Steine rein, damit wir leicht
// darauf zugreifen können
static const byte (*TETROMINOS[TETROMINO_COUNT])[4][4] = {
  SHAPE_I, SHAPE_O, SHAPE_T, SHAPE_S, SHAPE_Z, SHAPE_J, SHAPE_L
};


// Diese Funktion baut links und rechts Wände, damit die Steine
// nicht so einfach aus dem Spielfeld raus können
void buildWalls2()
{
  for (int y = 0; y < 16; y++)
  {
    board[y][0]  = 1;  // Linke Wand
    board[y][1]  = 1;  // Noch eine Spalte links
    board[y][14] = 1;  // Rechte Wand
    board[y][15] = 1;  // Noch eine Spalte rechts
  }
}


// Hier haben wir Funktionen, um die Daten an unsere LED-Matrix zu senden
// Wir machen das über ein Schieberegister, das pro Zeile die passenden Bits
// bekommt und so die LEDs an oder aus schaltet

static unsigned char Display_Buffer[2];

// Send schickt ein Byte seriell raus
void Send(unsigned char dat)
{
  // Erst Takt und Latch auf LOW
  digitalWrite(LEDARRAY_CLK, LOW);
  delayMicroseconds(1);
  digitalWrite(LEDARRAY_LAT, LOW);
  delayMicroseconds(1);

  // Dann jedes Bit einzeln rausschieben
  for (unsigned char i = 0; i < 8; i++)
  {
    if (dat & 0x01) {
      digitalWrite(LEDARRAY_DI, LOW);
    } else {
      digitalWrite(LEDARRAY_DI, HIGH);
    }
    delayMicroseconds(1);
    digitalWrite(LEDARRAY_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(LEDARRAY_CLK, LOW);
    delayMicroseconds(1);

    dat >>= 1;
  }
}

// Scan_Line schaut, welche Zeile wir gerade ansteuern wollen
void Scan_Line(unsigned char m)
{
  switch (m)
  {
    case 0:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, LOW); break;
    case 1:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, HIGH); break;
    case 2:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, LOW); break;
    case 3:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, HIGH); break;
    case 4:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, LOW); break;
    case 5:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, HIGH); break;
    case 6:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, LOW); break;
    case 7:  digitalWrite(LEDARRAY_D, LOW); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, HIGH); break;
    case 8:  digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, LOW); break;
    case 9:  digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, HIGH); break;
    case 10: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, LOW); break;
    case 11: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, LOW); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, HIGH); break;
    case 12: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, LOW); break;
    case 13: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, LOW); digitalWrite(LEDARRAY_A, HIGH); break;
    case 14: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, LOW); break;
    case 15: digitalWrite(LEDARRAY_D, HIGH); digitalWrite(LEDARRAY_C, HIGH); digitalWrite(LEDARRAY_B, HIGH); digitalWrite(LEDARRAY_A, HIGH); break;
    default: break;
  }
}

// Display zeigt das Bild zeilenweise an. Es wird jede Zeile kurz angezeigt
// so entsteht durch schnelles Umschalten ein komplettes Bild
void Display(const unsigned char dat[][32])
{
  for (unsigned char i = 0; i < 16; i++) {
    // LEDARRAY_G auf HIGH machen, um Zeile kurz auszuschalten
    digitalWrite(LEDARRAY_G, HIGH);

    unsigned char mirroredLow = 0;
    unsigned char mirroredHigh = 0;
    for (unsigned char bit = 0; bit < 8; bit++) {
      if (dat[0][i] & (1 << bit)) {
        mirroredLow |= (1 << (7 - bit));
      }
      if (dat[0][i + 16] & (1 << bit)) {
        mirroredHigh |= (1 << (7 - bit));
      }
    }
    Display_Buffer[0] = mirroredLow;
    Display_Buffer[1] = mirroredHigh;

    // Wir schicken zuerst das High-Byte, dann das Low-Byte
    Send(Display_Buffer[1]);
    Send(Display_Buffer[0]);

    // Jetzt den Latch-Pin kurz auf High, damit das Schieberegister aktualisiert wird
    digitalWrite(LEDARRAY_LAT, HIGH);
    delayMicroseconds(1);
    digitalWrite(LEDARRAY_LAT, LOW);
    delayMicroseconds(1);

    // Dann wählen wir die Zeile aus
    Scan_Line(i);

    // LEDARRAY_G auf LOW schaltet die Zeile wieder ein
    digitalWrite(LEDARRAY_G, LOW); 
    delayMicroseconds(1000);
  }
}


// Hier kommen Funktionen, die für das Tetris-Spiel wichtig sind

const byte (*getShape(TetrominoType type, int rotation))[4] {
  // Diese Funktion gibt uns den 4x4-Block des jeweiligen Steins in der gewünschten Rotation
  return TETROMINOS[type][rotation];
}

// Hier wird geprüft, ob wir einen Stein an einer bestimmten Position (x,y)
// mit einer bestimmten Rotation überhaupt platzieren können
bool canPlacePiece(int x, int y, TetrominoType type, int rotation)
{
  const byte (*shape)[4] = getShape(type, rotation);
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (shape[row][col] == 1) {
        int bx = x + col;
        int by = y + row;
        // Wenn bx oder by außerhalb des Felds liegen geht das nicht
        if (bx < 0 || bx >= 16 || by < 0 || by >= 16) {
          return false;
        }
        // Wenn dort schon ein Stein im Weg ist, geht das auch nicht
        if (board[by][bx] != 0) {
          return false;
        }
      }
    }
  }
  return true;
}

// Hier platzieren wir den Stein wirklich auf dem Board
// und geben dem Spieler +1 Punkt, solange score noch unter 999 ist
void placePiece(int x, int y, TetrominoType type, int rotation)
{
  const byte (*shape)[4] = getShape(type, rotation);
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      if (shape[row][col] == 1) {
        int bx = x + col;
        int by = y + row;
        board[by][bx] = 1;
      }
    }
  }
  if (score < 999) {
    score++;
  }
}

// Diese Funktion prüft, ob in einer Reihe alle 16 Felder gefüllt sind
// Wenn ja wird die komplette Reihe gelöscht und darüber liegende Reihen
// rutschen eine Zeile runter
void clearFullLines()
{
  for (int row = 0; row < 16; row++)
  {
    bool full = true;
    for (int col = 0; col < 16; col++)
    {
      if (board[row][col] == 0) {
        full = false;
        break;
      }
    }
    if (full)
    {
      // Ziehe jede Zeile über row eine runter
      for (int r = row; r > 0; r--)
      {
        for (int c = 0; c < 16; c++)
        {
          board[r][c] = board[r - 1][c];
        }
      }
      // Die oberste Zeile machen wir leer
      for (int c = 0; c < 16; c++)
      {
        board[0][c] = 0;
      }
      // row-- damit wir dieselbe Zeile nochmal prüfen können falls mehrere voll sind
      row--;
    }
  }
  // Danach bauen wir die Wände wieder neu
  buildWalls2();
}

// Diese Funktion wählt zufällig einen Stein aus
TetrominoType getRandomPiece()
{
  return (TetrominoType) random(0, TETROMINO_COUNT);
}

// Hier wandeln wir unser 16x16-Feld in 2 x 16 Bytes um
// damit wir sie mit der Display()-Funktion ausgeben können
void convertBoardToDisplayData(unsigned char outData[1][32])
{
  memset(outData[0], 0, 32);
  for (int y = 0; y < 16; y++) {
    unsigned char lowB = 0;
    unsigned char highB = 0;

    for (int x = 0; x < 8; x++) {
      if (board[y][x]) {
        lowB |= (1 << x);
      }
    }

    for (int x = 8; x < 16; x++) {
      if (board[y][x]) {
        highB |= (1 << (x - 8));
      }
    }

    outData[0][y]      = lowB;
    outData[0][y + 16] = highB;
  }
}
// In diesen Arrays stehen kleine 3x5-Bitmap-Muster
// die Buchstaben und Ziffern darstellen (s, c, :, und 0-9)
static const byte CHAR3_s[5] = {0b111,0b100,0b111, 0b001,0b111};
static const byte CHAR3_c[5] = { 0b111,0b100, 0b100, 0b100,0b111};
static const byte CHAR3_colon[5] = { 0b000,0b010,0b000, 0b010,0b000};
// Ziffern 0..9
static const byte CHAR3_0[5] = {0b111,0b101,0b101,0b101,0b111};
static const byte CHAR3_1[5] = {0b010,0b110,0b010,0b010,0b111};
static const byte CHAR3_2[5] = {0b111,0b001,0b111,0b100, 0b111};
static const byte CHAR3_3[5] = {0b111,0b001,0b111,0b001,0b111};
static const byte CHAR3_4[5] = {0b101,0b101,0b111,0b001,0b001};
static const byte CHAR3_5[5] = {0b111, 0b100, 0b111,0b001, 0b111};
static const byte CHAR3_6[5] = { 0b111,0b100,0b111,0b101,0b111};
static const byte CHAR3_7[5] = { 0b111, 0b001,0b010, 0b010, 0b010};
static const byte CHAR3_8[5] = {0b111, 0b101,0b111, 0b101, 0b111};
static const byte CHAR3_9[5] = {0b111,0b101,0b111,0b001,0b111};

// Diese Funktion zeichnet ein Zeichen in 3x5 Pixeln auf das board
// startX, startY sind die Koordinaten der linken oberen Ecke
void drawChar3x5(int startX, int startY, const byte pattern[5])
{
  for (int row = 0; row < 5; row++) {
    if (startY+row >= 16) break;   // Nicht aus dem Spielfeld raus
    byte bits = pattern[row];
    for (int col = 0; col < 3; col++) {
      if (startX+col >= 16) break;
      if (bits & (1 << (2 - col))) {
        board[startY+row][startX+col] = 1;
      }
    }
  }
}

// Diese Funktion gibt je nach Ziffer (0..9) das passende Muster zurück
static const byte* getDigitPat(int d)
{
  switch(d){
    case 0: return CHAR3_0;
    case 1: return CHAR3_1;
    case 2: return CHAR3_2;
    case 3: return CHAR3_3;
    case 4: return CHAR3_4;
    case 5: return CHAR3_5;
    case 6: return CHAR3_6;
    case 7: return CHAR3_7;
    case 8: return CHAR3_8;
    case 9: return CHAR3_9;
    default:return CHAR3_0;
  }
}

// Hier malen wir oben "sc:" und unten in Zeile 8..12 die drei Ziffern des Scores
void drawScoreTwoLines(int scVal)
{
  // Erst "sc:" ganz oben
  drawChar3x5(0, 0, CHAR3_s);     
  drawChar3x5(4, 0, CHAR3_c);     
  drawChar3x5(8, 0, CHAR3_colon);

  // Score zerlegen in Hunderter, Zehner und Einer
  int hund = (scVal/100) % 10;
  int zehn = (scVal/10) % 10;
  int eins = scVal % 10;

  // Dann unten ab Zeile 8 die Ziffern zeichnen
  drawChar3x5( 0, 8, getDigitPat(hund));
  drawChar3x5( 4, 8, getDigitPat(zehn));
  drawChar3x5( 8, 8, getDigitPat(eins));
}


// Hier starten wir einen neuen Stein setzen ihn oben
// und schauen ob er überhaupt noch Platz hat
void spawnPiece()
{
  currentPiece = getRandomPiece();
  currentRotation = 0;
  currentX = 6;
  currentY = 0;

  // Wenn der Stein nicht mehr reinpasst, wechseln wir in den Score-Bildschirm
  if (!canPlacePiece(currentX, currentY, currentPiece, currentRotation))
  {
    gameState = STATE_SHOWSCORE;
  }
}

void setup()
{
  Serial.begin(9600);

  // Die Pins für unsere LED-Steuerung setzen wir auf OUTPUT
  pinMode(LEDARRAY_D, OUTPUT);
  pinMode(LEDARRAY_C, OUTPUT);
  pinMode(LEDARRAY_B, OUTPUT);
  pinMode(LEDARRAY_A, OUTPUT);
  pinMode(LEDARRAY_G, OUTPUT);
  pinMode(LEDARRAY_DI, OUTPUT);
  pinMode(LEDARRAY_CLK, OUTPUT);
  pinMode(LEDARRAY_LAT, OUTPUT);

  // Pins für Joystick und Taster
  pinMode(VRX, INPUT);
  pinMode(VRY, INPUT);
  pinMode(SW,  INPUT_PULLUP);

  // board mit 0 füllen (alle LEDs aus)
  memset(board,0,sizeof(board));
  // Wände an den Rändern bauen
  buildWalls2();
  
  // Score auf 0 setzen
  score = 0;
  // Zufallszahl initialisieren, indem wir einmal analogRead(A0) lesen
  // (der Pin sollte im Rauschen liegen)
  randomSeed(analogRead(A0));

  // Ersten Stein erzeugen
  spawnPiece();
}

void loop()
{
  static unsigned long lastFall = 0;
  // So schnell fällt der Stein (Millisekunden)
  static const unsigned long fallInterval = 450;

  // Hier lesen wir den Joystick
  int xValue = analogRead(VRX);
  int yValue = analogRead(VRY);
  Serial.println(xValue);
  Serial.println(yValue);

  // Jetzt schauen wir in welchem Zustand wir sind: Spielen oder Score zeigen
  switch(gameState)
  {
    case STATE_PLAYING:
    {
      // Dieses Array speichert die Daten die wir ans Display schicken
      static unsigned char tetrisData[1][32];
      // count ist eine kleine Zahl die wir nutzen um nach jeder Joystick-Bewegung
      // ein bisschen zu warten
      int count = 15;

      // Prüfen, ob genug Zeit vergangen ist damit der Stein eine Zeile fällt
      if (millis() - lastFall >= fallInterval) {
        lastFall = millis();
        if (canPlacePiece(currentX, currentY+1, currentPiece, currentRotation)) {
          currentY++;
        } else {
          // Wenn er nicht mehr tiefer kann legen wir ihn ab und erzeugen einen neuen
          placePiece(currentX, currentY, currentPiece, currentRotation);
          clearFullLines();
          spawnPiece();
          if (gameState == STATE_SHOWSCORE) {
            // Wenn das Spiel vorbei ist gehen wir raus aus case STATE_PLAYING
            break;
          }
        }
      }

      // Joystick nach links -> xValue > 900 
      if (xValue > 900) {
        if (canPlacePiece(currentX - 1, currentY, currentPiece, currentRotation)) {
          currentX--;
        }
        // Kurz Display aktualisieren
        for(int i = 0; i < count; i++){
          Display(tetrisData);
          delay(1);
        }
      }
      // Joystick nach rechts -> xValue < 300
      if (xValue < 300) {
        if (canPlacePiece(currentX + 1, currentY, currentPiece, currentRotation)) {
          currentX++;
        }
        // Kurz Display aktualisieren
        for(int i = 0; i < count; i++){
          Display(tetrisData);
          delay(1);
        }
      }
      // Joystick nach oben -> drehen
      if (yValue > 900) {
        int newRot = (currentRotation + 1) % 4;
        if (canPlacePiece(currentX, currentY, currentPiece, newRot)) {
          currentRotation = newRot;
        }
        // Kurz Display aktualisieren
        for(int i = 0; i < count; i++){
          Display(tetrisData);
          delay(1);
        }
      }

      // Erstmal alles auf 0 setzen
      memset(tetrisData[0], 0, 32);
      // board in tetrisData kopieren
      convertBoardToDisplayData(tetrisData);

      // Jetzt den aktuellen Stein in tetrisData reinmalen
      {
        const byte (*shape)[4] = getShape(currentPiece, currentRotation);
        for(int row = 0; row < 4; row++){
          for(int col = 0; col < 4; col++){
            if(shape[row][col] == 1){
              int bx = currentX + col;
              int by = currentY + row;
              if (bx >= 0 && bx < 16 && by >= 0 && by < 16) {
                if(bx < 8){
                  tetrisData[0][by] |= (1 << (bx % 8));
                } else {
                  tetrisData[0][by + 16] |= (1 << ((bx - 8)));
                }
              }
            }
          }
        }
      }

      // Jetzt zeigen wir die Daten an
      Display(tetrisData);
      break;
    }

    case STATE_SHOWSCORE:
    {
      // Das Spiel ist zu Ende,also leeren wir das Feld
      memset(board, 0, sizeof(board));

      // Wir malen "sc:" oben und darunter den Score 
      drawScoreTwoLines(score);

      // Diese Daten werden angezeigt
      static unsigned char finalData[1][32];
      memset(finalData[0], 0, 32);
      convertBoardToDisplayData(finalData);


      // Wir zeigen jetzt für immer den Score an. 
      while(true){
        Display(finalData);
        delay(1);
      }
    }
  }
}
#endif 
