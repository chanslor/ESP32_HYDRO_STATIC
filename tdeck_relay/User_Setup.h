// User_Setup.h for LILYGO T-Deck
// Copy this file to your Arduino libraries/TFT_eSPI folder
// OR place it in the sketch folder (some versions of TFT_eSPI support this)

#define USER_SETUP_INFO "T-Deck ST7789"

// ===== Display Driver =====
#define ST7789_DRIVER

// ===== Display Size =====
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ===== T-Deck Pin Definitions =====
#define TFT_CS    12
#define TFT_DC    11
#define TFT_RST   -1   // Connected to board reset
#define TFT_MOSI  41
#define TFT_SCLK  40
#define TFT_BL    42   // Backlight control

// ===== SPI Settings =====
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  16000000

// ===== Color Order =====
#define TFT_RGB_ORDER TFT_RGB

// ===== Fonts to Load =====
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT
