/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Copyright (c) Botbench 2019, All rights reserved.                       */
/*                                                                            */
/*    Module:     main.c                                                      */
/*    Author:     Xander Soldaat, based on code found here:                   */
/*                lodev.org and iotexpert.com                                 */
/*    Created:    August 2019                                                 */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>
#include <math.h>

#include "odroid_display.h"
#include "ugui.h"

#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#define PALETTE_SIZE    256

static const char* TAG = "Plasma";

// This is a rainbow palette that loops around
const uint16_t palette[] = {
        63488,	63520,	63584,	63616,	63680,	63712,	63776,	63808,
        63872,	63904,	63968,	64000,	64064,	64096,	64160,	64192,
        64224,	64288,	64320,	64384,	64416,	64480,	64512,	64576,
        64608,	64672,	64704,	64768,	64800,	64864,	64896,	64960,
        64992,	65056,	65088,	65152,	65184,	65248,	65280,	65344,
        65376,	65440,	65472,	65504,	63456,	63456,	61408,	59360,
        57312,	57312,	55264,	53216,	51168,	51168,	49120,	47072,
        45024,	45024,	42976,	40928,	38880,	38880,	36832,	34784,
        32736,	32736,	30688,	28640,	26592,	26592,	24544,	22496,
        22496,	20448,	18400,	16352,	16352,	14304,	12256,	10208,
        10208,	8160,	6112,	4064,	4064,	2016,	2016,	2017,
        2018,	2018,	2019,	2020,	2021,	2021,	2022,	2023,
        2024,	2024,	2025,	2026,	2026,	2027,	2028,	2029,
        2029,	2030,	2031,	2032,	2032,	2033,	2034,	2035,
        2035,	2036,	2037,	2038,	2038,	2039,	2040,	2041,
        2041,	2042,	2043,	2044,	2044,	2045,	2046,	2047,
        2047,	2015,	1951,	1919,	1855,	1823,	1759,	1727,
        1663,	1631,	1567,	1535,	1471,	1439,	1375,	1343,
        1279,	1247,	1183,	1151,	1087,	1055,	991,	959,
        895,	863,	799,	767,	703,	671,	639,	575,
        543,	479,	447,	383,	351,	287,	255,	191,
        159,	95,	    63,	    31,	    2079,	2079,	4127,	6175,
        8223,	8223,	10271,	12319,	14367,	14367,	16415,	18463,
        20511,	20511,	22559,	24607,	24607,	26655,	28703,	30751,
        30751,	32799,	34847,	36895,	36895,	38943,	40991,	43039,
        43039,	45087,	47135,	49183,	49183,	51231,	53279,	55327,
        55327,	57375,	59423,	61471,	61471,	63519,	63519,	63518,
        63517,	63517,	63516,	63515,	63514,	63514,	63513,	63512,
        63511,	63511,	63510,	63509,	63508,	63508,	63507,	63506,
        63505,	63505,	63504,	63503,	63502,	63502,	63501,	63500,
        63499,	63499,	63498,	63497,	63497,	63496,	63495,	63494,
        63494,	63493,	63492,	63491,	63491,	63490,	63489,	63488,
    };

// Frame buffer and pointer to plasma array 
uint16_t fb [ LCD_WIDTH * LCD_HEIGHT ];
uint8_t *plasma;

// Struct for all UI related data
UG_GUI gui;

// Get milliseconds elapsed. Not super accurate.
unsigned long IRAM_ATTR millis()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
* Print a centered string
* @param {uint32_t} x position on X axis
* @param {uint32_t} y position on Y axis
* @param {uint32_t} fontWidth the width of the font
* @param {uint32_t} fontHeight the height of the font
* @param {char *} stringToPrint 
*/   
static void UG_PutstringCenter ( uint32_t x, uint32_t y, uint32_t fontWidth, uint32_t fontHeight, char *stringToPrint )
{
    y = y - fontHeight / 2;
    x = x - ( strlen ( stringToPrint ) / 2 ) * fontWidth;

    if ( strlen ( stringToPrint ) % 2 )
        x = x - fontWidth / 2;

    UG_PutString ( x, y, stringToPrint );
}

/**
* RGB565 conversion
* RGB565 is R(5)+G(6)+B(5)=16bit color format.
* Bit image "RRRRRGGGGGGBBBBB"
* @param {uint16_t} red the red component
* @param {uint16_t} green the green component
* @param {uint16_t} blue the blue component
* @return {uint16_t} RGB565 value
*/   
uint16_t rgb565_conv ( uint16_t red, uint16_t green, uint16_t blue ) {
	return ( ( ( red & 0xF8 ) << 8 ) | ( ( green & 0xFC ) << 3 ) | ( blue >> 3 ) );
}

/**
* HSV to RGB565 conversion
* @param {uint8_t} hue the hue component
* @param {uint8_t} sat the saturation component
* @param {uint8_t} val the value component
* @return {uint16_t} RGB565 value
*/   
uint16_t HSVtoRGB ( uint8_t hue, uint8_t sat, uint8_t val )
{
    float h = hue / 256.0;
    float s = sat / 256.0;
    float v = val / 256.0;
    float r = 0.0;
    float g = 0.0;
    float b = 0.0;

    //If saturation is 0, the color is a shade of gray
    if ( s == 0)  
        r = g = b = v;

    //If saturation > 0, more complex calculations are needed
    else
    {
        float f, p, q, t;
        int i;
        h *= 6; //to bring hue to a number between 0 and 6, better for the calculations
        i = floor ( h );  //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
        f = h - i;  //the fractional part of h
        p = v * ( 1 - s );
        q = v * ( 1 - ( s * f ) );
        t = v * ( 1 - ( s * ( 1 - f ) ) );
        switch ( i )
        {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }

    return rgb565_conv ( r * 255.0, g * 255.0, b * 255.0);
}

/**
* Set a pixel to a specific colour
* @param {UG_S16} x the x position
* @param {UG_S16} y the y position
* @param {UG_COLOR} the colour of the pixel
*/   
static void pset ( UG_S16 x, UG_S16 y, UG_COLOR color )
{
    fb [ y * LCD_WIDTH + x ] = color;
}

/**
* Send the current frame buffer to the LCD controller, takes about 50ms
*/   
static void UpdateDisplay()
{
    ili9341_write_frame_rectangleLE(0, 0, LCD_WIDTH, LCD_HEIGHT, fb);
}

/**
* Seed the plasma buffer with colour palette indexes
*/   
void seedPlasma()
{
    long start = millis();
    for ( uint16_t y = 0; y < LCD_HEIGHT; y++)
    {
        for ( uint16_t x = 0; x < LCD_WIDTH; x++)
        {
            plasma [ y * LCD_WIDTH + x ] = (
                                128.0 + ( 128.0 * cos ( x / 16.0 ) )
                                - 128.0 + (128.0 * sin ( y / 32.0 ) )
                                - 128.0 + (128.0 * sin(sqrt((x - LCD_WIDTH / 2.0)* (x - LCD_WIDTH / 2.0) + (y - LCD_HEIGHT / 2.0) * (y - LCD_HEIGHT / 2.0)) / 8.0))
                                ) / 3;

        }
    }
    ESP_LOGW ( TAG, "Frame draw time: %lu ms", millis() - start);
}

/**
* Create the palette of colours (defunct)
*/   
// void createPalette()
// {
//     ESP_LOGW(TAG, "createPalette"); 

//     for ( int i = 0; i < PALETTE_SIZE; i++ )
//     {
//         palette[i] = HSVtoRGB ( i, 255, 255 );
//     }
// }

/**
* Draw the plasma
* @param {uint8_t} paletteShift amount to shift the palette by
*/   
void drawPlasma ( uint8_t paletteShift )
{
    // long start = millis();
    for ( uint16_t y = 0; y < LCD_HEIGHT; y++)
    {
        for ( uint16_t x = 0; x < LCD_WIDTH; x++)
        {
            int colorIndex = ( plasma [ y * LCD_WIDTH + x ] + paletteShift ) % 256;
            UG_DrawPixel ( x, y, palette [ colorIndex ] );
        }
    }
    // ESP_LOGW ( TAG, "Frame draw time: %lu ms", millis() - start);
    UpdateDisplay();

}

/**
* Animate the plasma
*/   
void animatePlasma()
{
    // Do this until the cows come home.
    while ( true )
    {
        for (int paletteShift = 0; paletteShift < 256; paletteShift+=2 )
        {
            drawPlasma ( paletteShift );
            // Allow other tasks to do something
            vTaskDelay (1);
        }
    }
}

/**
* Program entry point
*/   
void app_main()
{
    // Initialise the LCD controller and set background to black
    ili9341_init();
    ili9341_clear(0xffff);

    // Create an array, this should end up on the external RAM
    plasma = (uint8_t *) malloc ( LCD_HEIGHT * LCD_WIDTH * sizeof ( uint8_t ) );

    // Initialise the uGUI framework
    UG_Init( &gui, pset, LCD_WIDTH, LCD_HEIGHT);

    // Set foreground colour to white
    UG_SetForecolor ( C_WHITE );

    // Use 12x20 font
    UG_FontSelect( &FONT_12X20 );

    // Display helpful message on screen and update
    UG_PutstringCenter ( LCD_WIDTH / 2, LCD_HEIGHT / 2, 12, 20, "Initialising...");
    UpdateDisplay();

    // createPalette();
    seedPlasma();
    animatePlasma();

    while(1)
    {
        vTaskDelay(1);
    }
}
