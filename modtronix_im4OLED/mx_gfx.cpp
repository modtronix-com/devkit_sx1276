/***********************************
This is a our graphics core library, for all our displays. 
We'll be adapting all the
existing libaries to use this core to make updating, support 
and upgrading easier!

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

/*
 *  Modified by Neal Horman 7/14/2012 for use in mbed
 */

#include "mbed.h"

#include "mx_gfx.h"
#include "mx_gfx_font.h"

#if (GFX_ENABLE_ABSTRACTS==1)
// draw a circle outline
void MxGfx::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    drawPixel(x0, y0+r, color);
    drawPixel(x0, y0-r, color);
    drawPixel(x0+r, y0, color);
    drawPixel(x0-r, y0, color);
    
    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void MxGfx::drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    
    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if (cornername & 0x4)
        {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        } 

        if (cornername & 0x2)
        {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }

        if (cornername & 0x8)
        {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        
        if (cornername & 0x1)
        {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

void MxGfx::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    drawFastVLine(x0, y0-r, 2*r+1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

// used to do circles and roundrects!
void MxGfx::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    
    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        if (cornername & 0x1)
        {
            drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
            drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
        }

        if (cornername & 0x2)
        {
            drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
            drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
        }
    }
}
#endif

#if defined(GFX_ENABLE_ABSTRACTS) || (GFX_SIZEABLE_TEXT==1)
// bresenham's algorithm - thx wikpedia
void MxGfx::drawLine(int16_t x0, int16_t y0,  int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep)
    {
        swap(x0, y0);
        swap(x1, y1);
    }
    
    if (x0 > x1)
    {
        swap(x0, x1);
        swap(y0, y1);
    }
    
    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    
    int16_t err = dx / 2;
    int16_t ystep;
    
    if (y0 < y1)
        ystep = 1;
    else
        ystep = -1;
    
    for (; x0<=x1; x0++)
    {
        if (steep)
            drawPixel(y0, x0, color);
        else
            drawPixel(x0, y0, color);

        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

void MxGfx::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    // stupidest version - update in subclasses if desired!
    drawLine(x, y, x, y+h-1, color);
}

/** Draw and fill a rectangle. The rectangle starts at given point, and extends DOWN(h) and RIGHTH(w)
 * @note GFX_ENABLE_ABSTRACTS or GFX_SIZEABLE_TEXT must be defined in im4oled_config.h
 *
 * @param x X coordinate, a value from 0 - (width-1)
 * @param y Y coordinate, a value from 0 - (height-1)
 * @param w Width from given x,y coordinate. Extends to the Right of given x,y
 * @param h Height from given x,y coordinate. Extends Down of given x,y
 * @param color For monochrome display, 0=off, 1=on
 */
void MxGfx::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    // stupidest version - update in subclasses if desired!
    for (int16_t i=x; i<x+w; i++)
        drawFastVLine(i, y, h, color); 
}
#endif

#if (GFX_ENABLE_ABSTRACTS==1)
// draw a rectangle
void MxGfx::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y+h-1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x+w-1, y, h, color);
}

void MxGfx::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    // stupidest version - update in subclasses if desired!
    drawLine(x, y, x+w-1, y, color);
}

void MxGfx::fillScreen(uint16_t color)
{
    fillRect(0, 0, _width, _height, color);
}

// draw a rounded rectangle!
void MxGfx::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    // smarter version
    drawFastHLine(x+r  , y    , w-2*r, color); // Top
    drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    drawFastVLine(  x    , y+r  , h-2*r, color); // Left
    drawFastVLine(  x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    drawCircleHelper(x+r    , y+r    , r, 1, color);
    drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// fill a rounded rectangle!
void MxGfx::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    // smarter version
    fillRect(x+r, y, w-2*r, h, color);
    
    // draw four corners
    fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// draw a triangle!
void MxGfx::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

// fill a triangle!
void MxGfx::fillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    int16_t a, b, y, last;
    
    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1)
        swap(y0, y1); swap(x0, x1);

    if (y1 > y2)
        swap(y2, y1); swap(x2, x1);

    if (y0 > y1)
        swap(y0, y1); swap(x0, x1);

    
    if(y0 == y2)
    { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)
            a = x1;
        else if(x1 > b)
            b = x1;
            
        if(x2 < a)
            a = x2;
        else if(x2 > b) b = x2;
            drawFastHLine(a, y0, b-a+1, color);
        return;
    }

    int16_t
        dx01 = x1 - x0,
        dy01 = y1 - y0,
        dx02 = x2 - x0,
        dy02 = y2 - y0,
        dx12 = x2 - x1,
        dy12 = y2 - y1,
        sa   = 0,
        sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2)
        last = y1;   // Include y1 scanline
    else
        last = y1-1; // Skip it

    for(y=y0; y<=last; y++)
    {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b)
            swap(a,b);
        drawFastHLine(a, y, b-a+1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<=y2; y++)
    {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b)
            swap(a,b);
        drawFastHLine(a, y, b-a+1, color);
    }
}

void MxGfx::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
    uint8_t mask;

    for (int16_t j=0; j<h; j++)
    {
        for (int16_t i=0; i<w; i++ )
        {
            mask = 0x01 << (j%8);
            if (bitmap[i + (j/8)*w] & mask)
                drawPixel(x+i, y+j, color);
        }
    }
}
#endif

size_t MxGfx::writeChar(uint8_t c)
{
    if (c == '\n')
    {
        cursor_y += textsize*8;
        cursor_x = 0;
    }
    else if (c == '\r')
        cursor_x = 0;
    else
    {
        drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
        cursor_x += textsize*6;
        if (wrap && (cursor_x > (_width - textsize*6)))
        {
            cursor_y += textsize*8;
            cursor_x = 0;
        }
    }
    return 1;
}

// draw a character
void MxGfx::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if(
        (x >= _width) || // Clip right
        (y >= _height) || // Clip bottom
        ((x + 5 * size - 1) < 0) || // Clip left
        ((y + 8 * size - 1) < 0)    // Clip top
        )
    return;
    
    for (int8_t i=0; i<6; i++ )
    {
        uint8_t line = 0;

        if (i == 5) 
            line = 0x0;
        else 
            line = font5x7[(c*5)+i];
            
        for (int8_t j = 0; j<8; j++)
        {
            if (line & 0x1)
            {
#if (GFX_ENABLE_ABSTRACTS==1) || (GFX_SIZEABLE_TEXT==1)
                if (size == 1) // default size
                    drawPixel(x+i, y+j, color);
                else // big size
                    fillRect(x+(i*size), y+(j*size), size, size, color);
#else
                drawPixel(x+i, y+j, color);
#endif
            }
            else if (bg != color)
            {
#if (GFX_ENABLE_ABSTRACTS==1) || (GFX_SIZEABLE_TEXT==1)
                if (size == 1) // default size
                    drawPixel(x+i, y+j, bg);
                else // big size
                    fillRect(x+i*size, y+j*size, size, size, bg);
#else
                drawPixel(x+i, y+j, bg);
#endif
            }
            line >>= 1;
        }
    }
}

void MxGfx::setRotation(uint8_t x)
{
    x %= 4;  // cant be higher than 3
    rotation = x;
    switch (x)
    {
        case 0:
        case 2:
            _width = _rawWidth;
            _height = _rawHeight;
            break;
        case 1:
        case 3:
            _width = _rawHeight;
            _height = _rawWidth;
            break;
    }
}
