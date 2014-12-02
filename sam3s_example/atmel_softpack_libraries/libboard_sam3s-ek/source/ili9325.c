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
 * Implementation of ILI9325 driver.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/
#include "board.h"

#include <string.h>
#include <stdio.h>

#ifdef BOARD_LCD_ILI9325

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/* Pixel cache used to speed up communication */
#define LCD_DATA_CACHE_SIZE BOARD_LCD_WIDTH
static LcdColor_t gLcdPixelCache[LCD_DATA_CACHE_SIZE];

/*----------------------------------------------------------------------------
 *        Export functions
 *----------------------------------------------------------------------------*/


/**
 * \brief Write data to LCD Register.
 *
 * \param reg   Register address.
 * \param data  Data to be written.
 */
static void LCD_WriteReg( uint8_t reg, uint16_t data )
{
    LCD_IR() = 0;
    LCD_IR() = reg;
    LCD_D()  = (data >> 8) & 0xFF;
    LCD_D()  = data & 0xFF;
}

/**
 * \brief Read data from LCD Register.
 *
 * \param reg   Register address.
 *
 * \return      Readed data.
 */
static uint16_t LCD_ReadReg( uint8_t reg )
{
    uint16_t value;

    LCD_IR() = 0;
    LCD_IR() = reg;

    value = LCD_D();
    value = (value << 8) | LCD_D();

    return value;
}


/**
 * \brief Prepare to write GRAM data.
 */
extern void LCD_WriteRAM_Prepare( void )
{
    LCD_IR() = 0 ;
    LCD_IR() = ILI9325_R22H ; /* Write Data to GRAM (R22h)  */
}

/**
 * \brief Write data to LCD GRAM.
 *
 * \param color  24-bits RGB color.
 */
extern  void LCD_WriteRAM( LcdColor_t dwColor )
{
    LCD_D() = ((dwColor >> 16) & 0xFF);
    LCD_D() = ((dwColor >> 8) & 0xFF);
    LCD_D() = (dwColor & 0xFF);
}

/**
 * \brief Write mutiple data in buffer to LCD controller.
 *
 * \param pBuf  data buffer.
 * \param size  size in pixels.
 */
static void LCD_WriteRAMBuffer(const LcdColor_t *pBuf, uint32_t size)
{
    uint32_t addr ;

    for ( addr = 0 ; addr < size ; addr++ )
    {
        LCD_WriteRAM(pBuf[addr]);
    }
}

/**
 * \brief Prepare to read GRAM data.
 */
extern void LCD_ReadRAM_Prepare( void )
{
    LCD_IR() = 0 ;
    LCD_IR() = ILI9325_R22H ; /* Write Data to GRAM (R22h)  */
}

/**
 * \brief Read data to LCD GRAM.
 *
 * \note Because pixel data LCD GRAM is 18-bits, so convertion to RGB 24-bits
 * will cause low color bit lose.
 *
 * \return color  24-bits RGB color.
 */
extern uint32_t LCD_ReadRAM( void )
{
    uint8_t value[2];
    uint32_t color;

    value[0] = LCD_D();       /* dummy read */
    value[1] = LCD_D();       /* dummy read */
    value[0] = LCD_D();       /* data upper byte */
    value[1] = LCD_D();       /* data lower byte */

    /* Convert RGB565 to RGB888 */
    /* For BGR format */
    color = ((value[0] & 0xF8)) |                                  /* R */
            ((value[0] & 0x07) << 13) | ((value[1] & 0xE0) << 5) | /* G */
            ((value[1] & 0x1F) << 19);                             /* B */
    return color;
}

/*----------------------------------------------------------------------------
 *        Basic ILI9225 primitives
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

    if ( *pX1 >= BOARD_LCD_WIDTH )
    {
        *pX1 = BOARD_LCD_WIDTH-1 ;
    }
    if ( *pX2 >= BOARD_LCD_WIDTH )
    {
        *pX2 = BOARD_LCD_WIDTH-1 ;
    }
    if ( *pY1 >= BOARD_LCD_HEIGHT )
    {
        *pY1 = BOARD_LCD_HEIGHT-1 ;
    }
    if ( *pY2 >= BOARD_LCD_HEIGHT )
    {
        *pY2 = BOARD_LCD_HEIGHT-1 ;
    }
    if (*pX1 > *pX2)
    {
        dw = *pX1;
        *pX1 = *pX2;
        *pX2 = dw;
    }
    if (*pY1 > *pY2)
    {
        dw = *pY1;
        *pY1 = *pY2;
        *pY2 = dw;
    }
}

/**
 * \brief Initialize the LCD controller.
 */
extern uint32_t LCD_Initialize( void )
{
    uint16_t chipid ;

    /* Check ILI9325 chipid */
    chipid = LCD_ReadReg( ILI9325_R00H ) ; /* Driver Code Read (R00h) */
    if ( chipid != ILI9325_DEVICE_CODE )
    {
        printf( "Read ILI9325 chip ID (0x%04x) error, skip initialization.\r\n", chipid ) ;
        return 1 ;
    }

    /* Turn off LCD */
    LCD_PowerDown() ;

    /* Start initial sequence */
    LCD_WriteReg(ILI9325_R10H, 0x0000); /* DSTB = LP = STB = 0 */
    LCD_WriteReg(ILI9325_R00H, 0x0001); /* start internal OSC */
    LCD_WriteReg(ILI9325_R01H, ILI9325_R01H_SS ) ; /* set SS and SM bit */
    LCD_WriteReg(ILI9325_R02H, 0x0700); /* set 1 line inversion */
    //LCD_WriteReg(ILI9325_R03H, 0xD030); /* set GRAM write direction and BGR=1. */
    LCD_WriteReg(ILI9325_R04H, 0x0000); /* Resize register */
    LCD_WriteReg(ILI9325_R08H, 0x0207); /* set the back porch and front porch */
    LCD_WriteReg(ILI9325_R09H, 0x0000); /* set non-display area refresh cycle ISC[3:0] */
    LCD_WriteReg(ILI9325_R0AH, 0x0000); /* FMARK function */
    LCD_WriteReg(ILI9325_R0CH, 0x0000); /* RGB interface setting */
    LCD_WriteReg(ILI9325_R0DH, 0x0000); /* Frame marker Position */
    LCD_WriteReg(ILI9325_R0FH, 0x0000); /* RGB interface polarity */

    /* Power on sequence */
    LCD_WriteReg(ILI9325_R10H, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(ILI9325_R11H, 0x0000); /* DC1[2:0], DC0[2:0], VC[2:0] */
    LCD_WriteReg(ILI9325_R12H, 0x0000); /* VREG1OUT voltage */
    LCD_WriteReg(ILI9325_R13H, 0x0000); /* VDV[4:0] for VCOM amplitude */
    Wait( 200 ) ;                       /* Dis-charge capacitor power voltage */
    LCD_WriteReg(ILI9325_R10H, 0x1290); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(ILI9325_R11H, 0x0227); /* DC1[2:0], DC0[2:0], VC[2:0] */
    Wait( 50 ) ;
    LCD_WriteReg(ILI9325_R12H, 0x001B); /* Internal reference voltage= Vci; */
    Wait( 50 ) ;
    LCD_WriteReg(ILI9325_R13H, 0x1100); /* Set VDV[4:0] for VCOM amplitude */
    LCD_WriteReg(ILI9325_R29H, 0x0019); /* Set VCM[5:0] for VCOMH */
    LCD_WriteReg(ILI9325_R2BH, 0x000D); /* Set Frame Rate */
    Wait( 50 ) ;

    /* Adjust the Gamma Curve */
    LCD_WriteReg(ILI9325_R30H, 0x0000);
    LCD_WriteReg(ILI9325_R31H, 0x0204);
    LCD_WriteReg(ILI9325_R32H, 0x0200);
    LCD_WriteReg(ILI9325_R35H, 0x0007);
    LCD_WriteReg(ILI9325_R36H, 0x1404);
    LCD_WriteReg(ILI9325_R37H, 0x0705);
    LCD_WriteReg(ILI9325_R38H, 0x0305);
    LCD_WriteReg(ILI9325_R39H, 0x0707);
    LCD_WriteReg(ILI9325_R3CH, 0x0701);
    LCD_WriteReg(ILI9325_R3DH, 0x000e);

    LCD_SetDisplayPortrait( 0 ) ;
    /* Vertical Scrolling */
    LCD_WriteReg( ILI9325_R61H, 0x0001 ) ;
    LCD_WriteReg( ILI9325_R6AH, 0x0000 ) ;

    /* Partial Display Control */
    LCD_WriteReg(ILI9325_R80H, 0x0000);
    LCD_WriteReg(ILI9325_R81H, 0x0000);
    LCD_WriteReg(ILI9325_R82H, 0x0000);
    LCD_WriteReg(ILI9325_R83H, 0x0000);
    LCD_WriteReg(ILI9325_R84H, 0x0000);
    LCD_WriteReg(ILI9325_R85H, 0x0000);

    /* Panel Control */
    LCD_WriteReg(ILI9325_R90H, 0x0010);
    LCD_WriteReg(ILI9325_R92H, 0x0600);
    LCD_WriteReg(ILI9325_R95H, 0x0110);

    LCD_SetWindow( 0, 0, BOARD_LCD_WIDTH, BOARD_LCD_HEIGHT ) ;
    LCD_SetCursor( 0, 0 ) ;

    return 0 ;
}

/**
 * \brief Turn on the LCD.
 */
extern void LCD_On( void )
{
    /* Display Control 1 (R07h) */
    /* When BASEE = “1”, the base image is displayed. */
    /* GON and DTE Set the output level of gate driver G1 ~ G320 : Normal Display */
    /* D1=1 D0=1 BASEE=1: Base image display Operate */
    LCD_WriteReg( ILI9325_R07H,   ILI9325_R07H_BASEE
                                | ILI9325_R07H_GON | ILI9325_R07H_DTE
                                | ILI9325_R07H_D1  | ILI9325_R07H_D0 ) ;
}


/**
 * \brief Turn off the LCD.
 */
extern void LCD_Off( void )
{
    /* Display Control 1 (R07h) */
    /* When BASEE = “0”, no base image is displayed. */
    /* When the display is turned off by setting D[1:0] = “00”, the ILI9325 internal display
       operation is halted completely. */
    /* PTDE1/0 = 0: turns off partial image. */
    LCD_WriteReg( ILI9325_R07H, 0x00 ) ;
}

/**
 * \brief Power down the LCD.
 */
extern void LCD_PowerDown( void )
{
    /* Display Control 1 (R07h) */
    /* When BASEE = “0”, no base image is displayed. */
    /* GON and DTE Set the output level of gate driver G1 ~ G320 : Normal Display */
    /* D1=1 D0=1 BASEE=1: Base image display Operate */
    LCD_WriteReg( ILI9325_R07H,   ILI9325_R07H_GON | ILI9325_R07H_DTE
                                | ILI9325_R07H_D1  | ILI9325_R07H_D0 ) ;
}

/**
 * \brief Convert 24 bit RGB color into 5-6-5 rgb color space.
 *
 * Initialize the LcdColor_t cache with the color pattern.
 * \param x  24-bits RGB color.
 * \return 0 for successfull operation.
 */
extern uint32_t LCD_SetColor( uint32_t dwRgb24Bits )
{
    uint32_t i ;

    /* Fill the cache with selected color */
    for ( i = 0 ; i < LCD_DATA_CACHE_SIZE ; ++i )
    {
        gLcdPixelCache[i] = dwRgb24Bits ;
    }

    return 0;
}

/**
 * \brief Set cursor of LCD srceen.
 *
 * \param x  X-coordinate of upper-left corner on LCD.
 * \param y  Y-coordinate of upper-left corner on LCD.
 */
extern void LCD_SetCursor( uint16_t x, uint16_t y )
{
    /* GRAM Horizontal/Vertical Address Set (R20h, R21h) */
    LCD_WriteReg( ILI9325_R20H, x ) ; /* column */
    LCD_WriteReg( ILI9325_R21H, y ) ; /* row */
}

extern void LCD_SetWindow( uint32_t dwX, uint32_t dwY, uint32_t dwWidth, uint32_t dwHeight )
{
    /* Horizontal and Vertical RAM Address Position (R50h, R51h, R52h, R53h) */

    /* Set Horizontal Address Start Position */
   LCD_WriteReg( ILI9325_R50H, (uint16_t)dwX ) ;

   /* Set Horizontal Address End Position */
   LCD_WriteReg( ILI9325_R51H, (uint16_t)dwX+dwWidth-1 ) ;

   /* Set Vertical Address Start Position */
   LCD_WriteReg( ILI9325_R52H, (uint16_t)dwY ) ;

   /* Set Vertical Address End Position */
   LCD_WriteReg( ILI9325_R53H, (uint16_t)dwY+dwHeight-1 ) ;
}

extern void LCD_SetDisplayLandscape( uint32_t dwRGB )
{
    uint16_t dwValue ;

    /* When AM = “1”, the address is updated in vertical writing direction. */
    /* DFM Set the mode of transferring data to the internal RAM when TRI = “1”. */
    /* When TRI = “1”, data are transferred to the internal RAM in 8-bit x 3 transfers mode via the 8-bit interface. */
    /* Use the high speed write mode (HWM=1) */
    /* ORG = “1”: The original address “00000h” moves according to the I/D[1:0] setting.  */
    /* I/D[1:0] = 00 Horizontal : decrement Vertical :  decrement, AM=0:Horizontal */
    dwValue = ILI9325_R03H_AM | ILI9325_R03H_DFM | ILI9325_R03H_TRI | ILI9325_R03H_HWM | ILI9325_R03H_ORG ;

    if ( dwRGB == 0 )
    {
        /* BGR=”1”: Swap the RGB data to BGR in writing into GRAM. */
        dwValue |= ILI9325_R03H_BGR ;
    }
    LCD_WriteReg( ILI9325_R03H, dwValue ) ;

    //    LCD_WriteReg( ILI9325_R60H, (0x1d<<8)|0x00 ) ; /*Gate Scan Control */

    LCD_SetWindow( 0, 0, BOARD_LCD_HEIGHT, BOARD_LCD_WIDTH ) ;
}

extern void LCD_SetDisplayPortrait( uint32_t dwRGB )
{
    uint16_t dwValue ;

    /* Use the high speed write mode (HWM=1) */
    /* When TRI = “1”, data are transferred to the internal RAM in 8-bit x 3 transfers mode via the 8-bit interface. */
    /* DFM Set the mode of transferring data to the internal RAM when TRI = “1”. */
    /* I/D[1:0] = 11 Horizontal : increment Vertical :  increment, AM=0:Horizontal */
    dwValue = ILI9325_R03H_HWM | ILI9325_R03H_TRI | ILI9325_R03H_DFM | ILI9325_R03H_ID1 | ILI9325_R03H_ID0 ;

    if ( dwRGB == 0 )
    {
        /* BGR=”1”: Swap the RGB data to BGR in writing into GRAM. */
        dwValue |= ILI9325_R03H_BGR ;
    }
    LCD_WriteReg( ILI9325_R03H, dwValue ) ;
    /* Gate Scan Control (R60h, R61h, R6Ah) */
    /* SCN[5:0] = 00 */
    /* NL[5:0]: Sets the number of lines to drive the LCD at an interval of 8 lines. */
    LCD_WriteReg( ILI9325_R60H, ILI9325_R60H_GS|(0x27<<8)|0x00 ) ;
}


extern void LCD_VerticalScroll( uint16_t wY )
{
    /* Gate Scan Control (R60h, R61h, R6Ah) */
    /*  Enables the grayscale inversion of the image by setting REV=1. */
    /* VLE: Vertical scroll display enable bit */
    LCD_WriteReg( ILI9325_R61H, 3 ) ;
    LCD_WriteReg( ILI9325_R6AH, wY ) ;
}


extern void LCD_SetPartialImage1( uint32_t dwDisplayPos, uint32_t dwStart, uint32_t dwEnd )
{
    assert( dwStart <= dwEnd ) ;

    /* Partial Image 1 Display Position (R80h) */
    LCD_WriteReg( ILI9325_R80H, dwDisplayPos&0x1ff ) ;
    /* Partial Image 1 RAM Start/End Address (R81h, R82h) */
    LCD_WriteReg( ILI9325_R81H, dwStart&0x1ff ) ;
    LCD_WriteReg( ILI9325_R82H, dwEnd&0x1ff ) ;
}

extern void LCD_SetPartialImage2( uint32_t dwDisplayPos, uint32_t dwStart, uint32_t dwEnd )
{
    assert( dwStart <= dwEnd ) ;

    /* Partial Image 2 Display Position (R83h) */
    LCD_WriteReg( ILI9325_R83H, dwDisplayPos&0x1ff ) ;
    /* Partial Image 2 RAM Start/End Address (R84h, R85h) */
    LCD_WriteReg( ILI9325_R84H, dwStart&0x1ff ) ;
    LCD_WriteReg( ILI9325_R85H, dwEnd&0x1ff ) ;
}

/**
 * \brief Draw a LcdColor_t on LCD of given color.
 *
 * \param x  X-coordinate of pixel.
 * \param y  Y-coordinate of pixel.
 */
extern uint32_t LCD_DrawPixel( uint32_t x, uint32_t y )
{
    if( (x >= BOARD_LCD_WIDTH) || (y >= BOARD_LCD_HEIGHT) )
    {
        return 1;
    }

    /* Set cursor */
    LCD_SetCursor( x, y );

    /* Prepare to write in GRAM */
    LCD_WriteRAM_Prepare();
    LCD_WriteRAM( *gLcdPixelCache );

    return 0;
}



extern void LCD_TestPattern( uint32_t dwRGB )
{
    uint32_t dwLine ;
    uint32_t dw ;

    LCD_SetWindow( 10, 10, 100, 20 ) ;
    LCD_SetCursor( 10, 10 ) ;
    LCD_WriteRAM_Prepare() ;

    for ( dwLine=0 ; dwLine < 20 ; dwLine++ )
    {
        /* Draw White bar */
        for ( dw=0 ; dw < 20 ; dw++ )
        {
                LCD_D() = 0xff ;
                LCD_D() = 0xff ;
                LCD_D() = 0xff ;
        }
        /* Draw Red bar */
        for ( dw=0 ; dw < 20 ; dw++ )
        {
            if ( dwRGB == 0 )
            {
                LCD_D() = 0xff ;
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
            }
            else
            {
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
                LCD_D() = 0xff ;
            }
        }
        /* Draw Green bar */
        for ( dw=0 ; dw < 20 ; dw++ )
        {
                LCD_D() = 0x00 ;
                LCD_D() = 0xff ;
                LCD_D() = 0x00 ;
        }
        /* Draw Blue bar */
        for ( dw=0 ; dw < 20 ; dw++ )
        {
            if ( dwRGB == 0 )
            {
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
                LCD_D() = 0xff ;
            }
            else
            {
                LCD_D() = 0xff ;
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
            }
        }
        /* Draw Black bar */
        for ( dw=0 ; dw < 20 ; dw++ )
        {
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
                LCD_D() = 0x00 ;
        }
    }

    LCD_SetWindow( 0, 0, BOARD_LCD_WIDTH, BOARD_LCD_HEIGHT ) ;
}


/**
 * \brief Write several pixels with the same color to LCD GRAM.
 *
 * LcdColor_t color is set by the LCD_SetColor() function.
 * This function is optimized using an sram buffer to transfer block instead of
 * individual pixels in order to limit the number of SPI interrupts.
 * \param dwX1      X-coordinate of upper-left corner on LCD.
 * \param dwY1      Y-coordinate of upper-left corner on LCD.
 * \param dwX2      X-coordinate of lower-right corner on LCD.
 * \param dwY2      Y-coordinate of lower-right corner on LCD.
 */
extern uint32_t LCD_DrawFilledRectangle( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    uint32_t size, blocks;

    /* Swap coordinates if necessary */
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    /* Determine the refresh window area */
    /* Horizontal and Vertical RAM Address Position (R50h, R51h, R52h, R53h) */
    LCD_WriteReg(ILI9325_R50H, (uint16_t)dwX1);
    LCD_WriteReg(ILI9325_R51H, (uint16_t)dwX2);
    LCD_WriteReg(ILI9325_R52H, (uint16_t)dwY1);
    LCD_WriteReg(ILI9325_R53H, (uint16_t)dwY2);

    /* Set cursor */
    LCD_SetCursor( dwX1, dwY1 );

    /* Prepare to write in GRAM */
    LCD_WriteRAM_Prepare();

    size = (dwX2 - dwX1 + 1) * (dwY2 - dwY1 + 1);
    /* Send pixels blocks => one SPI IT / block */
    blocks = size / LCD_DATA_CACHE_SIZE;
    while (blocks--)
    {
        LCD_WriteRAMBuffer(gLcdPixelCache, LCD_DATA_CACHE_SIZE);
    }
    /* Send remaining pixels */
    LCD_WriteRAMBuffer(gLcdPixelCache, size % LCD_DATA_CACHE_SIZE);

    /* Reset the refresh window area */
    /* Horizontal and Vertical RAM Address Position (R50h, R51h, R52h, R53h) */
    LCD_WriteReg(ILI9325_R50H, (uint16_t)0 ) ;
    LCD_WriteReg(ILI9325_R51H, (uint16_t)BOARD_LCD_WIDTH - 1 ) ;
    LCD_WriteReg(ILI9325_R52H, (uint16_t)0) ;
    LCD_WriteReg(ILI9325_R53H, (uint16_t)BOARD_LCD_HEIGHT - 1  ) ;

    return 0 ;
}

/**
 * \brief Write several pixels pre-formatted in a bufer to LCD GRAM.
 *
 * \param dwX1      X-coordinate of upper-left corner on LCD.
 * \param dwY1      Y-coordinate of upper-left corner on LCD.
 * \param dwX2      X-coordinate of lower-right corner on LCD.
 * \param dwY2      Y-coordinate of lower-right corner on LCD.
 * \param pBuffer   LcdColor_t buffer area.
 */
extern uint32_t LCD_DrawPicture( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2, const LcdColor_t *pBuffer )
{
    uint32_t size, blocks;
    LcdColor_t currentColor;

    /* Swap coordinates if necessary */
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    /* Determine the refresh window area */
    /* Horizontal and Vertical RAM Address Position (R50h, R51h, R52h, R53h) */
    LCD_WriteReg(ILI9325_R50H, (uint16_t)dwX1 ) ;
    LCD_WriteReg(ILI9325_R51H, (uint16_t)dwX2 ) ;
    LCD_WriteReg(ILI9325_R52H, (uint16_t)dwY1 ) ;
    LCD_WriteReg(ILI9325_R53H, (uint16_t)dwY2 ) ;

    /* Set cursor */
    LCD_SetCursor( dwX1, dwY1 );

    /* Prepare to write in GRAM */
    LCD_WriteRAM_Prepare();

    size = (dwX2 - dwX1 + 1) * (dwY2 - dwY1 + 1);

    /* Check if the buffer is within the SRAM */
    if ((IRAM_ADDR <= (uint32_t)pBuffer) && ((uint32_t)pBuffer < (IRAM_ADDR+IRAM_SIZE)))
    {
        LCD_WriteRAMBuffer(pBuffer, size);
    }
    /* If the buffer is not in SRAM, transfer it in SRAM first before transfer */
    else
    {
        /* Use color buffer as a cache */
        currentColor = gLcdPixelCache[0];
        /* Send pixels blocks => one SPI IT / block */
        blocks = size / LCD_DATA_CACHE_SIZE;
        while (blocks--)
        {
            memcpy(gLcdPixelCache, pBuffer, LCD_DATA_CACHE_SIZE * sizeof(LcdColor_t));
            LCD_WriteRAMBuffer(gLcdPixelCache, LCD_DATA_CACHE_SIZE);
            pBuffer += LCD_DATA_CACHE_SIZE;
        }
        /* Send remaining pixels */
        memcpy(gLcdPixelCache, pBuffer, (size % LCD_DATA_CACHE_SIZE) * sizeof(LcdColor_t));
        LCD_WriteRAMBuffer(gLcdPixelCache, size % LCD_DATA_CACHE_SIZE);

        /* Restore the color cache */
        LCD_SetColor(currentColor);
    }

    /* Reset the refresh window area */
    /* Horizontal and Vertical RAM Address Position (R50h, R51h, R52h, R53h) */
    LCD_WriteReg(ILI9325_R50H, (uint16_t)0 ) ;
    LCD_WriteReg(ILI9325_R51H, (uint16_t)BOARD_LCD_WIDTH - 1 ) ;
    LCD_WriteReg(ILI9325_R52H, (uint16_t)0 ) ;
    LCD_WriteReg(ILI9325_R53H, (uint16_t)BOARD_LCD_HEIGHT - 1 ) ;

    return 0 ;
}

/*
 * \brief Draw a line on LCD, which is not horizontal or vertical.
 *
 * \param x         X-coordinate of line start.
 * \param y         Y-coordinate of line start.
 * \param length    line length.
 * \param direction line direction: 0 - horizontal, 1 - vertical.
 * \param color     LcdColor_t color.
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

	LCD_DrawPixel( x, y ) ;

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
		LCD_DrawPixel( x, y ) ;
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

			LCD_DrawPixel( x, y ) ;
		}
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
  */
extern uint32_t LCD_DrawLine ( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    /* Optimize horizontal or vertical line drawing */
    if (( dwY1 == dwY2 ) || (dwX1 == dwX2))
    {
        LCD_DrawFilledRectangle( dwX1, dwY1, dwX2, dwY2 );
    }
    else
    {
        DrawLineBresenham( dwX1, dwY1, dwX2, dwY2 ) ;
    }

    return 0 ;
}

/**
 * \brief Draws a circle on LCD, at the given coordinates.
 *
 * \param dwX      X-coordinate of circle center.
 * \param dwY      Y-coordinate of circle center.
 * \param dwR      circle radius.
*/
extern uint32_t LCD_DrawCircle( uint32_t dwX, uint32_t dwY, uint32_t dwR )
{
    int32_t   d;    /* Decision Variable */
    uint32_t  curX; /* Current X Value */
    uint32_t  curY; /* Current Y Value */

    if (dwR == 0)
    {
        return 0;
    }
    d = 3 - (dwR << 1);
    curX = 0;
    curY = dwR;

    while (curX <= curY)
    {
        LCD_DrawPixel(dwX + curX, dwY + curY);
        LCD_DrawPixel(dwX + curX, dwY - curY);
        LCD_DrawPixel(dwX - curX, dwY + curY);
        LCD_DrawPixel(dwX - curX, dwY - curY);
        LCD_DrawPixel(dwX + curY, dwY + curX);
        LCD_DrawPixel(dwX + curY, dwY - curX);
        LCD_DrawPixel(dwX - curY, dwY + curX);
        LCD_DrawPixel(dwX - curY, dwY - curX);

        if (d < 0)
        {
            d += (curX << 2) + 6;
        }
        else
        {
            d += ((curX - curY) << 2) + 10;
            curY--;
        }
        curX++;
    }
    return 0;
}

extern uint32_t LCD_DrawFilledCircle( uint32_t dwX, uint32_t dwY, uint32_t dwRadius)
{
    signed int d ; /* Decision Variable */
    uint32_t dwCurX ; /* Current X Value */
    uint32_t dwCurY ; /* Current Y Value */
    uint32_t dwXmin, dwYmin;

    if (dwRadius == 0)
    {
        return 0;
    }
    d = 3 - (dwRadius << 1) ;
    dwCurX = 0 ;
    dwCurY = dwRadius ;

    while ( dwCurX <= dwCurY )
    {
        dwXmin = (dwCurX > dwX) ? 0 : dwX-dwCurX;
        dwYmin = (dwCurY > dwY) ? 0 : dwY-dwCurY;
        LCD_DrawFilledRectangle( dwXmin, dwYmin, dwX+dwCurX, dwYmin ) ;
        LCD_DrawFilledRectangle( dwXmin, dwY+dwCurY, dwX+dwCurX, dwY+dwCurY ) ;
        dwXmin = (dwCurY > dwX) ? 0 : dwX-dwCurY;
        dwYmin = (dwCurX > dwY) ? 0 : dwY-dwCurX;
        LCD_DrawFilledRectangle( dwXmin, dwYmin, dwX+dwCurY, dwYmin ) ;
        LCD_DrawFilledRectangle( dwXmin, dwY+dwCurX, dwX+dwCurY, dwY+dwCurX ) ;

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

extern uint32_t LCD_DrawRectangle( uint32_t dwX1, uint32_t dwY1, uint32_t dwX2, uint32_t dwY2 )
{
    CheckBoxCoordinates(&dwX1, &dwY1, &dwX2, &dwY2);

    LCD_DrawFilledRectangle( dwX1, dwY1, dwX2, dwY1 ) ;
    LCD_DrawFilledRectangle( dwX1, dwY2, dwX2, dwY2 ) ;

    LCD_DrawFilledRectangle( dwX1, dwY1, dwX1, dwY2 ) ;
    LCD_DrawFilledRectangle( dwX2, dwY1, dwX2, dwY2 ) ;

    return 0 ;
}


/**
 * \brief Set the backlight of the LCD (AAT3155).
 *
 * \param level   Backlight brightness level [1..16], 1 means maximum brightness.
 */
extern void LCD_SetBacklight (uint32_t level)
{
    uint32_t i;
    const Pin pPins[] = {BOARD_BACKLIGHT_PIN};

    /* Ensure valid level */
    level = (level < 1) ? 1 : level;
    level = (level > 16) ? 16 : level;

    /* Enable pins */
    PIO_Configure(pPins, PIO_LISTSIZE(pPins));

    /* Switch off backlight */
    PIO_Clear(pPins);
    i = 600 * (BOARD_MCK / 1000000);    /* wait for at least 500us */
    while(i--);

    /* Set new backlight level */
    for (i = 0; i < level; i++)
    {
        PIO_Clear(pPins);
        PIO_Clear(pPins);
        PIO_Clear(pPins);

        PIO_Set(pPins);
        PIO_Set(pPins);
        PIO_Set(pPins);
    }
}

#endif
