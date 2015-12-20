//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
int CLICK_DELAY = 200; // TODO JB: Let this be adjusted dynamic as the user is holding down a key.
int currentIndex = -1;
const int empNumbers[] = {111, 220, 249, 250, 279, 350, 365, 549};//
const String names[] = {"Gjerry", "Voldern", "Jokio", "Henry", "Amund", "Pavio", "Maggie", "Nilia"};
const int nrOfEmployees = 8;//sizeof(names);
String CLEAR_LINE = "                ";
bool chosenAlready = false;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons() {
 adc_key_in = analogRead(0); // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
/* if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  
*/
 // For V1.0 comment the other threshold and use the one below:

 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   


 return btnNONE;  // when all others fail, return this...
}

void setup()
{
 lcd.begin(16, 2); // start the library
 lcd.setCursor(0,0);
 lcd.print("Velg ansattnr  "); // print a simple message
 lcd.setCursor(0,1);
 lcd.print("med opp&ned-pil ");
}
 
void loop()
{
 lcd.setCursor(9,1); // move cursor to second line "1" and 9 spaces over
 //lcd.print(millis()/1000); // display seconds elapsed since power-up

 lcd.setCursor(0,0); // move to the begining of the second line
 lcd_key = read_LCD_buttons(); // read the buttons

 switch (lcd_key) // depending on which button was pushed, perform an action
 {
   /*case btnRIGHT:
     {
     lcd.print("RIGHT ");
     break;
     }
   case btnLEFT:
     {
     lcd.print("LEFT   ");
     break;
     } */
   case btnUP:
     {
     lcd.print("Velg ansattnr:  ");
     if(currentIndex >= nrOfEmployees-1) {
       currentIndex = 0;
     } else {
       currentIndex++;
     }
     delay(CLICK_DELAY);
     lcd.setCursor(0,1); 
     lcd.print(CLEAR_LINE);
     lcd.setCursor(0,1); 
     lcd.print(empNumbers[currentIndex]);
     chosenAlready = false;
     break;
     }
   case btnDOWN:
     {
     lcd.print("Velg ansattnr:  ");
     if(currentIndex <= 0) {
       currentIndex = nrOfEmployees-1;
     } else {
       currentIndex--;
     }
     delay(CLICK_DELAY);
     lcd.setCursor(0,1);
     lcd.print(CLEAR_LINE);
     lcd.setCursor(0,1);
     lcd.print(empNumbers[currentIndex]);
     chosenAlready = false;
     break;
     }
   case btnSELECT:
     {
     // Print name of selected currentIndex...
     if(chosenAlready) {
       lcd.setCursor(0,0);
       lcd.print("Device lagt til!");
       lcd.setCursor(0,1);
       lcd.print("Scan 2 pay!     ");
       chosenAlready = false;
     } else {
       // let user confirm
       lcd.setCursor(0,0);
       lcd.print("Er du           ");
       lcd.setCursor(0,1);
       lcd.print(CLEAR_LINE);
       lcd.setCursor(0,1);
       lcd.print(String(names[currentIndex]) + "?");
       chosenAlready = true;
     }
     delay(500);
     break;
     }
     /*case btnNONE:
     {
     lcd.print("NONE  ");
     break;
     }*/
 }
}
