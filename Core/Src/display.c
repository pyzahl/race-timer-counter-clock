

/*
 * display.c
 *
 *  Created on: Apr 29, 2020
 *      Author: pzahl
 */

#include "main.h"
#include "display.h"
#include "ws2812.h"

#include "rtc_ds1302.h"

#include "delay_us.h"

//#define LED_RGB_3x595x4DIGITS_SIGN
#define WS2812_STRIPS_7SEGMENTS_DOTS_SIGN

//extern void delay_us (uint16_t us);


// **************************************************
// INFO ONLY -- Race Display as reverse engineered...
// **************************************************
/*
Control Computer:
STM32F103c8  ... 64k (Like Arduino Nano)
Summary:
STM32 pin function to SERIAL LED interface:
595 R (DS)      RED-S-DATA   ---  PB14 --- STM32  (pin 27)
595 G (DS)    GREEN-S-DATA   ---  PC14-OSC32_IN --- STM32 (pin 3)
595 B (DS)     BLUE-S-DATA   ---  PB13 --- STM32 (pin 26)
595 * (ST_CP)  SERIAL-CLK    ---  PB12 --- STM32 (pin 25)
595 * (SH_CP)   Latch-CLK    ---  PB1  --- STM32 (pin 19)

Control Buttons:
Button PB1  -- PA4 (pin14)
Button PB2  -- PA5 (pin15)
Button PB3  -- PA6 (pin16)
Button PB4  -- PA7 (pin17)
Button MD -- PC13 (pin2)
Button S1 -- PA0 (pin11)
Button S2 -- PA1 (pin12)
Middle OnBoard Button -- PB2 (pin20)
OnBoard Button by USB -- BOOT0 (pin44)

LEDs from in segments A-G,P:

FRONT VIEW:

  AAA     AAA     AAA     AAA
 B   F   B   F   B   F   B   F
 B   F   B   F   B   F   B   F
  GGG     GGG     GGG     GGG
 C   E   C   E   C   E   C   E
 C   E   C   E   C   E   C   E
  DDD  P  DDD  P  DDD   P DDD  P

   2020     AAA     AAA     AAA
  40  10   B   F   B   F   B   F
  40  10   B   F   B   F   B   F
   8080     GGG     GGG     GGG
  01  04   C   E   C   E   C   E
  01  04   C   E   C   E   C   E
   0202  P  DDD  P  DDD   P DDD  P

R,G,B Data in parallel, 4 digits in one 4x8bit serial word:
Bit 0 … 7 -> { GBAFPEDC }
G=1, B=2, A=4, F=8, P=0x10, E=0x20, D=0x40, C=0x80
{{0, { AFEDCB }},
{1, { FE }},
{2, { AFGCD }},
{3, { AFGED }}.
{4, {BFGE}},
{5, {ABGED}},
{6, {ABGCED}},
{7, {AFE}},
{8, {ABFGCED}},
{9, {ABFGED}}}
{M, {FGCEP} {BGCE}}
*/


// 7 Segment LED Segment Values:
#define SEGM_C 1
#define SEGM_D 2
#define SEGM_E 4
#define SEGM_P 8
#define SEGM_F 0x10
#define SEGM_A 0x20
#define SEGM_B 0x40
#define SEGM_G 0x80

int Segments[8]      = { SEGM_A, SEGM_B, SEGM_C, SEGM_D, SEGM_E, SEGM_F, SEGM_G, SEGM_P };
int StripSegments[8] = { SEGM_G, SEGM_B, SEGM_A, SEGM_F, SEGM_E, SEGM_D, SEGM_C, SEGM_P };

#define STRIP_DIGI_SEG_N  16 // 16 LEDs per segment
#define STRIP_DIGI_DOT_N   2 // 2 LEDs per dot
#define STRIP_DIGI_DOTS_N (3*STRIP_DIGI_DOT_N) // 3 dots
#define STRIP_DIGI_N      (7*STRIP_DIGI_SEG_N) // 7 Segments

#define STRIP_DIGI1 0
#define STRIP_DOTS1 (STRIP_DIGI1+STRIP_DIGI_N)

#define STRIP_DIGI2 (STRIP_DOTS1+STRIP_DIGI_DOTS_N)
#define STRIP_DOTS2 (STRIP_DIGI2+STRIP_DIGI_N)

#define STRIP_DIGI3 (STRIP_DOTS2+STRIP_DIGI_DOTS_N)
#define STRIP_DOTS3 (STRIP_DIGI3+STRIP_DIGI_N)

#define STRIP_DIGI4 (STRIP_DOTS3+STRIP_DIGI_DOTS_N)
#define STRIP_TOTAL (STRIP_DIGI4+STRIP_DIGI_N)

// Digit Values:
uint8_t DigitValue[27] = {
  SEGM_A+SEGM_F+SEGM_E+SEGM_D+SEGM_C+SEGM_B, // 0
  SEGM_F+SEGM_E,                             // 1
  SEGM_A+SEGM_F+SEGM_G+SEGM_C+SEGM_D,        // 2
  SEGM_A+SEGM_F+SEGM_G+SEGM_E+SEGM_D,        // 3
  SEGM_B+SEGM_F+SEGM_G+SEGM_E,               // 4
  SEGM_A+SEGM_B+SEGM_G+SEGM_E+SEGM_D,        // 5
  SEGM_A+SEGM_B+SEGM_G+SEGM_C+SEGM_E+SEGM_D, // 6
  SEGM_A+SEGM_F+SEGM_E,                      // 7
  SEGM_A+SEGM_B+SEGM_F+SEGM_G+SEGM_C+SEGM_E+SEGM_D, // 8
  SEGM_A+SEGM_B+SEGM_F+SEGM_G+SEGM_E+SEGM_D, // 9
  SEGM_G+SEGM_C+SEGM_E,                      // m  10
  SEGM_A+SEGM_B+SEGM_G+SEGM_C+SEGM_D,        // E  11
  SEGM_A+SEGM_B+SEGM_G+SEGM_C,               // F  12
  SEGM_A+SEGM_B+SEGM_C+SEGM_D,               // C  13
  SEGM_G+SEGM_C+SEGM_D+SEGM_E,               // o  14
  SEGM_A+SEGM_B+SEGM_G+SEGM_F,               // ^o 15
  SEGM_G+SEGM_C+SEGM_E,                      // n  16
  SEGM_G+SEGM_C+SEGM_E+SEGM_F,               // d  17
  SEGM_D+SEGM_C+SEGM_E,                      // u  18
  SEGM_B+SEGM_G+SEGM_C+SEGM_D,               // t  19
  SEGM_C,                                    // i  20
  SEGM_C+SEGM_G,                             // r  21
  SEGM_B+SEGM_C+SEGM_D,                      // l  22
  SEGM_B+SEGM_C+SEGM_G,                      // k  23
  SEGM_G,                                    // -  24
  SEGM_D,                                    // _  25
  0 // OFF                                   // " "26
};

extern uint8_t LED_power;

#define DisplayLEDPower LED_power  // 0..255

//#define COLOR2HUE(C) (85*(C-1))
#define COLOR2HUE(C) (C)

// ****************************************************************
// Display Interface Control (Shift Register SM74HC595D 3x RBG x 4)
// ****************************************************************

#ifdef WS2812_STRIPS_7SEGMENTS_DOTS_SIGN
void display_stripled_segments (int start, int digit_value, uint8_t color){
  uint8_t hue = COLOR2HUE(color);
  uint8_t sat = 255;
  uint8_t val = 0;
  for (int i=0; i<7; i++){
	  val = (digit_value & StripSegments[i]) && color ? DisplayLEDPower:0;
	  int startled=start+(i<<4);
	  fillLEDcolorHSV (startled, startled+16-1, hue, color==255?0:sat, val);
  }
  //fillLEDcolor (startled, startled+16-1, (color&1)?val:0, (color&2)?val:0,(color&3)?val:0);
  //ws2812_update();
  //setLEDcolorHSV (uint32_t LEDnumber, uint8_t h, uint8_t s, uint8_t v);
  //fillLEDcolorHSV (uint32_t start, uint32_t end, uint8_t h, uint8_t s, uint8_t v);
}

void display_stripled_segments_color (int start, int digit_value, uint8_t color[8]){
	uint8_t sat = 255;
	uint8_t val = 0;
	for (int i=0; i<7; i++){
		uint8_t hue = COLOR2HUE(color[i]);
		val = (digit_value & StripSegments[i]) && color[i] ? DisplayLEDPower:0;
		int startled=start+(i<<4);
		//if (val > 0)
			fillLEDcolorHSV (startled, startled+16-1, hue, sat, val);
	}
	//ws2812_update();
	//setLEDcolorHSV (uint32_t LEDnumber, uint8_t h, uint8_t s, uint8_t v);
	//fillLEDcolorHSV (uint32_t start, uint32_t end, uint8_t h, uint8_t s, uint8_t v);
}

void display_stripled_dots (int start, uint8_t dots[6], uint8_t color[6]){
  uint8_t sat = 255;
  for (int i=0; i<6; i++)
	  setLEDcolorHSV (start+i, COLOR2HUE(color[i]), sat, dots[i]); // hue, sat, val
  //ws2812_update();
}

void display_stripled_dot_colon (int start,  uint8_t dot, uint8_t colon, uint8_t dcolor, uint8_t color){
  uint8_t cols[6] = { dcolor, dcolor, color, color, color, color };
  uint8_t dots[6]  = { dot, dot, colon, colon,  colon, colon };
  display_stripled_dots (start, dots, cols);
}
#endif

#define LEDON  HIGH
#define LEDOFF LOW
#define CLK_T   10  // Clock 1/2 period us
#define SETUP_T 2   // Data Setup Time us

int start_sequence=0;

#ifdef LED_RGB_3x595x4DIGITS_SIGN
void Latch(){
	  latchPin (HIGH);
	  delay_us (SETUP_T);
	  latchPin (LOW);
}
#endif

void StartDisplayUpdate(void){
	start_sequence=0;
#ifdef LED_RGB_3x595x4DIGITS_SIGN
	  // take the latchPin low so
	  // the LEDs don't change while you're sending in bits:
	  latchPin (LOW);
	  clockPin (LOW);
	  delay_us (SETUP_T);
#endif
}

void FinishDisplayUpdate(void){
#ifdef LED_RGB_3x595x4DIGITS_SIGN
	  //take the latch pin high so the LEDs will light up:
	  Latch ();
	  RedDataPin (LOW);
	  GreenDataPin (LOW);
	  BlueDataPin (LOW);
	  delay_us (SETUP_T);
#endif
}

void ShiftOut(uint8_t color, uint8_t digit){
#ifdef LED_RGB_3x595x4DIGITS_SIGN
	uint8_t b = digit;
	for (int i=0; i<8; ++i){
		RedDataPin   (((b&1) && (color&1)) ? LEDON:LEDOFF);
		GreenDataPin (((b&1) && (color&2)) ? LEDON:LEDOFF);
		BlueDataPin  (((b&1) && (color&4)) ? LEDON:LEDOFF);
		b >>= 1;
		delay_us (SETUP_T);
		clockPin (HIGH);
		delay_us (CLK_T);
		clockPin (LOW);
		delay_us (CLK_T);
	}
#endif
#ifdef WS2812_STRIPS_7SEGMENTS_DOTS_SIGN
	start_sequence++;
	switch (start_sequence){
	case 1: display_stripled_segments (STRIP_DIGI4, digit, color); break;
	case 2: display_stripled_segments (STRIP_DIGI3, digit, color); break;
	case 3: display_stripled_segments (STRIP_DIGI2, digit, color); break;
	case 4: display_stripled_segments (STRIP_DIGI1, digit, color); break;
	}
#endif
}

void ShiftOutIndividualColor(uint8_t color[8], uint8_t digit){
#ifdef LED_RGB_3x595x4DIGITS_SIGN
	int b = digit;
	for (int i=0; i<8; ++i){
		RedDataPin   (((b&1) && (color[i]&1)) ? LEDON:LEDOFF);
		GreenDataPin (((b&1) && (color[i]&2)) ? LEDON:LEDOFF);
		BlueDataPin  (((b&1) && (color[i]&4)) ? LEDON:LEDOFF);
		b >>= 1;
		delay_us (SETUP_T);
		clockPin (HIGH);
		delay_us (CLK_T);
		clockPin (LOW);
		delay_us (CLK_T);
	}
#endif
#ifdef WS2812_STRIPS_7SEGMENTS_DOTS_SIGN
	start_sequence++;
	switch (start_sequence){
	case 1: display_stripled_segments_color (STRIP_DIGI4, digit, color); break;
	case 2: display_stripled_segments_color (STRIP_DIGI3, digit, color); break;
	case 3: display_stripled_segments_color (STRIP_DIGI2, digit, color); break;
	case 4: display_stripled_segments_color (STRIP_DIGI1, digit, color); break;
	}
#endif
}

// ******************************
// DISPLAY SUPPORT FUNCTIONS
// ******************************

void ClearDisplay (){
	StartDisplayUpdate();
	ShiftOut(0, 0); // Digit 4
	ShiftOut(0, 0); // Digit 2
	ShiftOut(0, 0); // Digit 3
	ShiftOut(0, 0); // Digit 4
	FinishDisplayUpdate();
}

void SetSegments (uint8_t color[4], uint8_t segm[4]){
	uint8_t cc[8];
	StartDisplayUpdate();
    for (int m=0; m<4; ++m){
    	for (int k=0; k<8; ++k) cc[k]=color[m];
    	ShiftOutIndividualColor(cc, segm[m]);
    }
	FinishDisplayUpdate();
}

void RunSegmentTest (){
	uint8_t cc[8];
	ClearDisplay ();
	for (int i=0; i<16; ++i){
	  StartDisplayUpdate();
	  for (int k=0; k<8; ++k) cc[k]=k+1+i;
	  ShiftOutIndividualColor(cc, DigitValue[8]);
	  ShiftOutIndividualColor(cc, DigitValue[8]);
	  ShiftOutIndividualColor(cc, DigitValue[8]);
	  ShiftOutIndividualColor(cc, DigitValue[8]);
	  FinishDisplayUpdate();
	  HAL_Delay (200);
	}
	ClearDisplay ();
	HAL_Delay (500);
}

void RunDisplayTest (){
	ClearDisplay ();
	for (int i=0; i<10; ++i){
	  StartDisplayUpdate();
	  ShiftOut(1+(i)%7, DigitValue[i%10]); // Digit 4
	  ShiftOut(1+(1+i)%7, DigitValue[(i+1)%10]); // Digit 2
	  ShiftOut(1+(2+i)%7, DigitValue[(i+2)%10]); // Digit 3
	  ShiftOut(1+(3+i)%7, DigitValue[(i+3)%10]); // Digit 4
	  FinishDisplayUpdate();
	  HAL_Delay (400);
	}
	ClearDisplay ();
	HAL_Delay (600);
}


// display one 4 digit number in color with dots options
void display_number(int numberToDisplay, int color, int dots){
  int digit1=10; // [10] -> 0 -> digit OF+SEGM_G+SEGM_EF
  int digit2=10;
  int digit3=10;
  int digit4=10;

  StartDisplayUpdate();

  if (numberToDisplay>=0){
    digit4 =  numberToDisplay%10;
    digit3 = (numberToDisplay/10)%10;
    digit2 = (numberToDisplay/100)%10;
    digit1 = (numberToDisplay/1000)%10;
  } else {
	    digit4 =  -numberToDisplay%10;
	    digit3 = (-numberToDisplay/10)%10;
	    digit2 = (-numberToDisplay/100)%10;
	    digit1 = DIGIT_Minus;
  }
  // shift out the bits for all digit with color, with dot options:
  ShiftOut(color, DigitValue[digit1]); // Digit 1
  ShiftOut(color, DigitValue[digit2]); // Digit 2
  ShiftOut(color, DigitValue[digit3]); // Digit 3
  ShiftOut(color, DigitValue[digit4]); // Digit 4

  FinishDisplayUpdate();


#ifdef SERIAL_MONITOR
  if(digit1==10) Serial.print(" "); else Serial.print(digit1);
  if(digit2==10) Serial.print(" "); else Serial.print(digit2);
  if(digit3==10) Serial.print(" "); else Serial.print(digit3);
  if(digit4==10) Serial.print(" "); else Serial.print(digit4);
  Serial.print("  D=");
  Serial.print(dots);
  Serial.print("  Col =");
  Serial.print(color);
  Serial.println();
#endif
}

// display two 2 digit numbers in dual color with dots options
void display_two_numbers_leadzero(int numberToDisplayLeft, int numberToDisplayRight, int colorLeft, int colorRight, int dots){
  int digit1=DIGIT_OFF; // [10] -> 0 -> digit OFF
  int digit2=DIGIT_OFF;
  int digit3=DIGIT_OFF;
  int digit4=DIGIT_OFF;

  StartDisplayUpdate();

  if (numberToDisplayRight>=0){
    digit4 =  numberToDisplayRight%10;
    digit3 = (numberToDisplayRight/10)%10;
  }
  if (numberToDisplayLeft>=0){
    digit2 = (numberToDisplayLeft/1)%10;
    digit1 = (numberToDisplayLeft/10)%10;
    if (dots < 0 && digit1 == 0){
    	digit1=digit2;
    	if (dots&1)
   			digit2 = DIGIT_Minus;
    	else
			digit2 = DIGIT_OFF;
    }
  }

  // shift out the bits for left and right digit pairs in colors, with dot options:
  ShiftOut(colorLeft, DigitValue[digit1]); // Digit 1
  ShiftOut(colorLeft, DigitValue[digit2]); // Digit 2
  ShiftOut(colorRight, DigitValue[digit3]); // Digit 3
  ShiftOut(colorRight, DigitValue[digit4]); // Digit 4

  FinishDisplayUpdate();

#ifdef SERIAL_MONITOR
  if(digit1==10) Serial.print(" "); else Serial.print(digit1);
  if(digit2==10) Serial.print(" "); else Serial.print(digit2);
  Serial.print(":");
  if(digit3==10) Serial.print(" "); else Serial.print(digit3);
  if(digit4==10) Serial.print(" "); else Serial.print(digit4);
  Serial.print("  D=");
  Serial.print(dots);
  Serial.print("  Col LR=");
  Serial.print(colorLeft);
  Serial.print(colorRight);
  Serial.println();
#endif

}

// display two 2 digit numbers in dual color with dots options
void display_two_numbers(int numberToDisplayLeft, int numberToDisplayRight, int colorLeft, int colorRight, int dots){
  int digit1=DIGIT_OFF; // [10] -> 0 -> digit OFF
  int digit2=DIGIT_OFF;
  int digit3=DIGIT_OFF;
  int digit4=DIGIT_OFF;

  StartDisplayUpdate();

  if (numberToDisplayRight>=0){
    digit4 =  numberToDisplayRight%10;
    digit3 = (numberToDisplayRight/10)%10;
    if (digit3 == 0)
    		digit3 = DIGIT_OFF;
  }
  if (numberToDisplayLeft>=0){
    digit2 = (numberToDisplayLeft/1)%10;
    digit1 = (numberToDisplayLeft/10)%10;
    if (digit1 == 0)
    		digit1 = DIGIT_OFF;
  }

  // shift out the bits for left and right digit pairs in colors, with dot options:
  ShiftOut(colorLeft, DigitValue[digit1]); // Digit 1
  ShiftOut(colorLeft, DigitValue[digit2]); // Digit 2
  ShiftOut(colorRight, DigitValue[digit3]); // Digit 3
  ShiftOut(colorRight, DigitValue[digit4]); // Digit 4

  FinishDisplayUpdate();

#ifdef SERIAL_MONITOR
  if(digit1==10) Serial.print(" "); else Serial.print(digit1);
  if(digit2==10) Serial.print(" "); else Serial.print(digit2);
  Serial.print(":");
  if(digit3==10) Serial.print(" "); else Serial.print(digit3);
  if(digit4==10) Serial.print(" "); else Serial.print(digit4);
  Serial.print("  D=");
  Serial.print(dots);
  Serial.print("  Col LR=");
  Serial.print(colorLeft);
  Serial.print(colorRight);
  Serial.println();
#endif

}

void display_colon(int color){
#ifdef WS2812_STRIPS_7SEGMENTS_DOTS_SIGN
	uint8_t hue = COLOR2HUE(color);
	uint8_t sat = 255;
	uint8_t val = color ? DisplayLEDPower:0;
	for (int k=2; k<6; ++k)
		setLEDcolorHSV (2*7*16+6+k, hue, sat, val);
#endif
}

void display_dot(int pos, int lv, int color){
#ifdef WS2812_STRIPS_7SEGMENTS_DOTS_SIGN
	uint8_t hue = COLOR2HUE(color);
	uint8_t sat = 255;
	uint8_t val = color ? DisplayLEDPower:0;
	for (int k=2*lv; k<2*lv+2; ++k)
		setLEDcolorHSV ((pos-1)*(7*16+6)+k+7*16, hue, sat, val);
#endif
}

void display_TempC(int Tc10, int color){
	  int digit1=DIGIT_OFF; // [10] -> 0 -> digit OFF
	  int digit2=DIGIT_OFF;
	  int digit3=DIGIT_OFF;

	  StartDisplayUpdate();

	  if (Tc10 < 0){
		  digit1 = DIGIT_Minus;
	  }
	  digit3 =  Tc10%10;
	  digit2 = (Tc10/10)%10;
	  digit1 = (Tc10/100)%10;

	  // shift out the bits for left and right digit pairs in colors, with dot options:
	  if (Tc10 < 0){
		  ShiftOut(color, DigitValue[DIGIT_Minus]); // Digit 4
		  ShiftOut(color, DigitValue[digit1]); // Digit 1
		  ShiftOut(color, DigitValue[digit2]); // Digit 2
		  ShiftOut(color, DigitValue[digit3]); // Digit 3
		  FinishDisplayUpdate();
		  display_dot(1, 0, color);
		  display_dot(1, 2, 0);
		  display_dot(2, 0, 0);
	  }else{
		  ShiftOut(color, DigitValue[digit1]); // Digit 2
		  ShiftOut(color, DigitValue[digit2]); // Digit 3
		  ShiftOut(color, DigitValue[digit3]); // Digit 4
		  ShiftOut(color, DigitValue[DIGIT_C]); // Digit 1
		  FinishDisplayUpdate();
		  display_dot(1, 0, 0);
		  display_dot(1, 2, color);
		  display_dot(2, 0, color);
	  }
	  display_dot(2, 1, 0);
	  display_dot(2, 2, 0);
}

void display_RH(int Rh10, int color){
	  int digit1=DIGIT_OFF; // [10] -> 0 -> digit OFF
	  int digit2=DIGIT_OFF;
	  int digit3=DIGIT_OFF;
	  int digit4=DIGIT_OFF;

	  StartDisplayUpdate();

	  digit4=  Rh10%10;
	  digit3 = (Rh10/10)%10;
	  digit2 = (Rh10/100)%10;

	  // shift out the bits for left and right digit pairs in colors, with dot options:
	  ShiftOut(color, DigitValue[digit1]); // Digit 1
	  ShiftOut(color, DigitValue[digit2]); // Digit 2
	  ShiftOut(color, DigitValue[digit3]); // Digit 3
	  ShiftOut(color, DigitValue[digit4]); // Digit 4

	  FinishDisplayUpdate();
	  display_dot(1, 0, color);
	  display_dot(1, 2, 0);
	  display_dot(2, 0, 0);
	  display_dot(2, 1, 0);
	  display_dot(2, 2, 0);
}

// display Mode
void display_mode(int mode, int color[8]){
  int c = color[mode];
  int digit3 = (mode/10)%10;
  int digit4 = (mode/1)%10;
/*
	case 1:	RunModeLapCntr (); break; // LAPS
	case 2: RunModeTime (0, 1); break; // TT
	case 3: RunModeTime (1, -1); break; // Crit Blue
	case 4: RunModeTime (2, -1); break; // Crit Green
	case 5: RunModeTime (3, 1); break;  // Tmr
	case 6: RunRTCClock (); break; // RTClockk
	case 7: StandByMode (); break;
*/
  StartDisplayUpdate();
  switch (mode){
  	  case 1:
		  ShiftOut(c, DigitValue[DIGIT_C]);
		  ShiftOut(c, DigitValue[DIGIT_n]);
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  ShiftOut(c, DigitValue[DIGIT_r]);
		  break;
  	  case 2:
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  ShiftOut(0, 0); // Digit 3
		  ShiftOut(0, 0); // Digit 4
		  break;
  	  case 3:
		  ShiftOut(c, DigitValue[DIGIT_C]);
		  ShiftOut(c, DigitValue[DIGIT_r]);
		  ShiftOut(c, DigitValue[DIGIT_i]);
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  break;
  	  case 4:
		  ShiftOut(c, DigitValue[DIGIT_C]);
		  ShiftOut(c, DigitValue[DIGIT_r]);
		  ShiftOut(c, DigitValue[DIGIT_i]);
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  break;
  	  case 5:
		  ShiftOut(c, DigitValue[DIGIT_t]);
		  ShiftOut(c, DigitValue[DIGIT_i]);
		  ShiftOut(c, DigitValue[DIGIT_m]);
		  ShiftOut(c, DigitValue[DIGIT_r]);
		  break;
  	  case 6:
		  ShiftOut(c, DigitValue[DIGIT_C]);
		  ShiftOut(c, DigitValue[DIGIT_l]);
		  ShiftOut(c, DigitValue[DIGIT_k]);
		  ShiftOut(c, 0);
		  break;
  	  default:
		  ShiftOut(c, DigitValue[DIGIT_m]);
		  ShiftOut(c, DigitValue[DIGIT_m]);
		  ShiftOut(c, DigitValue[digit3]); // Digit 3
		  ShiftOut(c, DigitValue[digit4]); // Digit 4
		  break;
}
  FinishDisplayUpdate();

#ifdef SERIAL_MONITOR
  Serial.print("MM ");
  Serial.print(digit3);
  Serial.print(digit4);
  Serial.println();
#endif
}

// display Error
void display_error(int errcode, int c1, int c2){
	  int digit3 = (errcode/10)%10;
	  int digit4 = (errcode/1)%10;

	  StartDisplayUpdate();
	  ShiftOut(c1, DigitValue[DIGIT_E]);
	  ShiftOut(c1, DigitValue[DIGIT_E]);
	  ShiftOut(c2, DigitValue[digit3]); // Digit 3
	  ShiftOut(c2, DigitValue[digit4]); // Digit 4
	  FinishDisplayUpdate();

	#ifdef SERIAL_MONITOR
	  Serial.print("EE ");
	  Serial.print(digit3);
	  Serial.print(digit4);
	  Serial.println();
	#endif
}




