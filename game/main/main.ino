#include <SPI.h>
#include <SD.h>
#include <ESP8266HTTPUpdate.h>  // <--- ИСПРАВЛЕНО: правильный заголовочный файл для ESP8266#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

// ========== ПИНЫ ==========
#define SD_CS      D4      // CS SD-карты
#define I2C_SDA    D2
#define I2C_SCL    D1

// Кнопки на MCP23017
#define BTN_UP     6       // GPA6
#define BTN_DOWN   4       // GPA4
#define BTN_SHOOT  0       // GPA0 (A - выбор)
#define BTN_SHOP   1       // GPA1 (B - выход)

// ========== ЭКРАН ==========
#define SCREEN_W 160
#define SCREEN_H 128
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
Adafruit_MCP23X17 mcp;
String gameNames[30];
String gameFiles[30];
int gameCount = 0;
int selectedGame = 0;

// ========== АНТИДРЕБЕЗГ ==========
bool lastBtn[16] = {HIGH};
unsigned long lastDeb[16] = {0};

bool isPressed(uint8_t pin) {
  bool reading = mcp.digitalRead(pin);
  if (reading != lastBtn[pin]) lastDeb[pin] = millis();
  if (millis() - lastDeb[pin] > 50 && reading == LOW) {
    lastBtn[pin] = reading;
    return true;
  }
  lastBtn[pin] = reading;
  return false;
}

// ========== СКАНИРОВАНИЕ SD-КАРТЫ ==========
void scanSDCard() {
  gameCount = 0;
  File root = SD.open("/");
  if (!root) return;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String name = String(entry.name());
    if (name.endsWith(".bin")) {
      gameNames[gameCount] = name.substring(0, name.length() - 4);
      gameFiles[gameCount] = "/" + name;
      gameCount++;
    }
    entry.close();
  }
  root.close();

  // сортировка по имени
  for (int i = 0; i < gameCount - 1; i++) {
    for (int j = i + 1; j < gameCount; j++) {
      if (gameNames[i] > gameNames[j]) {
        String tmpName = gameNames[i];
        String tmpFile = gameFiles[i];
        gameNames[i] = gameNames[j];
        gameFiles[i] = gameFiles[j];
        gameNames[j] = tmpName;
        gameFiles[j] = tmpFile;
      }
    }
  }
}

// ========== ОТРИСОВКА МЕНЮ ==========
void drawMenu() {
  spr.fillSprite(TFT_BLACK);

  spr.setTextColor(TFT_GOLD, TFT_BLACK);
  spr.setTextSize(2);
  spr.drawString("GAME LOADER", 25, 5, 2);

  if (gameCount == 0) {
    spr.setTextColor(TFT_RED);
    spr.setTextSize(1);
    spr.drawString("No .bin files found!", 20, 60, 1);
    spr.drawString("Place games on SD", 25, 80, 1);
    spr.drawString("FAT32 format", 40, 100, 1);
    spr.pushSprite(0, 0);
    return;
  }

  int start = max(0, selectedGame - 3);
  int end = min(gameCount, start + 7);

  for (int i = start; i < end; i++) {
    int y = 35 + (i - start) * 13;
    if (i == selectedGame) {
      spr.setTextColor(TFT_GREEN);
      spr.drawString(">", 10, y, 1);
      spr.setTextColor(TFT_YELLOW);
    } else {
      spr.setTextColor(TFT_WHITE);
    }
    spr.drawString(gameNames[i], 25, y, 1);
  }

  spr.setTextColor(TFT_CYAN);
  spr.drawString("UP/DOWN: select", 10, 115, 1);
  spr.drawString("A: load game", 10, 125, 1);

  spr.pushSprite(0, 0);
}

// ========== ЗАГРУЗКА ИГРЫ С SD-КАРТЫ ==========
bool loadGameFromSD(String filename) {
  // Открываем файл
  File file = SD.open(filename.c_str(), FILE_READ);
  if (!file) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_RED);
    spr.drawString("ERROR: Can't open file", 15, 60, 1);
    spr.drawString(filename, 25, 75, 1);
    spr.pushSprite(0, 0);
    delay(2000);
    return false;
  }

  // Получаем размер файла
  size_t fileSize = file.size();

  // Показываем экран загрузки
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_GREEN);
  spr.drawString("Loading: " + filename, 10, 30, 1);
  spr.setTextColor(TFT_WHITE);
  spr.drawString("Size: " + String(fileSize) + " bytes", 20, 50, 1);
  spr.pushSprite(0, 0);

  // Начинаем обновление прошивки
  if (!Update.begin(fileSize, U_FLASH)) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_RED);
    spr.drawString("ERROR: Not enough space", 15, 60, 1);
    spr.pushSprite(0, 0);
    file.close();
    delay(2000);
    return false;
  }

  // Читаем файл и записываем во flash
  uint8_t buffer[1024];
  size_t totalWritten = 0;

  while (file.available()) {
    size_t bytesRead = file.read(buffer, 1024);
    if (bytesRead > 0) {
      size_t bytesWritten = Update.write(buffer, bytesRead);
      if (bytesWritten != bytesRead) {
        spr.fillSprite(TFT_BLACK);
        spr.setTextColor(TFT_RED);
        spr.drawString("ERROR: Write failed", 20, 60, 1);
        spr.pushSprite(0, 0);
        file.close();
        delay(2000);
        return false;
      }
      totalWritten += bytesWritten;

      // Прогресс-бар
      int progress = map(totalWritten, 0, fileSize, 0, 160);
      spr.fillRect(0, 90, progress, 8, TFT_GREEN);
      spr.pushSprite(0, 0);
    }
  }

  file.close();

  // Завершаем обновление
  if (!Update.end()) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_RED);
    spr.drawString("ERROR: Update failed", 20, 60, 1);
    spr.pushSprite(0, 0);
    delay(2000);
    return false;
  }

  // Проверяем успешность
  if (!Update.isFinished()) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_RED);
    spr.drawString("ERROR: Update not finished", 15, 60, 1);
    spr.pushSprite(0, 0);
    delay(2000);
    return false;
  }

  return true;
}

// ========== ИНИЦИАЛИЗАЦИЯ ==========
void setup() {
  Serial.begin(115200);

  // Дисплей
  tft.init();
  tft.setRotation(1);
  spr.createSprite(SCREEN_W, SCREEN_H);

  // MCP23017
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!mcp.begin_I2C(0x20)) {
    spr.fillSprite(TFT_BLACK);
    spr.drawString("MCP ERROR!", 45, 60, 2);
    spr.pushSprite(0, 0);
    while (1);
  }

  mcp.pinMode(BTN_UP, INPUT_PULLUP);
  mcp.pinMode(BTN_DOWN, INPUT_PULLUP);
  mcp.pinMode(BTN_SHOOT, INPUT_PULLUP);
  mcp.pinMode(BTN_SHOP, INPUT_PULLUP);

  // SD-карта
  SPI.begin();
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!SD.begin(SD_CS)) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_RED);
    spr.drawString("SD CARD ERROR!", 30, 50, 2);
    spr.drawString("Check wiring", 45, 70, 1);
    spr.drawString("and format FAT32", 35, 85, 1);
    spr.pushSprite(0, 0);
    while (1);
  }

  scanSDCard();
  drawMenu();
}

// ========== ОСНОВНОЙ ЦИКЛ ==========
void loop() {
  if (isPressed(BTN_UP)) {
    selectedGame--;
    if (selectedGame < 0) selectedGame = gameCount - 1;
    drawMenu();
    delay(150);
  }

  if (isPressed(BTN_DOWN)) {
    selectedGame++;
    if (selectedGame >= gameCount) selectedGame = 0;
    drawMenu();
    delay(150);
  }

  if (isPressed(BTN_SHOOT) && gameCount > 0) {
    if (loadGameFromSD(gameFiles[selectedGame])) {
      // Если загрузка успешна — перезагружаемся
      ESP.restart();
    } else {
      drawMenu();
    }
    delay(500);
  }

  delay(20);
}