#include <p18cxxx.h>
#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#define _XTAL_FREQ   40000000

#include "Includes.h"
#include "LCD.h"

int rb6;
int rb7;
int rb6_pressed;
int rb7_pressed;
int number;
int state=1;
int digit_flag = 0;
int number_of_voters;
int change_flag = 0;
int first_temp;
int second_counter = 0;
int second_flag = 0;
int remaining_time = 90;
int time_changed = 0;
int blink_counter = 0;
int blink_flag = 0;
int blink_status = 1;

char* name_table[10] = {
"Poe   ",
"Galibi",
"Selimi",
"Nesimi",
"Hatayi",
"Zweig ",
"Nabi  ",
"Austen"
};
int seven_seg[10] = {0x3F,0x06,0x5B,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
int digit_place = 1;
int digit_change_flag = 0;
int zeroth_digit = 0x40;
int first_digit = 0x40;
int second_digit = 0x40;
int third_digit = 0x40;
int analog_timer_counter = 0;
int analog_value_high;
int analog_value_low;
int analog_read = 0;
int analog_value;

void updateLCD();
void updateDigits();

void __interrupt() isr(void){
    if(TMR1IF){ //if the interrupt comes from timer1, it sends every 50ms
        TMR1IF = 0;
        if(second_counter == 20){
            second_flag = 1;
            second_counter = 0;
        }
        else{
            second_counter++;
        }
        TMR1 = 6000;
    }
    if(TMR0IF){ // if the interrrupt comes form timer0, it sends every 5ms
        TMR0IF = 0;
        if(blink_counter == 50){
            blink_flag = 1;
            blink_counter = 0;
        }
        else{
            blink_counter++;
        }
        digit_change_flag = 1;
        analog_timer_counter++;
        TMR0L = 64;
    }
    if(RBIF){ //if the interrupt comes from RB port
        if(RB6 && !rb6_pressed){
            rb6_pressed = 1;
            rb6 = 1;
        }
        else if(!RB6 && rb6_pressed){
            rb6_pressed = 0;
            rb6 = 0;
        }
        if(RB7 && !rb7_pressed){
            rb7_pressed = 1;
            rb7 = 1;
        }
        else if(!RB7 && rb7_pressed){
            rb7_pressed = 0;
            rb7 = 0;
        }
        RBIF = 0;
    }
    if(ADIF){ //if the convertion is done
        analog_value_high = ADRESH;
        analog_value_low = ADRESL;
        analog_read = 1;
        ADIF = 0;
    }
}

void init()
{
    GIE = 0;
    TRISB7 = 1;
    TRISB6 = 1;
    TRISB5 = 0;
    TRISB4 = 0;
    TRISB3 = 0;
    TRISB2 = 0;
    TRISB1 = 0;
    TRISB0 = 0;
    RBIE = 1;
    RBIF = 0;
    
    TMR1 = 6000;
    T1CON = 256;
    T1CKPS0 = 1;
    T1CKPS1 = 1;
    TMR1CS = 0;
    
    T08BIT = 1;
    TMR0ON = 0;
    T0CS = 0;
    T0SE = 0;
    PSA = 0;
    T0PS0 = 1;
    T0PS1 = 1;
    T0PS2 = 1;
    TMR0L = 50;
    
    ADCON0 = 0x33; //channel 12,
    ADCON1 = 0x02;  //AN15 and AN14 is Digital other analog
    ADFM = 1;       //A/D Result format select bit is left justified
    TRISH4 = 1;
    
    TMR1IE = 1;
    TMR0IE = 1;
    TMR0IF = 0;
    TMR1IF = 0;
    TMR0ON = 1;
    PEIE = 1;
    GIE = 1;
    //return;
    
}

void state1(){
    if(state == 1){
        WriteCommandToLCD(0x81);   // Goto to the beginning of the first line
        WriteStringToLCD("#Electro Vote#");	
        WriteCommandToLCD(0xC1);   // Goto to the beginning of the first line
        WriteStringToLCD("##############");	
        int c1,c2;
        c1 = 0;
        for(c1=0;c1<3000;c1++){
            for(c2=0;c2<908;c2++){}
        }
        state = 2;
        first_temp = 1;
    }
}

void state2(){
    if(state == 2){
        if(first_temp){
            WriteCommandToLCD(0x81);   // Goto to the beginning of the first line
            WriteStringToLCD("#Enter Voters#");	// Write Hello World on LCD
            WriteCommandToLCD(0xC1);   // Goto to the beginning of the first line
            WriteStringToLCD("00            ");	// Write Hello World on LCD
            first_temp = 0;
        }
        if(digit_flag == 0){
            if(rb6){
                change_flag = 1;
                rb6 = 0;
                if(number_of_voters > 89){
                    number_of_voters -= 90;
                }
                else{
                    number_of_voters += 10;
                }
            }
            else if(rb7){
                rb7 = 0;
                digit_flag = 1;
            }
        }
        else if(digit_flag == 1){
            if(rb6){
                change_flag = 1;
                rb6 = 0;
                if(number_of_voters % 10 == 9){
                        number_of_voters -= 9;
                    }
                    else{
                        number_of_voters += 1;
                    }
            }
            else if(rb7){
                rb7 = 0;
                digit_flag = 2;
            }
        }
        else{
            blink_counter = 0;
            TMR1ON = 1;
            first_temp = 1;
            state = 3;
        }
        __delay_ms(5);
        updateLCD();
    }   
}

void state3(){
    if(state == 3){
        if(first_temp){
            WriteCommandToLCD(0x81);   // Goto to the beginning of the first line
            WriteStringToLCD(" Time left :90");	// Write Hello World on LCD
            WriteCommandToLCD(0xC1);   // Goto to the beginning of the first line
            WriteStringToLCD(">             ");	// Write Hello World on LCD
            first_temp = 0;
        }
        if(blink_flag==1){
            if(blink_status){
                WriteCommandToLCD(0xC1);
                WriteStringToLCD(" ");
                blink_status = 0;
            }
            else{
                WriteCommandToLCD(0xC1);
                WriteStringToLCD(">");
                blink_status = 1;
            }
            blink_flag = 0;
        }
        if(time_changed){
            time_changed = 0;
            WriteCommandToLCD(0x8D);   // Goto to the beginning of the first line
            WriteDataToLCD((char)'0' + remaining_time/10);
            WriteCommandToLCD(0x8E);   // Goto to the beginning of the first line
            WriteDataToLCD((char)'0' + remaining_time%10);
        }
        if(analog_timer_counter > 20){
            analog_timer_counter = 0;
            GO = 1;
        }
        
        if(analog_read){
            analog_value = analog_value_low;
            if(analog_value_high==1){
                analog_value += 256;
            }
            else if(analog_value_high==2){
                analog_value += 512;
            }
            if(analog_value_high==3){
                analog_value += 768;
            }
            updateDigits();
            analog_read = 0;
        }
    }
    return;
}

void updateDigits(){
    /*
    if(analog_value / 1000){
        third_digit = seven_seg[1];
    }
    analog_value = analog_value%1000;
    if(analog_value / 100){
        second_digit = seven_seg[analog_value/100];
    }
    analog_value = analog_value%100;
    if(analog_value / 10){
        first_digit = seven_seg[analog_value/10];
    }
    analog_value = analog_value%10;
    zeroth_digit = seven_seg[analog_value
    */
    if(analog_value<127){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("18");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[0]);
    }
    else if(analog_value<255){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("64");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[1]);
    }
    else if(analog_value<383){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("34");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[2]);
    }
    else if(analog_value<511){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("23");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[3]);
    }
    else if(analog_value<639){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("33");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[4]);
    }
    else if(analog_value<767){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("67");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[5]);
    }
    else if(analog_value<895){
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("63");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[6]);
    }
    else{
        WriteCommandToLCD(0xC2);
        WriteStringToLCD("99");
        WriteCommandToLCD(0xC5);
        WriteStringToLCD(name_table[7]);
    }
}
void update_time(){
    if(second_flag){
        time_changed = 1;
        remaining_time--;
        second_flag = 0;
    }
    if(remaining_time == 0){
        state = 4;
    }
}

void updateLCD() {
    if(change_flag){
        change_flag = 0;
        if(digit_flag == 0){
            WriteCommandToLCD(0xC1); // Goto to the beginning of the second line
            WriteDataToLCD((char)'0' + number_of_voters/10);
        }
        else if(digit_flag == 1){
            WriteCommandToLCD(0xC2); // 
            WriteDataToLCD((char)'0' + number_of_voters%10);
        }
        __delay_ms(2);
    }
    return;
}

void seven_segment_driver(){
    if(digit_change_flag){
        if(digit_place==4){
            digit_place = 0;
        }
        digit_place++;
        digit_change_flag = 0;
    }
    if(digit_place==1){
        PORTJ = zeroth_digit;
        LATH = 0x08;
    }
    else if(digit_place==2){
        PORTJ = first_digit;
        LATH = 0x04;
    }
    else if(digit_place==3){
        PORTJ = second_digit;
        LATH = 0x02;
    }
    else{
        PORTJ = third_digit;
        LATH = 0x01;
    }
    return;
}

void main(void)
{
    InitLCD();			// Initialize LCD in 4bit mode
    init();
    while(1){
        seven_segment_driver();
        state1();
        state2();
        update_time();
        state3();
    }
}