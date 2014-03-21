/*
 *Uses the following display driver 
 *Adafruit i2c/SPI LCD backpack using MCP23008 I2C expander
 ( http://learn.adafruit.com/i2c-spi-lcd-backpack )
 * CLK to Analog #5
 * DAT to Analog #4
 */

// include the library code:
#include <Wire.h>
#include <LiquidTWI2.h>

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI2 lcd(0);

// defines for pins
#define Track_A_Detector 0      
#define Track_B_Detector 1      
#define Track_A_LED 9      
#define Track_B_LED 5
#define Buzzer 10
#define ResetButton 4
#define StartButton 8
#define IR_Mosfet 6

// define contstants
#define A_Threshold 80
#define B_Threshold 80
#define Timeout 10000 //10 seconds to complete the race.

// Define variables
int Winner = 0;
boolean RaceRunning = false;
unsigned long StartTime;
unsigned long CurrentTime;
unsigned long ElapsedA;
unsigned long ElapsedB;
char* timestrA;
char* timestrB;
char* timestrC;
char bufferA[20];
char bufferB[20];
char bufferC[20];
char Tripped;

void setup() {
  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008); 
  // set up the LCD's number of rows and columns:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("HotWheel Timing Track");
  lcd.setBacklight(HIGH);
  // Set pinmodes
  pinMode(Track_A_LED,OUTPUT);
  pinMode(Track_B_LED,OUTPUT);
  pinMode(Track_A_Detector, INPUT);
  pinMode(Track_B_Detector, INPUT);
  pinMode(ResetButton, INPUT);
  pinMode(StartButton, INPUT);
  pinMode(IR_Mosfet, OUTPUT);
  pinMode(Buzzer,OUTPUT);
  Serial.begin(9600);
  // Show starting LED animation
  digitalWrite(Track_A_LED,HIGH);
  digitalWrite(Track_B_LED,HIGH);
  delay(2000);
  digitalWrite(Track_A_LED,LOW);
  digitalWrite(Track_B_LED,LOW);
  digitalWrite(IR_Mosfet, LOW);
  WaitForStartingBlock();
  DisplayHeader();
}


void loop()
{
  if (digitalRead(StartButton))  //Check to see if starting block is up
  { 
    digitalWrite(IR_Mosfet, HIGH);
    StartTime = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    //        "0123456789012345"
    lcd.print("  Race Running  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    Winner = 0;
    Tripped = '0';
    RaceRunning = true;
    ElapsedA = 0;
    ElapsedB = 0;
    while (RaceRunning) 
    {
      CurrentTime = millis();
      if (CurrentTime >= StartTime + Timeout)
      {
        //Timeout exceeded.
        RaceRunning=false;
        lcd.clear();
        lcd.setCursor(0, 0);
        //        "0123456789012345"
        lcd.print("   Timeout!     ");
        lcd.setCursor(0, 1);
        lcd.print("  Press Reset   ");
      }
      if (not digitalRead(ResetButton))
      {
        //Reset button used to abort the race
        RaceRunning=false;
      }
      if ((Winner == 0) && RaceRunning)
      {
        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        timestrC = dtostrf((CurrentTime-StartTime)/1000.00,6,3,bufferC); 
        lcd.print(timestrC);
        Tripped=(Is_Finish_Tripped());
        if (Tripped=='T')
        {
          Winner=3;
          RaceRunning=false;
          ElapsedA = CurrentTime - StartTime;
          ElapsedB = CurrentTime - StartTime;        
          lcd.clear();
          PrintResultsA();
          PrintResultsB();
        }
        else if (Tripped=='A')
        {
          Winner=1;
          ElapsedA = CurrentTime - StartTime;
          lcd.clear();
          PrintResultsA();
        }
        else if (Tripped=='B')
        {
          Winner=2;
          ElapsedB = CurrentTime - StartTime;
          lcd.clear();
          PrintResultsB();
        }        
        FinishLights(Winner);
      }  //end if (Winner == 0)
      else if (Winner == 1)
      { 
        Tripped=(Is_Finish_Tripped());
        if (Tripped=='B')
        {
          RaceRunning=false;
          ElapsedB = CurrentTime - StartTime;        
          PrintResultsB();
        }
      }  // end else if (Winner == 1)
      else if (Winner == 2)
      {
        Tripped=(Is_Finish_Tripped());
        if (Tripped=='A')
        {
          RaceRunning=false;
          ElapsedA = CurrentTime - StartTime;        
          PrintResultsA();
        }
      }  // end else if (Winner == 2)
    }  //end while (RaceRunning) 
    digitalWrite(IR_Mosfet, LOW);
    WaitForReset();
    WaitForStartingBlock();
    DisplayHeader();
  }  //end if (digitalRead(StartButton))
}  //end loop


void FinishLights(int Winner)
{
  switch (Winner)
  {
  case 1:
    digitalWrite(Track_A_LED,HIGH);
    break;
  case 2:
    digitalWrite(Track_B_LED,HIGH);
    break;
  case 3:
    digitalWrite(Track_A_LED,HIGH);
    digitalWrite(Track_B_LED,HIGH);
    break;
  }    
}

char Is_Finish_Tripped()
{
  //return '0' if neither sensor is tripped.
  //return 'T' if both sensors are tripped at the same time  ie. a tie.
  //return 'A' if track A sensor is tripped.
  //return 'B' if track B sensor is tripped.
  int IR_val_B;
  int IR_val_A;
  boolean A_Tripped=false;
  boolean B_Tripped=false;
  IR_val_A = analogRead(Track_A_Detector);
  IR_val_B = analogRead(Track_B_Detector);
  if ( IR_val_A < A_Threshold )
  {
    //Serial.println(IR_val_A);             // debug value
    A_Tripped=true;
  }
  if ( IR_val_B < B_Threshold )
  {
    //Serial.println(IR_val_B);             // debug value
    B_Tripped=true;
  }
  if (A_Tripped && B_Tripped)
  {
    return 'T';
  }
  else if (A_Tripped)
  {
    return 'A';
  }
  else if (B_Tripped)
  {
    return 'B';
  }
  else
  {
    return '0';
  }
}

void DisplayHeader()
{
  // set the cursor to column 0, line 0
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.clear();
  lcd.setCursor(0, 0);
  //         0123456789012345
  lcd.print(" Ready to Race! ");
}

void PrintResultsA()
{
  // set the cursor to column 0, line 0
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  lcd.print("Time A: ");
  timestrA = dtostrf(ElapsedA/1000.00,6,3,bufferA); 
  lcd.print(timestrA);
  //Serial.println(timestrA);
}

void PrintResultsB()
{
  // set the cursor to column 0, line 0
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print("Time B: ");
  timestrB = dtostrf(ElapsedB/1000.00,6,3,bufferB); 
  lcd.print(timestrB);
  //Serial.println(timestrB);
}

void WaitForReset()
{
  //wait for reset button to be pressed
  while (digitalRead(ResetButton)) 
  {
    delay(100);
  }
  lcd.clear();
  // Turn off LEDs
  digitalWrite(Track_A_LED,LOW);
  digitalWrite(Track_B_LED,LOW);
}

void WaitForStartingBlock()
{
  // Check to see the starting Block is in place
  // If not prompt players insert block
  if (digitalRead(StartButton))
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    //        "0123456789012345"
    lcd.print(" Please Replace ");
    lcd.setCursor(0, 1);
    lcd.print(" Starting Block ");
    while (digitalRead(StartButton))
    {
      delay(100);    
    }
    lcd.clear();
    lcd.setCursor(0, 0);
  }
}


