//*****************************************************************************
//
// MSP432 main.c
//
// Assignment 1.c
//
// Press button 1.1 to switch between manual cycle mode and automatic cycle mode.
// Press button 1.4 to cycle to the next LED color while in manual cycle mode.
//
//
//  Author: Alex McIntosh
//
//****************************************************************************

#include "msp432.h"
#include "portfunc.h"
#include "clock.h"

void InitializeLEDs(void){
    //P1DIR |= BIT0; //Port 1.0 configured as output.
    //SelectPortFunction(1, 0, 0, 0);
    //P1OUT &= ~BIT0; //Turn off red LED

    P2DIR |=  (BIT0 | BIT1 | BIT2);
    SelectPortFunction(2, 0, 0, 0); // General purpose I/0, Red LED
    SelectPortFunction(2, 1, 0, 0); // General purpose I/0, Green LED
    SelectPortFunction(2, 2, 0, 0); // General purpose I/0, Blue LED
    P2OUT &= ~(BIT0 | BIT1 | BIT2); // Turn all the lights off
}

void InitializePushButton(uint8_t x){
    P1DIR &= ~BIT(x); //Resets port 1.x to input.
    P1REN |= BIT(x); //Pull-up or pull-down enabled
    P1OUT |= BIT(x); //Pull-up selected
    SelectPortFunction(1, x, 0, 0); // General purpose I/0
    P1IES |= BIT(x);
    P1IFG &= ~BIT(x); // Clear interrupt flag
    P1IE |= BIT(x);  // Enable the interrupt
}

void ConfigureTimer(void){
    TA0CTL = 0x0100; //ACLK (128 KHZ), no divider, timer is halted, disable the interrupt, no interrupt pending
    TA0CCTL0 = 0x2000; // No capture mode
    TA0CCR0 = 0x0080; // 128 - Period of timer (Once per millisecond)
    TA0CCTL1 = 0x2010; // Enable TA0CCTL interrupt on register 1
    TA0CCR1 = 0x0081; // After how many clock cycles to fire interrupt on register 1
    TA0CCTL2 = 0x2010; // Enable TA0CCTL interrupt on register 2
    TA0CCR2 = 0x0081; // After how many clock cycles to fire interrupt on register 2
    TA0CCTL3 = 0x2010; // Enable TA0CCTL interrupt on register 3
    TA0CCR3 = 0x0081; // After how many clock cycles to fire interrupt on register 3
    TA0CTL = 0x0116; // ACLK, no divder, up mode, Enable the interrupt, clear TimerA, no interrupt pending
}

struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct color colors[] = {
        {0x81, 0x81, 0x81},//Off
        {0x48, 0x7C, 0x7C},//Pink
        {0x40, 0x81, 0x81},//Red
        {0x48, 0x78, 0x81},//Orange
        {0x58, 0x68, 0x81},//Yellow
        {0x81, 0x40, 0x81},//Green
        {0x81, 0x60, 0x60},//Teal
        {0x7C, 0x7C, 0x48},//Light Blue
        {0x81, 0x81, 0x40},//Blue
        {0x60, 0x81, 0x60},//Violet
};

void AssignColors(struct color c){
    TA0CCR1 = c.red;
    TA0CCR2 = c.green;
    TA0CCR3 = c.blue;
}

uint8_t color_index = 9;

void CycleLED(void){
    if(++color_index > 9){ // Cycle LED
        color_index = 1; // Roll over (Skip 0, because 0 is off state)
    }
    AssignColors(colors[color_index]); //Update LED
}

uint8_t LEDState = 0; // 0: Off, 1: Manual, 2: Automatic

void TimerA0Interrupt(void){
    static uint16_t interruptCycles = 0;
    uint8_t intv = TA0IV;
    switch(intv){
    case 0x0E: // Overflow
        if(++interruptCycles >= 1000){
            interruptCycles = 0;
            if(LEDState == 2){ // Automatic
                CycleLED();
            }
        }
        P2OUT &= ~(BIT0 | BIT1 | BIT2);
        break;
    case 0x02: // TA0CCR1 (red)
        P2OUT |= BIT0;
        break;
    case 0x04: // TA0CCR2 (green)
        P2OUT |= BIT1;
        break;
    case 0x06:// TA0CCR3 (blue)
        P2OUT |= BIT2;
        break;
    }
}

void PortOneInterrupt(void){ // One of the buttons is pushed
    uint8_t iflag = P1IV; // Interrupt flag (hopefully 1.1 or 1.4)
    if(!(iflag & BIT1)){ // Button 1.1
        if(++LEDState > 2){ // Cycle LED State
            LEDState = 0;
        }
        if(!LEDState){ // Off mode
            AssignColors(colors[0]); // Turn off LED
        }
        else{ // Either On mode
            AssignColors(colors[color_index]); // Restore LED color
        }
    }
    if(!(iflag & BIT4)){ // Button 1.4
        if(LEDState == 1){  // Manual Mode
            CycleLED();
        }
    }
}

void main (void){
    WDTCTL = WDTPW | WDTHOLD; // Stop watch-dog timer
    InitializeLEDs();
    SetClockFrequency();
    ConfigureTimer();
    InitializePushButton(1);
    InitializePushButton(4);
    NVIC_EnableIRQ(TA0_N_IRQn);
    NVIC_EnableIRQ(PORT1_IRQn);
    for(;;);
}
