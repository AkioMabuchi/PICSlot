/*
 * File:   SlotMain.c
 * Author: Akio
 *
 * Created on 2018/08/14, 18:03
 */

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define FALSE 0
#define TRUE 1

#define STANDBY 0
#define PREPARE_FOR_ROTATE 1
#define ROTATING 2
#define FEVER 3

#define STOP_FLICKING 0
#define SLOW_FLICKING 1
#define MIDDLE_FLICKING 2
#define FAST_FLICKING 3

unsigned char numbersForDisplay[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char number0 = 1;
unsigned char number1 = 2;
unsigned char number2 = 3;
unsigned char isRotating0 = FALSE;
unsigned char isRotating1 = FALSE;
unsigned char isRotating2 = FALSE;
unsigned char flickMode0 = STOP_FLICKING;
unsigned char flickMode1 = STOP_FLICKING;
unsigned char flickMode2 = STOP_FLICKING;
unsigned char isDisplaying0 = TRUE;
unsigned char isDisplaying1 = TRUE;
unsigned char isDisplaying2 = TRUE;
unsigned char slotStatus = STANDBY;
unsigned char lightingDisplay = 0;
unsigned char isMuteMode = FALSE;
unsigned char prepareTimeForRotate = 0;
unsigned char prepareTimeForStandby = 0;
unsigned char prepareTimeForFlickL = 0;
unsigned char prepareTimeForFlickH = 0;
unsigned char flickControll = 0;
unsigned char soundControll = 0;

unsigned char isDisplayed(unsigned char flickMode){
    if(flickMode == STOP_FLICKING){
        return TRUE;
    }else if(flickMode == SLOW_FLICKING && (flickControll & 0x04)){
        return TRUE;
    }else if(flickMode == MIDDLE_FLICKING && (flickControll & 0x02)){
        return TRUE;
    }else if(flickMode == FAST_FLICKING && (flickControll & 0x01)){
        return TRUE;
    }
    return FALSE;
}

void clearGhost(void){
    PORTAbits.RA0 = 0;
    PORTAbits.RA1 = 0;
    PORTAbits.RA2 = 0;
    PORTB = 0b00000000;
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
}

void __interrupt() isr(void){
    
    if(INTCONbits.TMR0IF == 1){
        INTCONbits.TMR0IF = 0;
        if(lightingDisplay == 0){
            lightingDisplay = 1;
            clearGhost();
            if(isDisplaying0){
                PORTAbits.RA0 = 1;
                PORTB = numbersForDisplay[number0];
            }
        }else if(lightingDisplay == 1){
            lightingDisplay = 2;
            clearGhost();
            if(isDisplaying1){
                PORTAbits.RA1 = 1;
                PORTB = numbersForDisplay[number1];
            }
        }else if(lightingDisplay == 2){
            lightingDisplay = 0;
            clearGhost();
            if(isDisplaying2){
                PORTAbits.RA2 = 1;
                PORTB = numbersForDisplay[number2];
            }
        }
        //----------------------------------------------------------------------
        if(soundControll > 0 && isMuteMode == FALSE){
            soundControll--;
            PORTAbits.RA3 = 1;
        }else{
            PORTAbits.RA3 = 0;
        }
        //----------------------------------------------------------------------
        if(prepareTimeForFlickL > 240){
            prepareTimeForFlickL = 0;
            prepareTimeForFlickH++;
            if(prepareTimeForFlickH > 5){
                prepareTimeForFlickH = 0;
                isDisplaying0 = isDisplayed(flickMode0);
                isDisplaying1 = isDisplayed(flickMode1);
                isDisplaying2 = isDisplayed(flickMode2);
                if(flickControll >= 15){
                    flickControll = 0;
                }else{
                    flickControll++;
                }
            }else{
                prepareTimeForFlickH++;
            }
        }else{
            prepareTimeForFlickL++;
        }
    }
    
    if(PIR1bits.TMR1IF == 1){
        PIR1bits.TMR1IF = 0;
        if(slotStatus == PREPARE_FOR_ROTATE){
            prepareTimeForRotate--;
            if(prepareTimeForRotate == 4 || prepareTimeForRotate == 5){
                soundControll = 200;
            }
            if(prepareTimeForRotate == 0){
                slotStatus = 2;
                isRotating0 = TRUE;
                isRotating1 = TRUE;
                isRotating2 = TRUE;
            }
        }else if(slotStatus == ROTATING){
            soundControll = 150;
            if(isRotating0){
                number0++;
                if(number0 > 9){
                    number0 = 1;
                }
            }
            if(isRotating1){
                number1++;
                if(number1 > 9){
                    number1 = 1;
                }
            }
            if(isRotating2){
                number2++;
                if(number2 > 9){
                    number2 = 1;
                }
            }
        }else if(slotStatus == FEVER){
            if(prepareTimeForStandby == 0){
                slotStatus = STANDBY;
                flickMode0 = STOP_FLICKING;
                flickMode1 = STOP_FLICKING;
                flickMode2 = STOP_FLICKING;
            }else{
                prepareTimeForStandby--;
            }
        }
    }
}

void main(void) {
    ANSELA = 0x00;      //????PORTA????????????
    ANSELB = 0x00;      //????PORTB????????????
    TRISA = 0b11110000;     //RA4?RA7????????????
    TRISB = 0b00000000;     //PORTB???????
    OSCCON = 0b01110010;    //8MHz
    OPTION_REG = 0b00001010;    //use interbal clock, use PSA, 1/8
    INTCON = 0b11100000;    //??????????????????0???????
    T1CON = 0b01110001;
    PIE1bits.TMR1IE;
    
    if(PORTAbits.RA6 == 0){ //????????????RA6???????????????????
        isMuteMode = TRUE;
        number0 = 7;
        number1 = 8;
        number2 = 9;
    }
    
    while(TRUE){
        if(slotStatus == STANDBY){
            if((PORTA & 0b11110000) == 0b01110000){
                slotStatus = PREPARE_FOR_ROTATE;
                prepareTimeForRotate = 6;
            }
        }else if(slotStatus == ROTATING){
            //----------------------------------------------------------------------
            if((PORTA & 0b11110000) == 0b11100000){
                isRotating0 = FALSE;
            }else if((PORTA & 0b11110000) == 0b11010000){
                isRotating1 = FALSE;
            }else if((PORTA & 0b11110000) == 0b10110000){
                isRotating2 = FALSE;
            }
            //----------------------------------------------------------------------
            if(isRotating0 == FALSE && isRotating1 == FALSE && number0 == number1){
                flickMode0 = SLOW_FLICKING;
                flickMode1 = SLOW_FLICKING;
            }else if(isRotating0 == FALSE && isRotating2 == FALSE && number0 == number2){
                flickMode0 = SLOW_FLICKING;
                flickMode2 = SLOW_FLICKING;
            }else if(isRotating1 == FALSE && isRotating2 == FALSE && number1 == number2){
                flickMode1 = SLOW_FLICKING;
                flickMode2 = SLOW_FLICKING;
            }
            //----------------------------------------------------------------------
            if(isRotating0 == FALSE && isRotating1 == FALSE && isRotating2 == FALSE){
                if(number0 == number1 && number1 == number2){
                    slotStatus = FEVER;
                    if(number0 == 7){
                        prepareTimeForStandby = 250;
                        
                        flickMode0 = FAST_FLICKING;
                        flickMode1 = FAST_FLICKING;
                        flickMode2 = FAST_FLICKING;
                    }else{
                        prepareTimeForStandby = 180;
                        flickMode0 = MIDDLE_FLICKING;
                        flickMode1 = MIDDLE_FLICKING;
                        flickMode2 = MIDDLE_FLICKING;
                    }
                }else{
                    slotStatus = STANDBY;
                    flickMode0 = STOP_FLICKING;
                    flickMode1 = STOP_FLICKING;
                    flickMode2 = STOP_FLICKING;
                }
            }
        }else if(slotStatus == FEVER){
            soundControll = 100;
        }
    }
}

//??????????????????????????

