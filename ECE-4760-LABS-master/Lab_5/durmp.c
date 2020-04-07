
/**
///*
// * File:        Groovytimes
// * Author:      The Groovy Katz
// * Target PIC:  PIC32MX250F128B
// */
//
//////////////////////////////////////
//// clock AND protoThreads configure!
//// You MUST check this file!
//#include "config.h"
//// threading library
//#include "pt_cornell_1_2.h"
//#include "tft_master.h"
//#include "tft_gfx.h"
//#define DAC_config_chan_A 0b0011000000000000
//#define DAC_config_chan_B 0b1011000000000000
//#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
//
//volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
//// for 60 MHz PB clock use divide-by-3
//volatile int spiClkDiv = 2 ; // 20 MHz DAC clock
//
//
//// === thread structures ============================================
//// thread control structs
//
////print lock
//
//volatile int V_control;
//volatile int last_error;
//volatile int error_ang;
//volatile int proportional_cntl;
//volatile int differential_cntl;
//volatile int integral_cntl;
//static int Kp  = 90;
//static int Ki = 3;
//static int Kd = 3000;
//volatile int adc_9;
//
//static int bound = 39999;
//static struct pt_sem print_sem ;
//
//// note that UART input and output are threads
//static struct pt pt_cmd, pt_time, pt_input, pt_output, pt_adc, pt_demo, pt_select, pt_button ;
//
//// system 1 second interval tick
//int sys_time_seconds ;
//
////The actual period of the wave
//int generate_period = 0 ;
////volatile int pwm_on_time = 20000;
////print state variable
//int printing=0 ;
//volatile int desired_angle = 500;
//volatile int angle;
//volatile int time_test;
//volatile int V_test;
//static int adc_param;
//
//static int end=0;
//
//char buffer[60];
//
//static int temp_angle;
//static int temp_kp;
//static int temp_ki;
//static int temp_kd;
//static int inc;
//volatile int motor_disp=0;
//float angle_scale;
//
//volatile int v0;
//volatile int v1;
//volatile int v2;
//volatile int v3;
//volatile int v4;
//volatile int v5;
//volatile int v6;
//volatile int v7;
//volatile int v8;
//volatile int v9;
//volatile int v10;
//volatile int v11;
//volatile int v12;
//volatile int v13;
//volatile int v14;
//volatile int v15;
//volatile int ave;
//
//static int mode=0;
//
//// == Timer 2 ISR =====================================================
////just toggles a pin for timeing strobe
//void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
//{
//      // read the result of channel 9 conversion from the idle buffer
//    // generate a trigger strobe for timing other events
//    //mPORTBSetBits(BIT_0);
//    // clear the timer interrupt flag
//    mT2ClearIntFlag();
//    //mPORTBClearBits(BIT_0);
//    angle = ReadADC10(1); 
//    
//    last_error = error_ang;
//    error_ang = (desired_angle - angle);
//
//    proportional_cntl = Kp * error_ang ;
//    differential_cntl = Kd * (error_ang - last_error); 
//    integral_cntl = integral_cntl + error_ang ;
//    if (error_ang != last_error)
//        integral_cntl = 0;
//
//    V_control =  proportional_cntl + differential_cntl +  (integral_cntl>>Ki) ;
// 
//
//    //physical bound of the PWM 
//    if (V_control<0)
//        V_control = 0 ;
//    //physical bound of the PWM 
//    if (V_control>39999)
//        V_control = 39999 ;
//    v15=v14;
//    v14=v13;
//    v13=v12;
//    v12=v11;
//    v11=v10;
//    v10=v9;
//    v9=v8;
//    v8=v7;
//    v7=v6;
//    v6=v5;
//    v5=v4;
//    v4=v3;
//    v3=v2;
//    v2=v1;
//    v1=v0;
//    v0=V_control;
//    ave = (v0+v1+v2+v3+v4+v5+v6+v7+v8+v9+v10+v11+v12+v13+v14+v15)>>4;
//    
//    //if (time_test == 1) pwm_on_time = 10000;
//   // else pwm_on_time = 2000;
//    SetDCOC3PWM(V_control);
//    
//    motor_disp = motor_disp + ((ave - motor_disp)>>9); 
//    // === Channel A =============
//    // CS low to start transaction
//     mPORTBClearBits(BIT_4); // start transaction
//    // test for ready
//     //while (TxBufFullSPI2());
//    // write to spi2 
//    WriteSPI2( DAC_config_chan_A | (angle<<2));
//    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
//     // CS high
//    mPORTBSetBits(BIT_4); // end transaction   
//    
//        // === Channel B =============
//    // CS low to start transaction
//     mPORTBClearBits(BIT_4); // start transaction
//    // test for ready
//     //while (TxBufFullSPI2());
//    // write to spi2 
//    WriteSPI2( DAC_config_chan_B | (ave));
//    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
//     // CS high
//    mPORTBSetBits(BIT_4); // end transaction  
//}
// 
//// === ADC Thread =============================================
//// 
//volatile int select;
//volatile int load;
//
//static PT_THREAD (protothread_adc(struct pt *pt))
//{
//    PT_BEGIN(pt);
//    
//    static float V;
//    static float Vfix, ADC_scale ;
//    
//    ADC_scale = 3.3/1023.0; //Vref/(full scale)
//
//    while(1) {
//        // yield time 1 second
//        PT_YIELD_TIME_msec(60);
//        adc_param = ReadADC10(0);
//        
//        // read the ADC AN11 
//        // read the first buffer position
//        
//         // not needed if ADC_AUTO_SAMPLING_ON below
//
//        // draw adc and voltage
//        tft_fillRoundRect(0,100, 230, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
//        tft_setCursor(0, 100);
//        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
//        // convert to voltage
//        V = (float)adc_9 * 3.3 / 1023.0 ; // Vref*adc/1023
//        // convert to fixed voltage
//        Vfix = adc_9*ADC_scale;
//        
//        // print raw ADC, floating voltage, fixed voltage
//        sprintf(buffer,"%d %d %d %d %d %d", temp_angle, temp_kp, temp_ki, temp_kd, inc, mode);
//        tft_writeString(buffer);
//        
//        
//        // NEVER exit while
//      } // END WHILE(1)
//  PT_END(pt);
//} // animation thread
//
//static PT_THREAD (protothread_select(struct pt *pt)){
//    PT_BEGIN(pt);
//    mPORTASetPinsDigitalIn(BIT_2 | BIT_3 | BIT_4);
//    EnablePullDownA(BIT_2 | BIT_3| BIT_4);
//    
//    
//    inc = 0;
//
//    
//    while(1){
//
//        if(load !=0) {
//            desired_angle = temp_angle;
//            Kp = temp_kp;
//            Ki = temp_ki;
//            Kd = temp_kd;
//        }
//        if (select != 0){
//            inc ++;
//            if (inc>4) inc = 0;
//            select = 0;
//        }
//        
//        if (inc==1){ //Angle
//            temp_angle = adc_param - 100;
//            if (temp_angle<300) temp_angle = 300;
//        }
//        else if(inc==2){ //Proportional
//            temp_kp = (adc_param>>2);
//            if(temp_kp<0) temp_kp = 0;
//        }
//        
//        else if(inc==3){ //Integral
//            temp_ki = (adc_param>>5);
//        }
//        else if(inc==4){ //Derivative
//            temp_kd = adc_param*6;
//        }
//        
//        PT_YIELD_TIME_msec(100);
//    }
//    PT_END(pt);
//}
//
//
//// === One second Thread ======================================================
//// update a 1 second tick counter
//static PT_THREAD (protothread_button(struct pt *pt))
//{
//    PT_BEGIN(pt);
//
//      while(1) {
//            // yield time 1 second
//        load = mPORTAReadBits(BIT_4);
//        select = mPORTAReadBits(BIT_2);
//        mode = mPORTAReadBits(BIT_3);
//        PT_YIELD_TIME_msec(200);
//            // NEVER exit while
//      } // END WHILE(1)
//
//  PT_END(pt);
//} // thread 4
//
//static PT_THREAD (protothread_time(struct pt *pt))
//{
//    PT_BEGIN(pt);
//
//      while(1) {
//            // yield time 1 second
//            PT_YIELD_TIME_msec(1000) ;
//            sys_time_seconds++ ;
//            if (time_test==1) time_test=0;
//            else time_test=1;
//            // NEVER exit while
//      } // END WHILE(1)
//
//  PT_END(pt);
//} // thread 4
//// === Main  ======================================================
//
//int main(void)
//{
//    angle_scale = 3.14/520.0;
//    temp_angle=desired_angle;
//    temp_kp=Kp;
//    temp_ki=Ki;
//    temp_kd=Kd;
//  // === Config timer and output compares to make pulses ========
//  // set up timer2 to generate the wave period
//  OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 40000);
//  ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
//  mT2ClearIntFlag(); // and clear the interrupt flag
//
//  // set up compare3 for PWM mode
//  OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , V_control, V_control); //
//  // OC3 is PPS group 4, map to RPB9 (pin 18)
//  PPSOutput(4, RPB9, OC3);
//
//  // === config the uart, DMA, vref, timer5 ISR ===========
//  PT_setup();
//  
//    ANSELA = 0; ANSELB = 0; 
//
//
//  // === setup system wide interrupts  ========
//  INTEnableSystemMultiVectoredInt();
//  
// 
// 
// //DAC
//     // SCK2 is pin 26 
//    // SDO2 is in PPS output group 2, could be connected to RB5 which is pin 14
//    PPSOutput(2, RPB5, SDO2);
//    // control CS for DAC
//    mPORTBSetPinsDigitalOut(BIT_4);
//    mPORTBSetBits(BIT_4);
//    // divide Fpb by 2, configure the I/O ports. Not using SS in this example
//    // 16 bit transfer CKP=1 CKE=1
//    // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
//    // For any given peripherial, you will need to match these
//    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
//
//    CloseADC10();	
//    	// define setup parameters for OpenADC10
//	// Turn module on | ouput in integer | trigger mode auto | enable autosample
//    // ADC_CLK_AUTO -- Internal counter ends sampling and starts conversion (Auto convert)
//    // ADC_AUTO_SAMPLING_ON -- Sampling begins immediately after last conversion completes; SAMP bit is automatically set
//    // ADC_AUTO_SAMPLING_OFF -- Sampling begins with AcquireADC10();
//    #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON //
//
//	// define setup parameters for OpenADC10
//	// ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
//	#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
//        //
//	// Define setup parameters for OpenADC10
//    // use peripherial bus clock | set sample time | set ADC clock divider
//    // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
//    // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
//    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_15 | ADC_CONV_CLK_Tcy 
//
//	// define setup parameters for OpenADC10
//	// set AN11 and  as analog inputs
//	#define PARAM4	ENABLE_AN11_ANA | ENABLE_AN5_ANA // 
//
//	// define setup parameters for OpenADC10
//    // DO not skip the channels you want to scan
//    // do not specify channels  5 and 11
//	#define PARAM5	SKIP_SCAN_AN0 | SKIP_SCAN_AN1 | SKIP_SCAN_AN2 | SKIP_SCAN_AN3 | SKIP_SCAN_AN4 | SKIP_SCAN_AN6 | SKIP_SCAN_AN7 | SKIP_SCAN_AN8 | SKIP_SCAN_AN9 | SKIP_SCAN_AN10 | SKIP_SCAN_AN12 | SKIP_SCAN_AN13 | SKIP_SCAN_AN14 | SKIP_SCAN_AN15
//
//	// use ground as neg ref for A 
//    // actual channel number is specified by the scan list
//    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF); // 
//	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above
//
//	EnableADC10(); // Enable the ADC
//  tft_init_hw();
//  tft_begin();
//  tft_fillScreen(ILI9340_BLACK); //erase screen
//  
//    
//  // === set up i/o port pin ===============================
//  mPORTBSetPinsDigitalOut(BIT_0 );    //Set port as output
//
//  // === now the threads ===================================
//  
//  // init the threads
//  //PT_INIT(&pt_cmd);
//  PT_INIT(&pt_time);
//  PT_INIT(&pt_adc);
//  PT_INIT(&pt_demo);
//  PT_INIT(&pt_select);
//  PT_INIT(&pt_button);
//
//  // schedule the threads
//  while(1) {
//    //PT_SCHEDULE(protothread_cmd(&pt_cmd));
//    PT_SCHEDULE(protothread_time(&pt_time));
//    PT_SCHEDULE(protothread_adc(&pt_adc));
//    if(mode==0) PT_SCHEDULE(protothread_select(&pt_select));
//    PT_SCHEDULE(protothread_button(&pt_button));
//    if(mode!=0) PT_SCHEDULE(protothread_demo(&pt_demo));
//  }
//} // main
//
//
