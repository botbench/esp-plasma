/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Copyright (c) Botbench 2019, All rights reserved.                       */
/*                                                                            */
/*    Module:     genpalette.c                                                */
/*    Author:     Xander Soldaat, based on code found here:                   */
/*                lodev.org and iotexpert.com                                 */
/*    Created:    August 2019                                                 */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <math.h>

/**
* RGB565 conversion
* RGB565 is R(5)+G(6)+B(5)=16bit color format.
* Bit image "RRRRRGGGGGGBBBBB"
* @param {uint16_t} red the red component
* @param {uint16_t} green the green component
* @param {uint16_t} blue the blue component
* @return {uint16_t} RGB565 value
*/   
uint16_t rgb565_conv ( uint16_t r,uint16_t g,uint16_t b ) {
	return ( ( ( r & 0xF8 ) << 8 ) | ( ( g & 0xFC ) << 3 ) | ( b >> 3 ) );
}

/**
* HSV to RGB565 conversion
* @param {uint8_t} hue the hue component
* @param {uint8_t} sat the saturation component
* @param {uint8_t} val the value component
* @return {uint16_t} RGB565 value
*/
uint16_t HSVtoRGB(uint8_t hue, uint8_t sat, uint8_t val)
{
    float r, g, b, h, s, v; //this function works with floats between 0 and 1
    h = hue / 256.0;
    s = sat / 256.0;
    v = val / 256.0;
    r = 0.0;
    g = 0.0;
    b = 0.0;

    //If saturation is 0, the color is a shade of gray
    if(s == 0) r = g = b = v;

    //If saturation > 0, more complex calculations are needed
    else
    {
        float f, p, q, t;
        int i;
        h *= 6; //to bring hue to a number between 0 and 6, better for the calculations
        i = floor ( h );  //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
        f = h - i;  //the fractional part of h
        p = v * (1 - s);
        q = v * (1 - (s * f));
        t = v * (1 - (s * (1 - f)));
        switch(i)
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

int main()
{
    for ( int i = 0; i < 256; i++ )
    {
        if ( i % 8 == 7)
            printf ( "%d,\n", HSVtoRGB ( i , 255, 255 ) );
        else
            printf ( "%d,\t", HSVtoRGB ( i , 255, 255 ) );
    }
    return 0;
}
