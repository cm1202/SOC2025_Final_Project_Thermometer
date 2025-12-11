/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"

// Display "RST" (Reset)
void disp_RST(SsegCore *sseg_t) {
     // Clear others first
    sseg_t->set_dp(0x00); // Turn off decimal points
    
    // Display letters from left to right or right to left 
    // (Adjust positions 7, 6, 5 based on your preference)
    
    // R (small r)
    sseg_t->write_1ptn(0xAF, 2); 
    // S (looks like 5)
    sseg_t->write_1ptn(0x92, 1); 
    // T (small t)
    sseg_t->write_1ptn(0x87, 0); 
}

// Display "AVG" (Average)
void disp_AVG(SsegCore *sseg_t) {
    //sseg_clear(sseg_t);
    sseg_t->set_dp(0x00);

    // A
    sseg_t->write_1ptn(0x88, 2);
    // V (looks like U)
    sseg_t->write_1ptn(0xC1, 1); 
    // G (looks like 9/g)
    sseg_t->write_1ptn(0x90, 0); 
}

// Display "DIFF" (Difference)
void disp_DIFF(SsegCore *sseg_t) {
    //sseg_clear(sseg_t);
    sseg_t->set_dp(0x00);

    // D (small d)
    sseg_t->write_1ptn(0xA1, 3);
    // I (looks like 1)
    sseg_t->write_1ptn(0xF9, 2);
    // F
    sseg_t->write_1ptn(0x8E, 1);
    // F
    sseg_t->write_1ptn(0x8E, 0);
}
void disp_LIVE(SsegCore *sseg_t) {
    //sseg_clear(sseg_t);
    sseg_t->set_dp(0x00); // Turn off decimal points

    // L (Standard L) -> 0xC7
    sseg_t->write_1ptn(0xC7, 3); 
    
    // I (Uses digit 1) -> 0xF9
    sseg_t->write_1ptn(0xF9, 2); 
    
    // V (Uses U pattern) -> 0xC1
    sseg_t->write_1ptn(0xC1, 1); 
    
    // E (Standard E) -> 0x86
    sseg_t->write_1ptn(0x86, 0); 
}
void temp_diff(float temp, SsegCore *sseg_t, PwmCore *pwm_p){
    sseg_t->set_dp(0x00);

    // 1. MANUAL ABSOLUTE VALUE
    // We need positive numbers for the % math to work correctly
    bool is_negative = false;
    if (temp < 0.0) {
        is_negative = true;
        temp = -temp; // Flip it to positive manually
    }

    // 2. FIX ROUNDING ERROR (The 0.475 fix)
    // 0.475 is often stored as 0.474999. 
    // Adding 0.5 forces it to bump up to the next integer before casting.
    int temp_input = static_cast<int>(temp * 1000.0 + 0.5);

    int temp_array[5];
    temp_array[0] = (temp_input / 10000) % 10; // Tens place
    temp_array[1] = (temp_input / 1000) % 10;  // Ones place (digit left of dot)
    temp_array[2] = (temp_input / 100) % 10;   // 0.1
    temp_array[3] = (temp_input / 10) % 10;    // 0.01
    temp_array[4] = (temp_input / 1) % 10;     // 0.001
    
    // Display "C" symbol
    sseg_t->write_1ptn(sseg_t->h2s(12), 0);
    
    // Decimal point at position 4 (XX.XXX)
    // Note: Your original code had set_dp(0x10) which is Bit 4.
    sseg_t->set_dp(0x10);

    // Write the decimals
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);

    // 3. HANDLE NEGATIVE SIGN / LEADING ZERO
    if (is_negative) {
        // If it is negative, we usually put a dash.
        // If the number is -0.475, temp_array[0] is 0. 
        // We replace that leading 0 with a dash.
        
        // 0xBF is the raw segment code for a dash '-' (center segment only) on most boards
        // If your board is different, you might need a different hex code.
        sseg_t->write_1ptn(0xBF, 5); 
        pwm_p->set_duty(1023,0);
        pwm_p->set_duty(0,1);
        pwm_p->set_duty(0,2); 
    } 
    else if(temp==0){
        pwm_p->set_duty(0,0);
        pwm_p->set_duty(1,1);
        pwm_p->set_duty(0,2); 
    } 

    
    else {
        pwm_p->set_duty(0,0);
        pwm_p->set_duty(0,1);
        pwm_p->set_duty(1023,2); 
        
        // If positive, checking leading zero suppression
        if (temp_array[0] == 0) {
             // Blank the leading zero (0xFF is blank on Active Low)
             sseg_t->write_1ptn(0xFF, 5); 
        } else {
             sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
        }
    }
}

void temp_disp_C(float temp, SsegCore *sseg_t){
    sseg_t->set_dp(0x00);
	int temp_input = static_cast<int>(temp * 1000.0);
	int temp_array[5];
	temp_array[0] = (temp_input / 10000) % 10;
	temp_array[1] = (temp_input / 1000) % 10;
	temp_array[2] = (temp_input / 100) % 10;
	temp_array[3] = (temp_input / 10) % 10;
	temp_array[4] = (temp_input / 1) % 10;
    
    //Celcius
	sseg_t->write_1ptn(sseg_t->h2s(12), 0);
    //decimal point
	sseg_t->set_dp(0x10);

    sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
}

void temp_disp_F(float temp, SsegCore *sseg_t){
    sseg_t->set_dp(0x00);
	int temp_input = static_cast<int>(temp * 1000.0);
	int temp_array[5];
	temp_array[0] = (temp_input / 10000) % 10;
	temp_array[1] = (temp_input / 1000) % 10;
	temp_array[2] = (temp_input / 100) % 10;
	temp_array[3] = (temp_input / 10) % 10;
	temp_array[4] = (temp_input / 1) % 10;
    
    //Celcius
	sseg_t->write_1ptn(sseg_t->h2s(15), 0);
    //decimal point
	sseg_t->set_dp(0x10);

    sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
    sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
}

/*
 * read temperature from adt7420
 * @param adt7420_p pointer to adt7420 instance
 */
void adt7420_check(I2cCore *adt7420_p, GpoCore *led_p, SsegCore *sseg_t,GpiCore *sw_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   //int ack;
   uint16_t tmp;
   float tmpC;
   float tmpF; 
   int s; 
   s = sw_p->read(); 

   // read adt7420 id register to verify device existence
   // ack = adt7420_p->read_dev_reg_byte(DEV_ADDR, 0x0b, &id);

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   //uart.disp("read ADT7420 id (should be 0xcb): ");
   //uart.disp(bytes[0], 16);
   //uart.disp("\n\r");
   //debug("ADT check ack/id: ", ack, bytes[0]);
   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }
   if(s==1){
       
        tmpF = (tmpC*9/5)+32;
        temp_disp_F(tmpF, sseg_t);
   }
   else {
        temp_disp_C(tmpC, sseg_t);
   
   }
   

   /*uart.disp("temperature (C): ");
   uart.disp(tmpC);
   uart.disp("\n\r");
   uart.disp("temperature (F): ");
   uart.disp(tmpF);
   uart.disp("\n\r");*/
   //led_p->write(tmp);
   //sleep_ms(1000);
   //led_p->write(0);
}

float adt7420_read(I2cCore *adt7420_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   //int ack;
   uint16_t tmp;
   float tmpC;



   // read adt7420 id register to verify device existence
   // ack = adt7420_p->read_dev_reg_byte(DEV_ADDR, 0x0b, &id);

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   uart.disp("read ADT7420 id (should be 0xcb): ");
   uart.disp(bytes[0], 16);
   uart.disp("\n\r");
   //debug("ADT check ack/id: ", ack, bytes[0]);
   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }
   

   uart.disp(tmpC);
   uart.disp("\n\r");


   return tmpC; 
}


GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));

enum class MenuState {
    START,
    LIVE,
    RESET,
    EXIT
};
MenuState check_btn(DebounceCore *db_p, GpoCore *led_p, bool &S, bool &L, bool &R){
    int s; 
    s = db_p->read(); 
    MenuState c_menu; 
    if (s == 1){
        S = 1; 
        L =0; 
        R = 0; 

        c_menu = MenuState::START; 

    }
    else if(s==16){
        L = 1; 
        S = 0; 
        R = 0; 

        c_menu = MenuState::LIVE; 
        
    }
    else if(s==4){
        R = 1; 
        S = 0; 
        L = 0; 

        c_menu = MenuState::RESET;

    
    }
    else{

    }
    return c_menu;

}

void check_btn_r(DebounceCore *db_p, bool &A, bool &Z){
    int s; 
    s = db_p->read(); 

    if (s == 2){
        
        A = true; 

    }
    else if (s == 8){
        Z = true; 
    }
    //A = false; 
}
void sw_led(GpiCore *sw, GpoCore *led){
    int swi; 
    swi = sw->read();
    led->write(swi); 

}
void sseg_clear(SsegCore *sseg){
    sseg->write_1ptn(0xFF,0);
    sseg->write_1ptn(0xFF,1);
    sseg->write_1ptn(0xFF,2);
    sseg->write_1ptn(0xFF,3);
    sseg->write_1ptn(0xFF,4);
    sseg->write_1ptn(0xFF,5);
    sseg->write_1ptn(0xFF,6);
    sseg->write_1ptn(0xFF,7);
    sseg->write_1ptn(0xFF,8);
}

float measure_avg_tmp(){
      int j; 
      float total_tmp = 0; 
      float avg_tmp; 
      for(int j = 0; j<30; j++){
      total_tmp += adt7420_read(&adt7420);
      }
      avg_tmp = total_tmp / 30; 

      uart.disp("\n\r"); 
      uart.disp("total temp: "); 
      uart.disp(total_tmp);
      uart.disp("\n\r");
      uart.disp("avg_tmp: "); 
      uart.disp(avg_tmp);
      uart.disp("\n\r");
      sleep_ms(1000); 
      return avg_tmp; 

}
void led_animation(GpoCore *led){
    for (int i = 0; i<4; i++){
        led->write(1,i); 
        sleep_ms(100); 
    }
        for (int i = 3; i>-1; i--){
        led->write(0,i); 
        sleep_ms(100); 
    }

}
void led_animation_rst(GpoCore *led){
    for (int i = 0; i<4; i++){
        led->write(1,i); 
    }
    sleep_ms(1000); 
        for (int i = 3; i>-1; i--){
        led->write(0,i); 

    }

}

int main() {
   bool S = false;
   bool L = false; 
   bool R = false; 
   bool A = false; 
   bool Z = false; 
   float storage; 

  

   

   while (1) {
       
       
       MenuState currentState = check_btn(&btn,&led,S,L,R);
       switch (currentState) {
            case MenuState::START:
            disp_RST(&sseg); 
            led_animation_rst(&led); 
            sseg_clear(&sseg); 
            storage = 0;  
            Z =0; 
            led.write(0,0); 
            led.write(0,1); 
            led.write(0,2); 
            led.write(0,3);
                //sseg.write_1ptn(sseg.h2s(), int pos)

                check_btn(&btn,&led,S,L,R);

                break;

            case MenuState::LIVE:
                sseg_clear(&sseg); 
                disp_LIVE(&sseg);
                sleep_ms(2000); 
                while(L == 1){
                    adt7420_check(&adt7420, &led, &sseg, &sw);
                    check_btn(&btn,&led,S,L,R);
                    if(L==0){
                        sseg_clear(&sseg);
                        break; 
                    }
                  
                }
                
                break;

            case MenuState::RESET:
                disp_AVG(&sseg); 
                while(R==1){
                    check_btn_r(&btn, A,Z); 
                    if(A){
                    sseg_clear(&sseg); 
                    disp_AVG(&sseg); 
                    led_animation(&led); 
                    sleep_ms(1000); 
                    sseg_clear(&sseg); 
                    
                    float tmp = measure_avg_tmp(); 
                    temp_disp_C(tmp, &sseg);
                    storage= tmp; 
                 
                    sleep_ms(500);
                    A= false; 
                    }
                  
                    //sseg_clear(&sseg);
                    if(Z){
                        sseg_clear(&sseg);
                        disp_DIFF(&sseg);
                        sleep_ms(2000); 
                        sseg_clear(&sseg);
                    }
                    while(Z){
                        float current =  adt7420_read(&adt7420); 
                
                        float difference = storage - current;
                        uart.disp("stored : ");
                        uart.disp(storage); 
                        uart.disp("\n\r");
                         uart.disp("current : ");
                        uart.disp(current); 
                        uart.disp("\n\r");
                        uart.disp("difference : ");
                        uart.disp(difference); 
                        uart.disp("\n\r");
                        temp_diff(difference, &sseg, &pwm);                    
                        sleep_ms(500);
                        //Z = false; 
                        check_btn(&btn,&led,S,L,R);
                        if(S==1||L==1){
                            pwm.set_duty(0,0); 
                            pwm.set_duty(0,1); 
                            pwm.set_duty(0,2); 
                            Z =false;
                        }
                        
                    }
                    
                     
                  
                    check_btn(&btn,&led,S,L,R);
                }
                if(R==0){
                    sseg_clear(&sseg); 
                    break; 
                }

                break;

            default:

                break;
        }

  


      }


   } //while
 //main

