#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Sprite.h"

// Pines de la pantalla TFT
#define TFT_DC 7
#define TFT_CS 6
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 10
#define TFT_MISO 12
#define BUZZER_PIN 9

// Pines de los botones
#define BOTON_SALTAR 18
#define BOTON_BLOQUEAR 19

// Configuración de la pantalla TFT
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Dimensiones de la pantalla
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;

// Variables del caballero
int playerX = 40;  // Posición fija en X
int playerY = SCREEN_HEIGHT - 32 - 15; // Ajusta según el alto del sprite
int prevPlayerY = playerY; // Para limpiar la posición anterior
bool isJumping = false;
bool isBlocking = false;

//variable animacion
int currentFrame = 0;  // Frame actual de la animación
unsigned long lastFrameTime = 0; // Última vez que se cambió de frame
const int frameInterval = 70; // Intervalo entre frames (ms)
int desplazamientoSuelo = 0;

// Variables de salto
int jumpCounter = 0;
const int jumpHeight = 35;
const int jumpDuration = 30;

// Variables de obstáculos
int obstacleX = SCREEN_WIDTH;
int obstacleY = SCREEN_HEIGHT - 50;
int obstacleType = 0;
int obstacleSpeed = 12;

// Variables del juego
unsigned long gameTime = 0;
bool gameOver = false;
int puntuacion = 0;

//sonidos
const int FRECUENCIA_SALTO = 1000; 
const int DURACION_SALTO = 200;   
const int FRECUENCIA_BLOQUEO = 500; 
const int DURACION_BLOQUEO = 300;   

// Prototipos
void esperarParaIniciar();
void mostrarGameOver();
void moverObstaculos();
void dibujarPantalla();
void dibujarCaballeroAnimado();
void verificarColision();
void reiniciarJuego();
void actualizarSuelo();
void mostrarPuntuacion();
void reproducirSonido(int frecuencia, int duracion);

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0)); // Usamos una lectura de ruido para la semilla
  pinMode(BOTON_SALTAR, INPUT_PULLUP);
  pinMode(BOTON_BLOQUEAR, INPUT_PULLUP);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  esperarParaIniciar();
  reiniciarJuego();
}

void loop() {
  if (gameOver) {
    mostrarGameOver();
    esperarParaIniciar();
    reiniciarJuego();
    return;
  }

  if (digitalRead(BOTON_SALTAR) == LOW && jumpCounter == 0) {
    isJumping = true;
    jumpCounter = jumpDuration;
    reproducirSonido(FRECUENCIA_SALTO, DURACION_SALTO);
  }

  if (digitalRead(BOTON_BLOQUEAR) == LOW) {
    isBlocking = true;
    reproducirSonido(FRECUENCIA_BLOQUEO, DURACION_BLOQUEO);
  } else {
    isBlocking = false;
  }

  if (jumpCounter > 0) {
    jumpCounter--;
    prevPlayerY = playerY;

    if (jumpCounter > jumpDuration / 2) {
      playerY = SCREEN_HEIGHT - 50 - jumpHeight;
    } else {
      playerY = SCREEN_HEIGHT - 50;
    }

    if (jumpCounter == 0) {
      isJumping = false;
    }
  }
  actualizarSuelo();
  moverObstaculos();
  dibujarPantalla();
  verificarColision();
  mostrarPuntuacion(); 
  gameTime++;
  puntuacion++;
  delay(30);
}

void reiniciarJuego() {
  playerY = SCREEN_HEIGHT - 50;
  prevPlayerY = playerY;
  isJumping = false;
  isBlocking = false;
  jumpCounter = 0;

  obstacleX = SCREEN_WIDTH;
  obstacleType = 0;
  obstacleSpeed = 6;

  gameTime = 0;
  gameOver = false;
}

void moverObstaculos() {
  // Tamaño del obstáculo según el tipo
  int obstacleWidth = (obstacleType == 0) ? 40 : 32; // Tronco o flecha
  int obstacleHeight = (obstacleType == 0) ? 25 : 32;

  // Borra completamente el obstáculo anterior
  tft.fillRect(obstacleX, obstacleY, obstacleWidth, obstacleHeight, ILI9341_BLACK);

  // Mueve el obstáculo
  obstacleX -= obstacleSpeed;

  // Si el obstáculo sale de la pantalla, reinícialo
  if (obstacleX + obstacleWidth < 0) {
    obstacleX = SCREEN_WIDTH;
    obstacleType = random(0, 2); // 0 = Tronco, 1 = Flecha
  }
}

void mostrarPuntuacion() {
  tft.setCursor(10, 10); // Coordenadas para mostrar la puntuación
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Texto blanco con fondo negro
  tft.setTextSize(2);
  tft.print("Puntos: ");
  tft.print(puntuacion);
}

void actualizarSuelo() {
  desplazamientoSuelo -= 2; // Velocidad de movimiento
  if (desplazamientoSuelo < -10) {
    desplazamientoSuelo = 0; // Reiniciar desplazamiento
  }

  for (int x = desplazamientoSuelo; x < 240; x += 10) {
    tft.fillRect(x, 305, 10, 10, ILI9341_WHITE);
  }
}


void dibujarPantalla() {
  // Borra la posición anterior del jugador
  tft.fillRect(10, playerY == SCREEN_HEIGHT - 50 ? SCREEN_HEIGHT - 50 - jumpHeight : SCREEN_HEIGHT - 50, 38, 32, ILI9341_BLACK);

  // Dibujar jugador actual
  dibujarCaballeroAnimado();

  // Borra completamente el área del obstáculo anterior
  tft.fillRect(obstacleX + obstacleSpeed, obstacleY, 40, 32, ILI9341_BLACK);

  // Dibujar obstáculo actual
  if (obstacleType == 0) {
    tft.drawBitmap(obstacleX, obstacleY, TroncoSprite, 40, 25, ILI9341_GREEN); // Tronco
  } else if (obstacleType == 1) {
    tft.drawBitmap(obstacleX, obstacleY, flechaSprite, 32, 32, ILI9341_RED); // Flecha
  }
}

void dibujarCaballeroAnimado() {
  const int spriteWidth = 38;  // Ancho del caballero
  const int spriteHeight = 32; // Altura del caballero

  // Limpia completamente el área previa del caballero
  tft.fillRect(playerX, prevPlayerY, spriteWidth, spriteHeight, ILI9341_BLACK);

  // Actualiza el frame actual basado en el tiempo
  if (millis() - lastFrameTime > frameInterval) {
    currentFrame = (currentFrame + 1) % 6; // Avanza al siguiente frame
    lastFrameTime = millis();
  }

  // Dibuja el nuevo frame del caballero
  tft.drawBitmap(playerX, playerY, ShoveFrames[currentFrame], spriteWidth, spriteHeight, ILI9341_WHITE);

  // Si está bloqueando, sobrescribe con el sprite de bloqueo
  if (isBlocking) {
    tft.drawBitmap(playerX, playerY, ShoveFrames[currentFrame], spriteWidth, spriteHeight, ILI9341_CYAN);
  }

  // Actualiza la posición previa del caballero
  prevPlayerY = playerY;
}


void verificarColision() {
  // Tamaño del obstáculo según el tipo
  int obstacleWidth = (obstacleType == 0) ? 40 : 32;

  // Colisión con tronco
  if (obstacleType == 0 && !isJumping &&
      obstacleX < playerX + 38 && obstacleX + obstacleWidth > playerX) {
    gameOver = true;
  }

  // Colisión con flecha
  if (obstacleType == 1 && !isBlocking && // Solo si no está bloqueando
      obstacleX < playerX + 38 && obstacleX + obstacleWidth > playerX &&
      playerY == SCREEN_HEIGHT - 50) { // Asegúrate de que el jugador esté en tierra
    gameOver = true;
  }

  // Si está bloqueando y hay una flecha en rango, eliminar la flecha
  if (obstacleType == 1 && isBlocking &&
      obstacleX < playerX + 38 && obstacleX + obstacleWidth > playerX) {
    // Borra completamente la flecha actual
    tft.fillRect(obstacleX, obstacleY, 32, 32, ILI9341_BLACK);

    // Resetea la flecha
    obstacleX = SCREEN_WIDTH;
    obstacleType = random(0, 2); // Generar nuevo obstáculo aleatorio
  }
}


void mostrarGameOver() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.setCursor(30, 150);
  tft.print("GAME OVER");
  delay(3000);
}

void esperarParaIniciar() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 150);
  tft.print("Pulsa para iniciar");

  while (digitalRead(BOTON_SALTAR) == HIGH) {
    delay(10);
  }

  tft.fillScreen(ILI9341_BLACK);
}

void reproducirSonido(int frecuencia, int duracion) {
  tone(BUZZER_PIN, frecuencia, duracion); 
  delay(duracion);
  noTone(BUZZER_PIN); // Detiene el sonido
}
