
/*
 * File:        TFT, keypad, DAC, LED test
 * Author:      Bruce Land
 * For use with Sean Carroll's Big Board
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
#include <math.h>
////////////////////////////////////

////////////////////////////////////
// pullup/down macros for keypad
// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;
#define EnablePullUpB(bits) CNPDBCLR=bits; CNPUBSET=bits;
#define DisablePullUpB(bits) CNPUBCLR=bits;
//PORT A
#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
#define DisablePullDownA(bits) CNPDACLR=bits;
#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;
#define DisablePullUpA(bits) CNPUACLR=bits;
////////////////////////////////////

////////////////////////////////////
// some precise, fixed, short delays
// to use for extending pulse durations on the keypad
// if behavior is erratic
#define NOP asm("nop");
// 1/2 microsec
#define wait20 NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
// one microsec
#define wait40 wait20;wait20;
////////////////////////////////////

/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[60];

////////////////////////////////////
// DAC ISR
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000
//== Timer 2 interrupt handler ===========================================
volatile unsigned int DAC_data ;// output value
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC

#define sine_table_size 256
#define Fs 40000.0
volatile int sin_table[sine_table_size];
#define two32 4294967296.0 // 2^32 
volatile unsigned int phase_incr_1, phase_incr_2, phase_accum_1, phase_accum_2, phase_incr_test;
volatile unsigned int phase_accum_main, phase_incr_main;
volatile int test_isr_int = 2;
volatile int ramp_shift = 12;
volatile int ramp_end = 0;
volatile int ramp_init = 0;

volatile int isr_inc = 0;
volatile int ramp_inc = 0;

volatile int key_index = 0;
volatile int play = 0;
volatile int phase1_save[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
volatile int phase2_save[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
volatile int phase_select_1;
volatile int phase_select_2;
volatile int pushing;

volatile unsigned int test_mode = 0;
volatile unsigned int ramp = 0;
volatile unsigned int other_inc;


void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    mT2ClearIntFlag();
    
   isr_inc +=1;

   if(test_mode==0){
        if(play==1){
            phase_select_1 = phase_incr_1;
            phase_select_2 = phase_incr_2;
            
            if(isr_inc<2600){
                if(ramp<255) ramp++;
            }
            else if(isr_inc<3000){
                if(ramp>0)ramp--;
            }
            else{
                ramp = 0;
                play = 0;
            }
           phase_accum_1 += phase_incr_1;
           phase_accum_2 += phase_incr_2;
        }
        else if(play==2){
            if(phase1_save[key_index]==-1 || key_index>11){
                play = 0;
            }
            else{
                phase_select_1 = phase1_save[key_index];
                phase_select_2 = phase2_save[key_index];
                phase_accum_1 += phase_select_1;
                phase_accum_2 += phase_select_2;
                if(isr_inc<3000){
                    if(ramp<255) ramp++;
                }
                else if(isr_inc<9001){
                    if(ramp>0)ramp--;
                }
                else{
                    key_index ++;
                    isr_inc = 0;
                    ramp = 0;
                }
            }
        }
        else{
            ramp_shift = 12;
        }
        DAC_data = (((sin_table[phase_accum_1>>24]+sin_table[phase_accum_2>>24])*ramp)>>8)+2048;
        // DAC_data = ((sin_table[phase_accum_1>>24]+sin_table[phase_accum_2>>24]+2048)>>ramp_shift)  ;
    }

   else{
        
        phase_select_1 = phase_incr_test;
        if(pushing){
            ramp_shift = 0;
        }
        else{
            ramp_shift = 12;
        }
        phase_accum_1 += phase_select_1;
        DAC_data = ((sin_table[phase_accum_1>>24]+2048)>>ramp_shift)  ;
    }

    // === Channel A =============
    // CS low to start transaction
     mPORTBClearBits(BIT_4); // start transaction
    // test for ready
     //while (TxBufFullSPI2());
    // write to spi2 
    WriteSPI2( DAC_config_chan_A | (DAC_data));
    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
     // CS high
    mPORTBSetBits(BIT_4); // end transaction
     
}
static struct pt pt_timer, pt_key, pt_keyprint;



// === thread structures ============================================
// thread control structs
// note that UART input and output are threads

// system 1 second interval tick
int sys_time_seconds ;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
     tft_setCursor(0, 0);
     tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
     tft_writeString("Time in seconds since boot\n");
     // set up LED to blink
     mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        // toggle the LED on the big board
        mPORTAToggleBits(BIT_0);
        // draw sys_time
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Color Thread =================================================
// draw 3 color patches for R,G,B from a random number
static int color ;
static int i;
// animation thread

// === Keypad Thread =============================================
// connections:
// A0 -- row 1 -- thru 300 ohm resistor -- avoid short when two buttons pushed
// A1 -- row 2 -- thru 300 ohm resistor
// A2 -- row 3 -- thru 300 ohm resistor
// A3 -- row 4 -- thru 300 ohm resistor
// B7 -- col 1 -- internal pulldown resistor -- avoid open circuit input when no button pushed
// B8 -- col 2 -- internal pulldown resistor
// B9 -- col 3 -- internal pulldown resistor

static int test = 0;
static int keycode_out;


static int print = 0;
static int inc = 0;

static PT_THREAD (protothread_keyprint(struct pt *pt))
{
    PT_BEGIN(pt);
    
    PT_YIELD_TIME_msec(100);
      while(1) {
          PT_YIELD_UNTIL(&pt_keyprint,print==1);
          tft_fillRoundRect(30,240, 200, 20, 1, ILI9340_BLACK);
          if (keycode_out == 10){
              tft_fillRoundRect(30,200, 200, 20, 1, ILI9340_BLACK);
              int c = 0;
              for(c; c<12; c++){
                phase1_save[c] = -1;
                phase2_save[c] = -1;
              }
              key_index = 0;
              inc = 0;
          }
          else if(keycode_out==11){
              key_index = 0;
              isr_inc = 0;
              play = 2;
          }
          else if(inc > 11){
              print = 0;
          }
          else{
              tft_setCursor(30+15*inc, 200);
              tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
              sprintf(buffer,"%d", keycode_out);
              tft_writeString(buffer);
             /* for(i=0;i<12;i++){
                  tft_setCursor(30+15*i,240);
                  sprintf(buffer,"%d", phase1_save[i]);
                  tft_writeString(buffer);
              }
              */
              
              inc ++;
          }
          print=0;
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    static int keypad, i, pattern;
    // order is 0 thru 9 then * ==10 and # ==11
    // no press = -1
    // table is decoded to natural digit order (except for * and #)
    // 0x80 for col 1 ; 0x100 for col 2 ; 0x200 for col 3
    // 0x01 for row 1 ; 0x02 for row 2; etc
    static int keytable[12]={0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};
    static float freq_table_1[12]={941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941};
    static float freq_table_2[12]={1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1477};
    static float freq_table_test[8]={0, 697, 770, 852, 941, 1209, 1336, 1477};
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);  
    mPORTASetPinsDigitalIn(BIT_4);//Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9 | BIT_15);    //Set port as input
    // and turn on pull-down on inputs
    EnablePullDownA(BIT_4);
    EnablePullDownB( BIT_7 | BIT_8 | BIT_9 | BIT_15);
    
    static int saved_keycode;
    static enum state{release, maybe_push, push, maybe_release};
    static enum state keypad_state = release;
    static float freq_1;
    static float freq_2;
    static float freq_test;
    static int test_read;
    

      while(1) {

          
        // test read toggle
        test_read = mPORTAReadBits(BIT_4);
        test_mode = test_read;
       // mPORTBClearBits(BIT_15);
        
        // yield time
        PT_YIELD_TIME_msec(30);
        
        // read each row sequentially
        mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        pattern = 1; mPORTASetBits(pattern);
   
        for (i=0; i<4; i++) {
            wait40 ;
            keypad  = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            if(keypad!=0) {keypad |= pattern ; break;}
            mPORTAClearBits(pattern);
            pattern <<= 1;
            mPORTASetBits(pattern);
        }
        
        // search for keycode
        if (keypad > 0){ // then button is pushed
            for (i=0; i<12; i++){
                if (keytable[i]==keypad) {
                    
                    break;
                }
                  
            }
            // if invalid, two button push, set to -1
            if (i==12) i=-1;
        }
        else i = -1; // no button pushed
               
        switch(keypad_state){
            case release:
                if (i == -1){
                    keypad_state = release;
                }
                else{
                    keypad_state = maybe_push;
                    saved_keycode = i;
                }
               
                break;
                
            case maybe_push:
                if (i == saved_keycode){
                    freq_1 = freq_table_1[i];
                    freq_2 = freq_table_2[i];        
                    phase_incr_1 = (int)(freq_1*(float)two32/Fs);
                    phase_incr_2 = (int)(freq_2*(float)two32/Fs);
                    if(i<8 && test_mode){
                        freq_test = freq_table_test[i];
                        phase_incr_test = (int)(freq_test*(float)two32/Fs);
                    }
                    keycode_out = i;
                    print = 1;
                    play = 1;
                    isr_inc = 0;
                    if(i<10){
                        phase1_save[key_index] = phase_incr_1;
                        phase2_save[key_index] = phase_incr_2;
                        key_index++;
                    }
                    if(test_mode) pushing = 1;
                    keypad_state = push;    
                  
                }
                else{
                    keypad_state = release;
                }
                break;
                
            case push:
                //put in play tone logic
                if (i == saved_keycode){
                    keypad_state = push;
                }
                else{
                    keypad_state = maybe_release;
                }
                break;
                
            case maybe_release:
                if (i == saved_keycode){
                    keypad_state = push;                    
                }
                else{
                    pushing = 0;
                    keypad_state = release;
                }
                break;
        }

        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // keypad thread

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // set up DAC on big board
  // timer interrupt //////////////////////////
    // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
    // at 30 MHz PB clock 60 counts is two microsec
    // 400 is 100 ksamples/sec
    // 2000 is 20 ksamp/sec
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 1000);

    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

    // SCK2 is pin 26 
    // SDO2 (MOSI) is in PPS output group 2, could be connected to RB5 which is pin 14
    PPSOutput(2, RPB5, SDO2);

    // control CS for DAC
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);

    // divide Fpb by 2, configure the I/O ports. Not using SS in this example
    // 16 bit transfer CKP=1 CKE=1
    // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
    // For any given peripherial, you will need to match these
    // clk divider set to 2 for 20 MHz
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , 2);
  // end DAC setup
    float Fout = 1000.0;
    
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();
  
  int j;
  for (j = 0; j < sine_table_size; j++){
        sin_table[j] = (int)(1023*sin((float)j*6.283/(float)sine_table_size));
  }
  phase_incr_main = (int)(Fout*(float)two32/Fs);
  // init the threads
  PT_INIT(&pt_timer);
  PT_INIT(&pt_key);
  PT_INIT(&pt_keyprint);

  // init the display
  // NOTE that this init assumes SPI channel 1 connections
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_key(&pt_key));
      PT_SCHEDULE(protothread_keyprint(&pt_keyprint));
      }
  } // main

// === end  ======================================================
