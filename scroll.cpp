#include <SPI.h>
#include <GD.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "splitscreen.h"
#include "platformer.h"
#include "font8x8.h"

#define GET_FAR_ADDRESS(var) (var)

int atxy(int x, int y)
{
  return (y << 6) + x;
}

// copy a (w,h) rectangle from the source image (x,y) into picture RAM
static void rect(unsigned int dst, byte x, byte y, byte w, byte h)
{
  prog_uchar *src = platformer_pic + (16 * y) + x;
  while (h--) {
    GD.copy(dst, src, w);
    dst += 64;
    src += 16;
  }
}

#define SINGLE(x, y) (pgm_read_byte_near(&platformer_pic[(y) * 16 + (x)]))

// Draw a random 8-character wide background column at picture RAM dst

void draw_column(unsigned int dst)
{
  byte y;
  byte x;
  byte ch;

  // Clouds and sky, 11 lines
  rect(dst, 0, 0, 8, 11);

  // bottom plain sky, lines 11-28
  ch = SINGLE(0,11);
  for (y = 11; y < 28; y++)
    GD.fill(dst + (y << 6), ch, 8);
  
  // randomly choose between background elements
  byte what = random(256);
  if (what < 10) {
    // big mushroom thing
    y = random(11, 18);
    rect(dst + atxy(0, y), 8, 18, 8, 9);
    y += 9;
    byte i;
    while (y < 28) {
      rect(dst + atxy(0, y), 8, 23 + (i & 3), 8, 1);
      i++, y++;
    }
  } else if (what < 32) {
    // pair of green bollards
    for (x = 0; x < 8; x += 4) {
      y = random(20, 25);
      rect(dst + atxy(x, y), 6, 11, 4, 3);
      y += 3;
      while (y < 28) {
        rect(dst + atxy(x, y), 6, 13, 4, 1);
        y++;
      }
    }
  } else {
    // hills
    for (x = 0; x < 8; x += 2) {
      y = random(20, 25);
      rect(dst + atxy(x, y), 4, 11, 2, 3);
      y += 3;
      while (y < 28) {
        rect(dst + atxy(x, y), 4, 13, 2, 1);
        y++;
      }
    }
    // foreground blocks
    x = random(5);
    y = random(11, 24);
    byte blk = random(4);
    rect(dst + atxy(x, y), blk * 4, 14, 4, 3);
    y += 3;
    while (y < 28) {
      rect(dst + atxy(x, y), blk * 4, 17, 4, 1);
      y++;
    }
  }

  // Ground, line 28
  ch = SINGLE(0,18);
  GD.fill(dst + atxy(0,28), ch, 8);
  // Underground, line 29
  ch = SINGLE(0,19);
  GD.fill(dst + atxy(0,29), ch, 8);
}

unsigned long xscroll;
unsigned long i;

void putstr(int x, int y, const char *s)
{
  GD.__wstart((y << 6) + x);
  while (*s)
    SPI.transfer(125 + *s++);
  GD.__end();
}

void putchr(int x, int y, const char s)
{
  GD.__wstart((y << 6) + x);
  SPI.transfer(s);
  GD.__end();
}
static byte stretch[16] = {
  0x00, 0x03, 0x0c, 0x0f,
  0x30, 0x33, 0x3c, 0x3f,
  0xc0, 0xc3, 0xcc, 0xcf,
  0xf0, 0xf3, 0xfc, 0xff
};

void setup()
{
  GD.begin();
  GD.copy(RAM_CHR, platformer_chr, sizeof(platformer_chr));
  GD.copy(RAM_PAL, platformer_pal, sizeof(platformer_pal));

  for (i = 0; i < 96*8; i++) {
    byte b = pgm_read_byte_near(GET_FAR_ADDRESS(font8x8) + i);

    byte h = stretch[b >> 4];
    byte l = stretch[b & 0xf];
    GD.wr(0x1000 + 2512 + (2 * i), h);
    GD.wr(0x1000 + 2512 + (2 * i) + 1, l);
  }
  for (i = 157; i < 256; i++) {
    GD.setpal(4 * i + 0, RGB(255,255,255));
    GD.setpal(4 * i + 3, TRANSPARENT);
  }
  for (i = 0; i < 256; i++)
    GD.sprite(i, 400, 400, 0, 0, 0);

  for (i = 0; i < 64; i += 8) {
    draw_column(atxy(i, 0));
  }
  Serial.begin(1000000);

  /*
   For the splitscreen microprogram, the COMM area holds 8 short words
   that control the 3-way screen split:

   COMM+0   SCROLL_X for top section
   COMM+2   SCROLL_Y for top section
   COMM+4   Y-coordinate of start of middle section
   COMM+6   SCROLL_X for middle section
   COMM+8   SCROLL_Y for middle section
   COMM+10  Y-coordinate of start of bottom section
   COMM+12  SCROLL_X for bottom section
   COMM+14  SCROLL_Y for bottom section
  */
  GD.wr16(COMM+0, 0);
  GD.wr16(COMM+2, 264);
  GD.wr16(COMM+4, 20);   // split at line 100

  GD.wr16(COMM+6, 0);
  GD.wr16(COMM+8, (511 & -16));
  GD.wr16(COMM+10, 260);   // split at line 200

  GD.wr16(COMM+12, 0);
  GD.wr16(COMM+14, (511 & 496));

  GD.microcode(splitscreen_code, sizeof(splitscreen_code));
  
  putstr(0, 33, "Hello World!");
  //for(i=157;i<157+50;i++) {
  //  putchr(i-157, 33, (char) i);
  //}
  //for(i=207;i<256;i++) {
  //  putchr(i-207, 34, (char) i);
  //}
}

void loop()
{
  xscroll++;
  if ((xscroll & 63) == 0) {
    // figure out where to draw the 64-pixel draw_column
    // offscreen_pixel is the pixel x draw_column that is offscreen...
    int offscreen_pixel = ((xscroll + (7 * 64)) & 511);
    // offscreen_ch is the character address
    byte offscreen_ch = (offscreen_pixel >> 3);
    draw_column(atxy(offscreen_ch, 0));
  }
  //int x = analogRead(0);       // read analog input pin 0
  //int y = analogRead(1);       // read analog input pin 1
  //char tmp[6];
  //snprintf(tmp, 6, "%d", x);
  //putstr(((xscroll & 511) >> 3) + 10, 34, tmp);
  //Serial.println(x,DEC);
  //snprintf(tmp, 6, "%d", y);
  //putstr(((xscroll & 511) >> 3) + 10, 35, tmp);
  //Serial.println(y,DEC);
  if ((xscroll % 8) == 0) {
    putchr(5, 34, (char) ((i-5) & 255));
    putchr(4, 34, (char) ((i-4) & 255));
    putchr(3, 34, (char) ((i-3) & 255));
    putchr(2, 34, (char) ((i-2) & 255));
    putchr(1, 34, (char) ((i-1) & 255));
    putchr(0, 34, (char) (i++ & 255));
  }
  GD.waitvblank();
  //GD.wr16(SCROLL_X, xscroll);
  GD.wr16(COMM+6, xscroll);
}
