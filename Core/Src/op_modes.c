/*
 * op_modes.c
 *
 *  Created on: Apr 29, 2020
 *      Author: pzahl
 */

#include "main.h"
#include "op_modes.h"
#include "display.h"
#include "rtc_ds1302.h"
#include "ws2812.h"
#include "delay_us.h"

extern ADC_HandleTypeDef hadc1;


// #define STORE_MODE_IN_RTC 0    // at index  -- THIS is not working right yet
// #define STORE_LAPS_IN_RTC 1    // starting at index  -- THIS is not working right yet
// #define STORE_TIMERS_IN_RTC 4  // starting at index  -- THIS is not working right yet

// Mode 1: Lap Counters. Manual. # Fields
#define NFields 6

// Modes 2..4 are Timers
// Timer 0: Down Timer  (Mode 2)
// Timer 1: Up Timer A  (Mode 3)
// Timer 2: Up Timer B  (Mode 4)
#define NUM_TIMERS 4 // MUST match with data init sets!

// COLOR SCHEMES
// LED Colors are defined via Bits 0,1,2 = Red,Green,Blue

#ifdef pureRGB // RBG LED SIGN

int ColorLookup[8] = { 1, 2, 4, 3, 5, 7, 6, 0 }; // RED, GREEN, BLUE, YELLOW, VIOLET, CYAN, WHITE, BLACK
// Timer Color Scheme
int TimerColorRun[NUM_TIMERS]     = { 1, 4, 2, 2 }; // RED, GREEN, BLUE
int TimerColorSetup[NUM_TIMERS]   = { 4, 4, 2, 5 }; // YELLOW
int TimerColorStop[NUM_TIMERS]    = { 5, 5, 5, 4 }; // VIOLET
int TimerColorElapsed[NUM_TIMERS] = { 4, 4, 4, 4 }; // BLUE
int TimerColorSeconds[NUM_TIMERS] = { 1, 4, 2, 4 }; // = RunColors
int TimerMinutesPerHour[NUM_TIMERS] = { 100, 100, 100, 60 };
int TTTimerColorPre[3] = { 3, 2, 1 };   //

int mode_colors[8] = { 7, 1, 2, 4, 2, 1, 1, 1 };

#else // RBG full HUW and VALUE WS2812, HUE values

#define HUE_RED     1
#define HUE_GREEN   85
#define HUE_BLUE    171
#define HUE_YELLOW  42
#define HUE_VIOLETT 213
#define HUE_CYAN    128
#define HUE_WHITE   255 // **
#define HUE_BLACK   0 // **

int ColorLookup[8] = { HUE_RED, HUE_GREEN, HUE_BLUE, HUE_YELLOW, HUE_VIOLETT, HUE_CYAN, HUE_WHITE, HUE_BLACK }; // RED, GREEN, BLUE, YELLOW, VIOLET, CYAN, CYANOGREEN, ORANGE -- 256 scale HUE coding
// Timer Color Scheme
int TimerColorRun[NUM_TIMERS]     = { HUE_RED, HUE_BLUE, HUE_GREEN, HUE_GREEN }; // RED, GREEN, BLUE
int TimerColorSetup[NUM_TIMERS]   = { HUE_BLUE, HUE_BLUE, HUE_GREEN, HUE_VIOLETT }; // YELLOW
int TimerColorStop[NUM_TIMERS]    = { HUE_VIOLETT, HUE_VIOLETT, HUE_VIOLETT, HUE_BLUE }; // VIOLET
int TimerColorElapsed[NUM_TIMERS] = { HUE_BLUE, HUE_BLUE, HUE_BLUE, HUE_BLUE }; // BLUE
int TimerColorSeconds[NUM_TIMERS] = { HUE_RED, HUE_BLUE, HUE_GREEN, HUE_BLUE }; // = RunColors
int TimerMinutesPerHour[NUM_TIMERS] = { 100, 100, 100, 60 };
int TTTimerColorPre[3] = { HUE_YELLOW, HUE_GREEN, HUE_RED };   //

int mode_colors[8] = { 7, 1, 2, 4, 2, 1, 1, 1 };
#endif

uint8_t LED_power=64;

// default minutes max per hour
//#define TIMER_MIN_MAX 59
#define TIMER_MIN_MAX 99

// ******************************
// OPERATION MODE TASKS
// non blocking state machine design
// ******************************

int last_op=-1;

void RunModeLapCntr (void){
#ifdef STORE_LAPS_IN_RTC
	static int init=1;
#endif
	static int LapCnt[NFields] = { 10, 15, 20, -1, -1, -1 }; // default laps
	static int LapSel=0;

	static uint32_t td=100L;
	static uint32_t timeLastPress=0;
	static int lpress=0;
	static int state=1;
	static int uptodate=0;

	uint32_t now = HAL_GetTick();

#ifdef STORE_LAPS_IN_RTC
	if (init){
		for (int i=0; i<3; ++i)
			LapCnt[i] = rtc_read_reg (kRamAddress0+STORE_LAPS_IN_RTC+i);
		init = 0;
	}
#endif

	switch (state){
		case 0: // check controls
			// SHR
			if (ButtonS1Pin == BUTTON_PRESSED)
			  LapSel++, state=1;

			// SHL
			if (ButtonS2Pin == BUTTON_PRESSED)
			  LapSel--, state=1;

			LapSel += NFields;
			LapSel %= NFields; // cyclic

			// LapL++
			if (Button3Pin == BUTTON_PRESSED)
			  if (LapCnt[LapSel%NFields] < 99) // limit
				LapCnt[LapSel%NFields]++, state=2;

			// LapL--
			if (Button4Pin == BUTTON_PRESSED)
			  if (LapCnt[LapSel%NFields] >= 0) // limit
				LapCnt[LapSel%NFields]--, state=2;


			// LapR++
			if (Button1Pin == BUTTON_PRESSED)
			  if (LapCnt[(LapSel+1)%NFields] < 99)
				LapCnt[(LapSel+1)%NFields]++, state=2;

			// LapR--
			if (Button2Pin == BUTTON_PRESSED)
			  if (LapCnt[(LapSel+1)%NFields] >= 0)
				LapCnt[(LapSel+1)%NFields]--, state=2;

			if (state > 0){
				timeLastPress = now;
				uptodate = 0;
			} else
				lpress = 0;
			break;

		case 1: // delay until next check, repeat
			if (now - timeLastPress < 600) return;
			state = 0;
			break;

		case 2: // check for long press
			if (lpress)
			  td=100; // speedup on long press
			else
			  td=700; // 1st press

			if (now - timeLastPress < td) return;

			lpress= 1;
			state = 0;

#ifdef STORE_LAPS_IN_RTC
		for (int i=0; i<3; ++i)
				rtc_write_reg (kRamAddress0+STORE_LAPS_IN_RTC+i, (uint8_t )LapCnt[i]);
#endif

			break;

		default: state = 0; break;
	}

	if (state > 0){
		timeLastPress = now;
	}

	if (!uptodate || last_op != 1){
		display_two_numbers (LapCnt[LapSel%NFields], LapCnt[(LapSel+1)%NFields], ColorLookup[LapSel%NFields], ColorLookup[(LapSel+1)%NFields], 0);
		ws2812_update ();
		uptodate = 1;
		last_op=1;
	}
}

void RunModeTime (int index, int timer_dir){
   // Timer Modes 2,3,4 DATA
#ifdef STORE_TIMERS_IN_RTC
   // not enough storage to put all static vra in there!
#endif
  static int HHMM[NUM_TIMERS][2] = {{ 00,00 }, { 40,00 }, { 60,00 }, { 00,00 }};
  static uint32_t timeRef[NUM_TIMERS] = {0, 0, 0, 0}; // ms
  static int ssrstate[NUM_TIMERS] = {3,3,3,3};
  static int ssrstateP[NUM_TIMERS]= {0,0,3,0};
  static int go[NUM_TIMERS] = {0,0,0,0};
  static int timeElapsed[NUM_TIMERS]   = {0, 0, 0, 0}; // ms
  static int RTCtimeRef[NUM_TIMERS]    = {0, 0, 0, 0}; // sec
  static int RTCtimeElapsed[NUM_TIMERS]= {0, 0, 0, 0}; // sec
  static int RTCtimeRefms[NUM_TIMERS]  = {0, 0, 0, 0}; // ms offset
  static int state=0;
  int min_per_hour = TimerMinutesPerHour[index];

  // short term button timings
  static uint32_t td=100;
  static uint32_t timeLast=0;
  static uint32_t timeLastPress=0;
  static int lpress=0;
  static int indexLast=-1;

  static int uptodate=0;
  const int crun = TimerColorRun[index];
  const int cset = TimerColorSetup[index];
  const int cstop = TimerColorStop[index];
  const int celaps = TimerColorElapsed[index];
  const int csec = TimerColorSeconds[index];
  static int critsec_last=0;
  int set_hh;
  int set_mm;
  int set_ss=0;
  int hh, mm, ss;

  uint32_t now = HAL_GetTick();

  switch (state){
    case 0: // check controls
		// Start/Stop/Reset...
		if (ButtonS2Pin == BUTTON_PRESSED){
			if (ssrstate[index]==3){
				ssrstate[index]=1;
				ssrstateP[index]=0;
				state=1;
			}
		}
		if (ButtonS1Pin == BUTTON_PRESSED){
			ssrstate[index]++;
			state=1;
#if 0
			if (min_per_hour > 60)
				min_per_hour = 60;
			else
				min_per_hour = TIMER_MIN_MAX+1;
#endif
			state=1;
		}

		// HH++
		if (Button3Pin == BUTTON_PRESSED)
			if (HHMM[index][0] < 99){ // limit
				HHMM[index][0]++; state=2;
			}
		// HH--
		if (Button4Pin == BUTTON_PRESSED)
			if (HHMM[index][0] > 0){ // limit
				HHMM[index][0]--; state=2;
			}

		// MM++
		if (Button1Pin == BUTTON_PRESSED)
			if (HHMM[index][1] < (min_per_hour-1)){ // limit to 59 (normal HH:MM) or 99 (special) or what ever...
				HHMM[index][1]++; state=2;
			}
		// MM--
		if (Button2Pin == BUTTON_PRESSED)
			if (HHMM[index][1] > 0){ // limit
				HHMM[index][1]--; state=2;
			}
		if (state > 0){
			timeLastPress = now;
			uptodate = 0;
		} else
			lpress = 0;
		break;

	case 1: // delay until next check, repeat
		if (now - timeLastPress < 1500) return;
		state = 0;
		break;

	case 2: // check for long press
		if (lpress)
		  td=300; // speedup on long press
		else
		  td=1000; // 1st press

		if (now - timeLastPress < td) return;

		lpress= 1;
        state = 0;
        break;

    default: state = 0; break;
  }

  // Start/Stop/Reset state control
  if (ssrstate[index] != ssrstateP[index]){
	  if (ssrstate[index] > 3) ssrstate[index] = 1;
	  switch (ssrstate[index]){
		  case 1: // Start
			  timeRef[index] = now; // system tics
			  RTCtimeRef[index]   = rtc_get_seconds_of_day (); // seconds of day from external RTC DS1302
			  RTCtimeRefms[index] = now; // ms offset
			  go[index] = 1;
			  break;
		  case 2: // Stop
			  go[index] = 0;
			  break;
		  case 3: // Reset
			  go[index] = 0; timeElapsed[index] = 0; RTCtimeElapsed[index] = 0;
			  break;
		  default:
			  ssrstate[index] = 1;
			  break;
	  }
	  ssrstateP[index] = ssrstate[index];
  }

  if (state > 0)
    timeLastPress = now;

  if (now - timeLast < 200) return; // display refresh

  if (timeRef[index] == 0L)
    timeRef[index] = now;

  if (timer_dir > 0){ // Timer
	  set_hh = HHMM[index][0];
	  set_mm = HHMM[index][1];
  } else { // Down Counter
	  set_hh = 0;
	  set_mm = HHMM[index][0];
	  set_ss = HHMM[index][1];
  }

  if (go[index]){
    //timeElapsed[index] = now-timeRef[index];
    RTCtimeElapsed[index] = 1000*(rtc_get_seconds_of_day () - RTCtimeRef[index]); // seconds of day from external RTC DS1302
    // Auto Sub Second compensation RTC + Ticks
    if (RTCtimeRefms[index] > 1000){
    	if (RTCtimeElapsed[index] == 1){
    		RTCtimeRefms[index] = 1000 - (RTCtimeRefms[index] - now);
    	}
        timeElapsed[index] = now-timeRef[index];
    } else
        timeElapsed[index] = RTCtimeElapsed[index]-RTCtimeRefms[index];
  }

  int critsec = set_hh*(60*(min_per_hour)) + set_mm*60 + set_ss + timer_dir*timeElapsed[index]/1000; // use system tics

  hh = critsec/(60*(min_per_hour));
  mm = critsec/60-hh*(min_per_hour);
  ss = critsec-hh*(60*(min_per_hour))-mm*60;

  if (!uptodate || critsec_last!=critsec || indexLast!=index || last_op != index+2){
    if (critsec < 0){ // timer elapsed
        hh = -critsec/(60*(min_per_hour));
        mm = -critsec/60-hh*(min_per_hour);
        ss = -critsec-hh*(60*(min_per_hour))-mm*60;
        if (ss%2 == 0)
          if (hh > 0)
            display_two_numbers_leadzero (hh,mm, celaps, celaps, 0); // time elapsed (negative), show MM:SS red alternating on/off
          else
            display_two_numbers_leadzero (mm, ss, celaps, csec, 0); // time elapsed (negative), show MM:SS red alternating on/off
        else
          display_two_numbers_leadzero (-1, -1, 0, 0, 0); // OFF at even sec (BLINK)
    }else{
      switch (ssrstate[index]){
      case 1: // running (started)
		  if (hh > 0) // HH:MM
        	  if (index == 3){
        		  display_two_numbers_leadzero (hh, mm, crun, crun, -ss); // HH:MM : green
        	  } else
        		  display_two_numbers_leadzero (hh, mm, crun, crun, 0); // HH:MM : green
          else{ // MM:SS
        	  if (index == 0){
        		  switch (ss){
        		  	  case 27: case 57: chime(3); break;
        		  	  case 29: case 59: display_two_numbers_leadzero (mm, ss, TTTimerColorPre[0], TTTimerColorPre[0], 0); break; // h=00, show seconds in blue
        		  	  case 30: case  0: display_two_numbers_leadzero (mm, ss, TTTimerColorPre[1], TTTimerColorPre[1], 0); break; // h=00, show seconds in blue
        		  	  default: display_two_numbers_leadzero (mm, ss, crun, csec, 0); break; // h=00, show seconds in blue
        		  }
        	  } else {
        		  display_two_numbers_leadzero (mm, ss, crun, csec, 0); // h=00, show seconds in blue
        	  }
          }
    	  break;
      case 2: // stopped
    	  if (hh > 0) // HH:MM
    		  display_two_numbers_leadzero (hh, mm, cstop, cstop, 0); // STOPPED state : blue
    	  else // MM:SS
    	  	  display_two_numbers_leadzero (mm, ss, cstop, csec, 0); // STOPPED state : blue
    	  break;
      case 3: // reset, hold mode, adjust time mode
    	  // always HH:MM for adjusting
    	  if (timer_dir > 0) // Timer
    		  display_two_numbers_leadzero (set_hh, set_mm, cset, cset, 0); // STOPPED SETUP state : blue
		  else
    		  display_two_numbers_leadzero (set_mm, set_ss, cset, cset, 0); // STOPPED SETUP state : blue
          break;
      default:
          display_error (1, 1, 1); // Error -- invalid mode
    	  break;
      }
    }
    display_colon (ss & 1 ? crun:0);
    ws2812_update ();
    uptodate = 1;
    critsec_last=critsec;
    last_op=index+2;
  }

  indexLast=index;
  timeLast = now;
}

// ******************************
// TESTING CODE ONLY
// ******************************

void RunTimerTest (void){
  static uint32_t timeLast=0;
  static uint32_t timeRef=0;
  static int run=1;
  uint32_t now = HAL_GetTick();

  if (now < timeLast+50)
	  return;

  if (Button2Pin == BUTTON_PRESSED) // reset
	  timeRef = now;

  if (Button1Pin == BUTTON_PRESSED){ // start/stop
	  timeLast = now;
	  run = run ? 0:1;
  }

  if (run){
	  uint32_t timeElapsed = (now-timeRef)/10;

	  int critsec = timeElapsed/100;

	  int hh = critsec/3600;
	  int mm = critsec/60-hh*60;
	  int ss = critsec-hh*3600-mm*60;

	  if (hh > 0)
		  display_two_numbers (hh, mm, 1,1, 0);
	  else if (mm > 0)
		  display_two_numbers (mm, ss, 1,2, 0);
	  else
		  display_two_numbers ((timeElapsed/100)%60, timeElapsed%100, 1,4, 0);
	  ws2812_update ();
  }
	last_op=-10;
}

void DisplayRTCDate (){
	  uint8_t rtc_yr, rtc_wd, rtc_mo, rtc_dy;

	  rtc_get_date(&rtc_yr, &rtc_mo, &rtc_dy, &rtc_wd);
	  display_two_numbers(20,rtc_yr, 1, 1, 0);
	  HAL_Delay (2000);
	  display_two_numbers(rtc_mo,rtc_dy, 1, 1, 0);
	  HAL_Delay (2000);
	  display_two_numbers(88,rtc_wd, 1, 1, 0);
	  HAL_Delay (2000);
	  ws2812_update ();
}

/* RTC structure */
extern RTC_HandleTypeDef hrtc;

void RunRTCClock (){
	static uint32_t tp=0;
	uint32_t t, t_fine, now, full_sec; // ms
	uint32_t leds_minute=0;
	const uint32_t LEDS_TAIL=6*16;
	//uint8_t rtc_h, rtc_m, rtc_s;
	static int state=0;
	static int view_mode=0; // 0: Time/ 1: Date/ 2:Year
	static uint32_t td=100;
	static uint32_t timeLast=0;
	static uint32_t timeLastPress=0;
	static int lpress=0;
	static int chime_h=0;
	static int chime_next=0;
	static int chime_mode=0;
	static int chime_i=0;

	RTC_TimeTypeDef cTime = {0};
	RTC_DateTypeDef cDate = {0};

	HAL_RTC_GetTime(&hrtc, &cTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &cDate, RTC_FORMAT_BIN);
	now = HAL_GetTick();

	/*
	cDate.Date;
	cDate.Month;
	cDate.Year;
	cDate.WeekDay;
	cTime.Seconds;
	cTime.Minutes;
	cTime.Hours;
*/

	// rtc_get_time(&rtc_h, &rtc_m, &rtc_s);
	//t=rtc_s; // rtc_get_seconds_of_day ();
	t = cTime.Seconds;
	if (t != tp){
		full_sec = now;
	}
	tp=t;


	uint8_t *hh, *mm;
	switch (state){
	    case 0: // check controls
			if (ButtonS1Pin == BUTTON_PRESSED){ // show date, year
				fillBufferBlack ();
				view_mode++, state=1;
				view_mode %= 3;
			}
			switch (view_mode){
			case 0: hh = &cTime.Hours; mm = &cTime.Minutes; break;
			case 1: hh = &cDate.Month; mm = &cDate.Date; break;
			case 2: hh = &cDate.WeekDay; mm = &cDate.Year; break;
			}
			if (ButtonS2Pin == BUTTON_PRESSED){ // adjust LED power
				LED_power <<= 1;
				if (LED_power > 128 || LED_power < 1)
					LED_power = 1;
				state=1;
			}

			// HH++
			if (Button3Pin == BUTTON_PRESSED){ // cTime.Hours, cTime.Minutes
				if (*hh < 23){ // limit
					(*hh)++; state=2;
				} else {
					*hh=0; state=2;
				}
			}
			// HH--
			if (Button4Pin == BUTTON_PRESSED){
				if (*hh > 0){ // limit
					(*hh)--; state=2;
				} else {
					*hh=23; state=2;
				}
			}
			// MM++
			if (Button1Pin == BUTTON_PRESSED){
				if (*mm < 59){ // limit
					(*mm)++; state=2;
				} else {
					*mm = 0; (*hh)++; state=2;
				}
			}
			// MM--
			if (Button2Pin == BUTTON_PRESSED){
				if (*mm > 0){ // limit
					(*mm)--; state=2;
					if (view_mode==0)
						cTime.Seconds=0;
				} else {
					*mm = 59; (*hh)--; state=2;
				}
			}
			if (*hh > 23){ *hh=0; }


			if (state > 0){
				timeLastPress = now;
			} else
				lpress = 0;

			if (state==2){
				__HAL_RTC_WRITEPROTECTION_DISABLE (&hrtc);
				if (view_mode>0){
					//RTC_EnterInitMode (&hrtc);
					HAL_RTC_SetDate(&hrtc, &cDate, RTC_FORMAT_BIN);
					HAL_RTC_WaitForSynchro(&hrtc);
				}else
					HAL_RTC_SetTime(&hrtc, &cTime, RTC_FORMAT_BIN);
				__HAL_RTC_WRITEPROTECTION_ENABLE (&hrtc);
			}
			break;

		case 1: // toggle calendar
			if (now - timeLastPress < 1500) return;
			state = 0;
			break;

		case 2: // check for long press
			if (lpress)
			  td=300; // speedup on long press
			else
			  td=1000; // 1st press

			if (now - timeLastPress < td) return;

			lpress= 1;
	        state = 0;
	        break;

	    default: state = 0; break;
	}

	if (state > 0)
		timeLastPress = now;

	if (now < timeLast+50)  // DMA transfer time: 800000/466/24 = 14ms
		return;

	timeLast = now;

	t_fine = t*1000 + now - full_sec;

	switch (view_mode){
	case 0:
		display_two_numbers_leadzero (cTime.Hours, cTime.Minutes, 1, 1, 0);
		//display_two_numbers_leadzero (rtc_h,rtc_m, 1, 1, 0);
		//display_two_numbers_leadzero(rtc_m,rtc_s, 1, 1, 0);

		leds_minute = t*96/60;
		addLEDcolor (16+(leds_minute+0) % LEDS_TAIL, 0, 0, LED_power);
		addLEDcolor (16+(leds_minute+1) % LEDS_TAIL, 0, 0, 2*LED_power);
		addLEDcolor (16+(leds_minute+2) % LEDS_TAIL, 0, 0, LED_power);
#if 0
		leds_minute = (t_fine * 3*LEDS_TAIL / 60000) % (3*LEDS_TAIL);
		int p, p1, p2, p3;
		p = LED_power<<1;
		switch (leds_minute%3){
			case 0: p1 = LEDpower<<1; p2 = LEDpower;  p3 = LED_power>>4; break;
			case 1: p1 = LEDpower; p2 = LEDpower<<1;  p3 = LED_power>>2; break;
			case 2: p1 = LEDpower>>1; p2 = LEDpower;  p3 = LED_power<<1; break;
		}
		leds_minute = (t_fine * LEDS_TAIL / 60000 ) % LEDS_TAIL;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p3); leds_minute += LEDS_TAIL-1;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p2); leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p1); p1 >>= 2; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p); p >>= 2; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p); p >>= 2; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p); p >>= 2; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p); p >>= 2; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, 0, p); p >>= 2; leds_minute--;
#endif
#if 1
		leds_minute = (t_fine * LEDS_TAIL / 1000 ) % LEDS_TAIL;
		int p=LED_power>>1;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute += LEDS_TAIL-1;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
		addLEDcolor (16+leds_minute % LEDS_TAIL, 0, p, 0); p >>= 1; leds_minute--;
#endif
		// blinking colon
		for (int k=2; k<6; ++k)
			setLEDcolor (2*7*16+6+k, t & 1 ? LED_power/2 : 0, 0,0);
		break;
	case 1:
		display_two_numbers (cDate.Month, cDate.Date, 2, 2, 0);
		for (int k=0; k<2; ++k)
			setLEDcolor (2*7*16+6+k, 0, LED_power/2,0);
		break;
	case 2:
		display_two_numbers_leadzero (20, cDate.Year, 5, 5, 0);
		for (int k=5; k<6; ++k)
			setLEDcolor (2*7*16+6+k, 0, 0, LED_power/2);
		break;
	}

	if (cTime.Hours >= 6) // silence at night!
		switch (chime_mode){
			case 0:
				if (cTime.Minutes==0 && cTime.Seconds==0 && chime_h!=cTime.Hours){ // full hour chime
					chime_h=cTime.Hours;
					chime_i=cTime.Hours>12 ? 13 : 1; // 12 max
					chime (6);
					chime_next=2;
					chime_mode=1;

				}
				if (cTime.Minutes==30 && cTime.Seconds==0){ // half hour chime
					chime (4);
					chime_mode=2;
				}
				if (cTime.Minutes==59 && cTime.Seconds==1){ // update
					if (cTime.Hours < 7)
						dfplayer_volume(16);
					else
						dfplayer_volume(22);
					chime_mode=2;
				}
				break;
			case 1:
				if (cTime.Seconds==chime_next){
					if (chime_i<cTime.Hours){
						chime (6); // 5 large church bell
						chime_next += 2;
						chime_i++;
					} else chime_mode=0;
				}
				break;
			case 2:
				if (cTime.Seconds>10)
					chime_mode=0;
				break;
		}

	// running HUE leds test
#if 0
	static int testi=0;
	p=3*6+4*7*16;
	setLEDcolorHSV ((testi++) % p, testi&0xff, 255, 2);
	setLEDcolorHSV ((testi) % p, testi&0xff, 255, 20);
#endif

	ws2812_update ();


#if 0
	for (int i=0; i<LEDS_TAIL; ++i){
		int p=i*LEDpower/LEDS_TAIL;
		int m=0;
		setLEDcolor (m+16+(leds_minute+LEDS_TAIL-i)%LEDS_TAIL, LEDpower-p, 0, p/4);
		for (int j=0; j<4; ++j, m+=LEDS_DIGI){
			t_fine = (t%20)*1000 + now - full_sec;
			  	  int leds_ten = t_fine * 16 / 10000;
				  for (int k=0; k<16; ++k)
					  setLEDcolor (m+(leds_ten+16-k)%16, 0, LEDpower-k, k/4);
			  	  int leds_dots = t_fine * 6 / 2000;
				  for (int k=0; k<6; ++k)
					  setLEDcolor (m+(7*16)+(leds_dots+6-k)%6, 0,0,LEDpower*k/6);
			  }
		  }
	  }
#endif
last_op=99;
}

// DHT22 on
#define DHT22_PORT GPIOB
#define DHT22_PIN  GPIO_PIN_9

static void DHT22_pin_write_mode()
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(DHT22_PORT , DHT22_PIN, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = DHT22_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void DHT22_pin_read_mode()
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(DHT22_PORT , DHT22_PIN, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = DHT22_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

int DHT22_Start (void)
{
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
	DHT22_pin_write_mode (); // HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);   // pull the pin low
	delay_us (1200);   // wait for > 1ms
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_SET);
	HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);   // pull the pin high
	delay_us (4);   // wait for 30us
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
	DHT22_pin_read_mode ();
	uint8_t x;
	int r=0;
	while ((x=HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))==GPIO_PIN_SET){
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, x?GPIO_PIN_SET:GPIO_PIN_RESET); r++; }
	while ((x=HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))==GPIO_PIN_RESET){
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, x?GPIO_PIN_SET:GPIO_PIN_RESET); r++; }
	while ((x=HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))==GPIO_PIN_SET){
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, x?GPIO_PIN_SET:GPIO_PIN_RESET); r++; }
	//while ((x=HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))==GPIO_PIN_RESET){
	//	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, x?GPIO_PIN_SET:GPIO_PIN_RESET); r++; }
	return r;
}

uint8_t DHT22_Check_Response (void)
{
	//DHT22_pin_read_mode ();
	uint8_t Response = 0;
	delay_us (2);  // wait for 40us
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
	if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) // if the pin is low
	{
		//delay_us (5);   // wait for 80us

		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_SET);
		if ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) Response = 1;  // if the pin is high, response is ok
		else Response = -1;
	}

	uint32_t ref = HAL_GetTick();
	while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))){   // wait for the pin to go low
		uint32_t now = HAL_GetTick();
		if (now > ref+2000) // timeout
			return -1;
	}
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
	return Response;
}
uint8_t DHT22_Read (void)
{
	uint8_t i,j;
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_SET);
	delay_us (2);   // wait for 40 us
	HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
	for (j=0;j<8;j++)
	{
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_SET);
		delay_us (1);   // wait for 40 us
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
		while (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)));   // wait for the pin to go high
		delay_us (40);   // wait for 40 us

		if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)))   // if the pin is low
		{
			i&= ~(1<<(7-j));   // write 0
			HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);
		}
		else{
			i|= (1<<(7-j));  // if the pin is high, write 1
			HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_SET);
		}
		while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)));  // wait for the pin to go low
	}

	return i;
}

void RunModeLight(){
	static uint32_t adc0, adc1;
	static uint32_t timeLast=0;
	uint32_t now = HAL_GetTick();

	if (now < timeLast+100000)
		return;

	display_number(-1, 4, 0);
	ws2812_update();

	HAL_ADC_Start(&hadc1);

	display_number(-2, 4, 0);
	ws2812_update();

	HAL_ADC_PollForConversion(&hadc1, 100);
	adc0 = HAL_ADC_GetValue(&hadc1);

	display_number(-3, 4, 0);
	ws2812_update();

	HAL_ADC_PollForConversion(&hadc1, 100);
	adc1 = HAL_ADC_GetValue(&hadc1);

	display_number(-4, 4, 0);
	ws2812_update();

	HAL_ADC_Stop (&hadc1);

	display_number(adc0, 4, 0);
	ws2812_update();

	timeLast = HAL_GetTick();

}

void RunModeDisplayTempHumidity(){
	static uint32_t timeLast=0;
	static int Tc10=0, Rh10=0;
	uint32_t now = HAL_GetTick();
	if (Rh10>0 && now < timeLast+20000){
		if (now > timeLast+17000){
			display_TempC (Tc10, Tc10>0? 1:3);
			ws2812_update();
		}else if(now > timeLast+15000){
			display_RH (Rh10, 3);
			ws2812_update();
		}else{
			display_dot(1, 0, 0);
			display_dot(1, 2, 0);
			display_dot(2, 0, 0);
			RunRTCClock ();
		}
		return;
	}
	timeLast = HAL_GetTick();

	if (DHT22_Start ()){
	//if (DHT22_Check_Response () == 1){
		uint8_t humidi = DHT22_Read ();
		uint8_t humidd = DHT22_Read ();
		uint8_t tempi = DHT22_Read ();
		uint8_t tempd = DHT22_Read ();
		uint8_t checks = DHT22_Read ();
		uint8_t sum = humidi+humidd+tempi+tempd;
		HAL_GPIO_WritePin(DHT22_PORT , GPIO_PIN_3, GPIO_PIN_RESET);

		Tc10 = (int)((tempi&0x7f)<<8 | tempd )*(tempi&0x80 ? -1:1);

//	      f = ((word)(data[2] & 0x7F)) << 8 | data[3];
//	      f *= 0.1;
//	      if (data[2] & 0x80) f *= -1;


		Rh10 = (int)((humidi&0x7f)<<8 | humidd );
	} else {
		display_two_numbers (99, 99, 1,2, 0);
		ws2812_update();
	}
}

extern uint8_t DigitValue[18];
void StandByMode(){
	static uint32_t timeLast=0;
	static uint8_t c = 1;
	uint32_t now = HAL_GetTick();
	uint8_t color[4] = { 0, 1, 2, 0 };
	uint8_t segm[4]  = { 0, DigitValue[DIGIT_Minus], DigitValue[DIGIT_Minus], 0 };


	if (now < timeLast+500){
		return;
	}
	timeLast = HAL_GetTick();

	if (c&1){
		color[1] = c, color[2]=0;
	} else {
		color[1] = 0, color[2]=c;
	}
	c++; c%=32;
	SetSegments (color, segm); // Note: does NOT turn LEDs OFF!

#if 0
	static int x=0;
	static int y=0;
	// [-48 ... +48] [ -17 ... +17 ]
	setLEDcolorHSVxy(x, y,  0,0,0);
	setLEDcolorHSVxy(x+1, y,0,0,0);
	setLEDcolorHSVxy(x,   y+1,0,0,0);
	setLEDcolorHSVxy(x+1, y+1,0,0,0);
	++x; if (x > 48) { x=-48; ++y; if (y>17) y=-17; }
	setLEDcolorHSVxy(x,   y,  2*x+100, 200, 200);
	setLEDcolorHSVxy(x+1, y,  2*x+100, 200, 200);
	setLEDcolorHSVxy(x,   y+1,  2*x+100, 200, 200);
	setLEDcolorHSVxy(x+1, y+1,  2*x+100, 200, 200);
#endif
	ws2812_update();
}

// ******************************
// MODE CONTROL state machine
// ******************************

int RunModeControl(int mmax){
  static uint32_t timeLast=0L;
  static int state=1;

#ifdef STORE_MODE_IN_RTC
  static int mode=-1;
  if (mode < 0)
		mode = rtc_read_reg (kRamAddress0+STORE_MODE_IN_RTC);
#else
  static int mode=5;
#endif

  if ((HAL_GetTick() - timeLast < 250)) return 0;
  switch (state){
    case 0:
      if (ButtonMDPin == BUTTON_PRESSED){
    	  ++mode;
    	  mode %= mmax;
#ifdef STORE_MODE_IN_RTC
    	  rtc_write_reg (kRamAddress0+STORE_MODE_IN_RTC, mode);
#endif
    	  state++;
      } else
    	  return mode+1;
      break;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: state++; break;
    default: state=0; break;
  }

  if (state>0){
	fillBufferBlack ();
    display_mode (mode+1, mode_colors);
	ws2812_update ();
    last_op = -1;
  }

  timeLast = HAL_GetTick();

  return 0;
}

#if 0
void main_loop(){
	  fillBufferBlack ();
	  for(;;){
			switch(RunModeControl(7)){
				case 0: break;
				case 1:	RunModeLapCntr (); break; // LAPS
				case 2: RunModeTime (0, 1); break; // TT
				case 3: RunModeTime (1, -1); break; // Crit Blue
				case 4: RunModeTime (2, -1); break; // Crit Green
				case 5: RunModeTime (3, 1); break;  // Tmr
				case 6: RunRTCClock (); break; // RTClockk
				case 7: StandByMode (); break;
			}
	  }
#endif

