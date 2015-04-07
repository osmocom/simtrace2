/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2010, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
 * \file
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"
#include <stdint.h>
#include <string.h>

/** Frame buffer color cache size used to optimize memcpy */
#define FB_COLOR_CACHE_SIZE 8
/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/
/** Pointer to frame buffer. It is 16 bit aligned to allow PDC operations
 * LcdColor_t shall be defined in the physical lcd API (ili9225.c)
*/
static LcdColor_t *gpBuffer;
/** Frame buffer width */
static uint8_t gucWidth;
/** Frame buffer height */
static uint8_t gucHeight;
/* Pixel color cache */
static LcdColor_t gFbPixelCache[FB_COLOR_CACHE_SIZE];

/*----------------------------------------------------------------------------
 *        Static functions
 *----------------------------------------------------------------------------*/
/**
 * \brief Check Box coordinates. Return upper left and bottom right coordinates.
 *
 * \param pX1      X-coordinate of upper-left corner on LCD.
 * \param pY1      Y-coordinate of upper-left corner on LCD.
 * \param pX2      X-coordinate of lower-right corner on LCD.
 * \param pY2      Y-coordinate of lower-right corner on LCD.
 */
static void CheckBoxCoordinates( uint32_t *pX1, uint32_t *pY1, uint32_t *pX2, uint32_t *pY2 )
{
    uint32_t dw;

    if ( *pX1 >= gucWidth )
        *pX1=gucWidth-1 ;

    if ( *pX2 >= gucWidth )
        *pX2=gucWidth-1 ;

    if ( *pY1 >= gucHeight )
        *pY1=gucHeight-1 ;

    if ( *pY2 >= gucHeight )
        *pY2=gucHeight-1 ;

    if (*pX1 > *pX2) {
        dw = *pX1;
        *pX1 = *pX2;
        *pX2 = dw;
    }
    if (*pY1 > *pY2) {
        dw = *pY1;
        *pY1 = *pY2;
        *pY2 = dw;
    }
}

/*
 * \brief Draw a line on LCD, which is not horizontal or vertical.
 *
 * \param x         X-coordinate of line start.
 * \param y         Y-coordinate of line start.
 * \param length    line length.
 * \param direction line direction: 0 - horizontal, 1 - vertical.
 * \param color     Pixel color.
 */
static uint32_t DrawLineBresenham( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    int dx, dy ;
    int i ;
    int xinc, yinc, cumul ;
    int x, y ;

    x = dwX1 ;
    y = dwY1 ;
    dx = dwX2 - dwX1 ;
    dy = dwY2 - dwY1 ;

    xinc = ( dx > 0 ) ? 1 : -1 ;
    yinc = ( dy > 0 ) ? 1 : -1 ;
    dx = ( dx > 0 ) ? dx : -dx ;
    dy = ( dy > 0 ) ? dy : -dy ;

    FB_DrawPixel(x, y);

    if ( dx > dy )
    {
      cumul = dx / 2 ;
      for ( i = 1 ; i <= dx ; i++ )
      {
        x += xinc ;
        cumul += dy ;

        if ( cumul >= dx )
        {
          cumul -= dx ;
          y += yinc ;
        }
        FB_DrawPixel(x, y);
      }
    }
    else
    {
        cumul = dy / 2 ;
        for ( i = 1 ; i <= dy ; i++ )
        {
            y += yinc ;
            cumul += dx ;

            if ( cumul >= dy )
            {
                cumul -= dy ;
                x += xinc ;
            }

            FB_DrawPixel(x, y);
        }
    }

    return 0 ;
}

/*----------------------------------------------------------------------------
 *        External functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Configure the current frame buffer.
 * Next frame buffer operations will take place in this frame buffer area.
 * \param pBuffer 16 bit aligned sram buffer. PDC shall be able to access this
 *        memory area.
 * \param ucWidth frame buffer width
 * \param ucHeight frame buffer height
 */
extern void FB_SetFrameBuffer(LcdColor_t *pBuffer, uint8_t ucWidth, uint8_t ucHeight)
{
    /* Sanity check */
    assert(pBuffer != NULL);

    gpBuffer = pBuffer;
    gucWidth = ucWidth;
    gucHeight = ucHeight;
}


/**
 * \brief Configure the current color that will be used in the next graphical operations.
 *
 * \param dwRgb24Bits 24 bit rgb color
 */
extern void FB_SetColor(uint32_t dwRgb24Bits)
{
    uint16_t i;
//    LcdColor_t wColor;

//    wColor = (dwRgb24Bits & 0xF80000) >> 8 |
//             (dwRgb24Bits & 0x00FC00) >> 5 |
//             (dwRgb24Bits & 0x0000F8) >> 3;

    /* Fill the cache with selected color */
    for (i = 0; i < FB_COLOR_CACHE_SIZE; ++i) {
        gFbPixelCache[i] = dwRgb24Bits ;
    }
}
/**
 * \brief Draw a pixel on FB of given color.
 *
 * \param x  X-coordinate of pixel.
 * \param y  Y-coordinate of pixel.
 *
 * \return 0 is operation is successful, 1 if pixel is out of the fb
 */
extern uint32_t FB_DrawPixel(
    uint32_t dwX,
    uint32_t dwY)
{
    if ((dwX >= gucWidth) || (dwY >= gucHeight)) {
        return 1;
    }
    gpBuffer[dwX + dwY * gucWidth] = gFbPixelCache[0];

    return 0;
}

/**
 * \brief Write several pixels with the same color to FB.
 *
 * Pixel color is set by the LCD_SetColor() function.
 * This function is optimized using an sram buffer to transfer block instead of
 * individual pixels in order to limit the number of SPI interrupts.
 * \param dwX1      X-coordinate of upper-left corner on LCD.
 * \param dwY1      Y-coordinate of upper-left corner on LCD.
 * \param dwX2      X-coordinate of lower-right corner on LCD.
 * \param dwY2      Y-coordinate of lower-right corner on LCD.
 *
 * \return 0 if operation is successfull
 */
extern uint32_t FB_DrawFilledRectangle( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    LcdColor_t *pFbBuffer, *pDestFbBuffer;
    uint32_t dwY, blocks, bytes;

    /* Swap coordinates if necessary */
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    blocks  = ((dwX2 - dwX1 + 1) / FB_COLOR_CACHE_SIZE);
    bytes   = ((dwX2 - dwX1 + 1) % FB_COLOR_CACHE_SIZE);

    /* Each row is sent by block to benefit from memcpy optimizations */
    pFbBuffer = &(gpBuffer[dwY1 * gucWidth + dwX1]);
    for (dwY = dwY1; dwY <= dwY2; ++dwY) {
        pDestFbBuffer = pFbBuffer;
        while (blocks--) {
            memcpy(pDestFbBuffer, gFbPixelCache, FB_COLOR_CACHE_SIZE * sizeof(LcdColor_t));
            pDestFbBuffer += FB_COLOR_CACHE_SIZE;

        }
       memcpy(pDestFbBuffer, gFbPixelCache, bytes * sizeof(LcdColor_t));
       pFbBuffer += gucWidth;
    }

    return 0;
}

/**
 * \brief Write several pixels pre-formatted in a bufer to FB.
 *
 * \param dwX1      X-coordinate of upper-left corner on LCD.
 * \param dwY1      Y-coordinate of upper-left corner on LCD.
 * \param dwX2      X-coordinate of lower-right corner on LCD.
 * \param dwY2      Y-coordinate of lower-right corner on LCD.
 * \param pBuffer   pixel buffer area (no constraint on alignment).
 */
extern uint32_t FB_DrawPicture( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2, const void *pBuffer )
{
    LcdColor_t *pFbBuffer;
    uint32_t dwY;

    /* Swap coordinates if necessary */
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    pFbBuffer = &(gpBuffer[dwY1 * gucWidth + dwX1]);
    for (dwY = dwY1; dwY <= dwY2; ++dwY) {
       memcpy(pFbBuffer, pBuffer, (dwX2 - dwX1 + 1) * sizeof(LcdColor_t));
       pFbBuffer += gucWidth;
    }

    return 0 ;
}

/*
 * \brief Draw a line on LCD, horizontal and vertical line are supported.
 *
 * \param dwX1      X-coordinate of line start.
 * \param dwY1      Y-coordinate of line start.
 * \param dwX2      X-coordinate of line end.
 * \param dwY2      Y-coordinate of line end.
 *
 * \return 0 if operation is successful
*/
extern uint32_t FB_DrawLine ( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    /* Optimize horizontal or vertical line drawing */
    if (( dwY1 == dwY2 ) || (dwX1 == dwX2)) {
        FB_DrawFilledRectangle( dwX1, dwY1, dwX2, dwY2 );
    }
    else {
        DrawLineBresenham( dwX1, dwY1, dwX2, dwY2 ) ;
    }

    return 0 ;
}

/**
 * \brief Draws a circle in FB, at the given coordinates.
 *
 * \param dwX      X-coordinate of circle center.
 * \param dwY      Y-coordinate of circle center.
 * \param dwR      circle radius.
 *
 * \return 0 if operation is successful
*/
extern uint32_t FB_DrawCircle(
        uint32_t dwX,
        uint32_t dwY,
        uint32_t dwR)
{
    int32_t   d;    /* Decision Variable */
    uint32_t  curX; /* Current X Value */
    uint32_t  curY; /* Current Y Value */

    if (dwR == 0)
        return 0;
    d = 3 - (dwR << 1);
    curX = 0;
    curY = dwR;

    while (curX <= curY)
    {
        FB_DrawPixel(dwX + curX, dwY + curY);
        FB_DrawPixel(dwX + curX, dwY - curY);
        FB_DrawPixel(dwX - curX, dwY + curY);
        FB_DrawPixel(dwX - curX, dwY - curY);
        FB_DrawPixel(dwX + curY, dwY + curX);
        FB_DrawPixel(dwX + curY, dwY - curX);
        FB_DrawPixel(dwX - curY, dwY + curX);
        FB_DrawPixel(dwX - curY, dwY - curX);

        if (d < 0) {
            d += (curX << 2) + 6;
        }
        else {
            d += ((curX - curY) << 2) + 10;
            curY--;
        }
        curX++;
    }
    return 0;
}

/**
 * \brief Draws a filled circle in FB, at the given coordinates.
 *
 * \param dwX      X-coordinate of circle center.
 * \param dwY      Y-coordinate of circle center.
 * \param dwR      circle radius.
 *
 * \return 0 if operation is successful
*/
extern uint32_t FB_DrawFilledCircle( uint32_t dwX, uint32_t dwY, uint32_t dwRadius)
{
    signed int d ; // Decision Variable
    uint32_t dwCurX ; // Current X Value
    uint32_t dwCurY ; // Current Y Value
    uint32_t dwXmin, dwYmin;

    if (dwRadius == 0)
        return 0;
    d = 3 - (dwRadius << 1) ;
    dwCurX = 0 ;
    dwCurY = dwRadius ;

    while ( dwCurX <= dwCurY )
    {
        dwXmin = (dwCurX > dwX) ? 0 : dwX-dwCurX;
        dwYmin = (dwCurY > dwY) ? 0 : dwY-dwCurY;
        FB_DrawFilledRectangle( dwXmin, dwYmin, dwX+dwCurX, dwYmin ) ;
        FB_DrawFilledRectangle( dwXmin, dwY+dwCurY, dwX+dwCurX, dwY+dwCurY ) ;
        dwXmin = (dwCurY > dwX) ? 0 : dwX-dwCurY;
        dwYmin = (dwCurX > dwY) ? 0 : dwY-dwCurX;
        FB_DrawFilledRectangle( dwXmin, dwYmin, dwX+dwCurY, dwYmin ) ;
        FB_DrawFilledRectangle( dwXmin, dwY+dwCurX, dwX+dwCurY, dwY+dwCurX ) ;

        if ( d < 0 )
        {
            d += (dwCurX << 2) + 6 ;
        }
        else
        {
            d += ((dwCurX - dwCurY) << 2) + 10;
            dwCurY-- ;
        }

        dwCurX++ ;
    }

    return 0 ;
}

/**
* Pixel color is set by the LCD_SetColor() function.
* This function is optimized using an sram buffer to transfer block instead of
* individual pixels in order to limit the number of SPI interrupts.
* \param dwX1      X-coordinate of upper-left corner on LCD.
* \param dwY1      Y-coordinate of upper-left corner on LCD.
* \param dwX2      X-coordinate of lower-right corner on LCD.
* \param dwY2      Y-coordinate of lower-right corner on LCD.
*
* \return 0 if operation is successfull
*/
extern uint32_t FB_DrawRectangle( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    FB_DrawFilledRectangle( dwX1, dwY1, dwX2, dwY1 ) ;
    FB_DrawFilledRectangle( dwX1, dwY2, dwX2, dwY2 ) ;

    FB_DrawFilledRectangle( dwX1, dwY1, dwX1, dwY2 ) ;
    FB_DrawFilledRectangle( dwX2, dwY1, dwX2, dwY2 ) ;

    return 0 ;
}



