//*****************************************************************************
//
// MSP432 main.c
//
// Assignment 1.b
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
    P2OUT &= ~(BIT0 | BIT1 | BIT2);
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
    TA0CTL = 0x0140; //ACLK (128 KHZ), no divider, timer is halted, disable the interrupt, no interrupt pending
    TA0CCTL0 = 0x2000; // No capture mode
    TA0CCR0 = 0xFA00; // 32000 - Period of timer (1/4 of a second)
    TA0CTL = 0x0156; // ACLK, no divder, up mode, Enable the interrupt, clear TimerA, no interrupt pending
}

uint8_t automatic = 0;  // 0: Manual
                        // 1: Automatic


uint8_t color[] = {
        0,                      //Off
         BIT0,                  //Red
                BIT1,           //Green
        (       BIT1 | BIT2),   //Yellow
                       BIT2,    //Blue
        (BIT0 |        BIT2),   //Violet
        (       BIT1 | BIT2),   //Teal
        (BIT0 | BIT1 | BIT2)    //White
};

uint8_t color_index = 0;
void CycleLED(void){
    if(color_index > 6){ // Roll over
        color_index = 0;
    }
    else{
        color_index++; // Next color
    }
}

void TurnOnLED(void){
    P2OUT = color[color_index]; // Update color
}

void TurnOffLED(void){
    P2OUT = color[0];
}

uint8_t blink = 0;

void TimerA0Interrupt(void){
    uint8_t intv = TA0IV;
    if(intv == 0x0E){ // Interrupt called by overflow
        if(blink){
            if(automatic){
                CycleLED();
            }
            TurnOnLED();
        }
        else
            TurnOffLED();
        blink = !blink;
    }
}

void PortOneInterrupt(void){ // One of the buttons is pushed
    uint8_t iflag = P1IV; // Interrupt flag (hopefully 1.1 or 1.4)
    if(!(iflag & BIT1)){ // Automatic toggle
        automatic = !automatic;
    }
    if(!(iflag & BIT4)){ // Manual cycle
        if(!automatic){
            CycleLED();
            if(!blink) TurnOnLED();
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
