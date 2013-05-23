/*
  Arduino Esplora educational data logger and data viewer.
  Logs data into a microSD and "plot" it into the LCD.
   
  Written by Angelo Scialabba 
  <a.scialabba@arduino.cc>
  11/04/2013
*/
#include <Esplora.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <TFT.h>            // Arduino LCD library
#include <SPI.h>
#include <SD.h>

#define SD_CS 8
#define MENU 0
#define RECORD 1
#define DISPLAY 2
#define GRAPH 3

int st;
int STORE_DELAY = 1000;
void setup(){

  // initialize the screen
  EsploraTFT.begin();

  // clear the screen
  EsploraTFT.background(0,0,0); 
  st = MENU; //Select menu for default option
  
  // ---------Splash Screen---------
  EsploraTFT.stroke(0,255,0);
  EsploraTFT.setTextSize(4);
  EsploraTFT.text("Esplora",0,25); 
  EsploraTFT.stroke(255,255,255);
  EsploraTFT.setTextSize(1); 
  EsploraTFT.text("Educational datalogger",0,60);
  delay(3000);
  //-----------------------------
}

void loop(){
  //Read the option selected execute the right function
  switch(st){
    case MENU:
    menu();   // Main menu
    break;
    case RECORD:
    record(); // Record data menu
    break;
    case DISPLAY:
    display(); // Display the sensor data
    break;
    case GRAPH:
    graph(); // Display the graph of stored data
    break;
   
  }
  
 
}

void menu(){
  
  EsploraTFT.background(0,0,0);
  EsploraTFT.stroke(255,255,255);
  EsploraTFT.setTextSize(1);
  
  EsploraTFT.text("Select an option:\n1.Record data\n2.Diplay data\n3.Graph\n",0,25);
  delay(500);
  while(1) // infinite loop until a button is pressed
  {
    if (Esplora.readButton(SWITCH_1) == LOW) {
      st = RECORD; // set new option
      return; // the "while" will break and the loop function will execute the new option
    } else if (Esplora.readButton(SWITCH_2) == LOW) {
      st = DISPLAY; // same here
      return; 
    } else if (Esplora.readButton(SWITCH_3) == LOW) {
      st = GRAPH; // same here
      return;
    }
  }
}

void record() {
  
  int value;
  File dataFile; 
  SD.begin(SD_CS);    // init the SD library
  SD.remove("data.txt");   // delete old data file
  dataFile = SD.open("data.txt", FILE_WRITE);
  String data;
  EsploraTFT.background(0,0,0);  //clear the screen
  if (!dataFile) {
      EsploraTFT.stroke(255,255,255);                                 //+
      EsploraTFT.setTextSize(2);                                      //| Show error Message
      EsploraTFT.text("SD Error\n",0,0);                              //|    
      EsploraTFT.setTextSize(1);                                      //|
      EsploraTFT.text("Plug in your SD\nand restart Esplora.",0,30);  //+
      while(1){        
      }
  }  
  EsploraTFT.stroke(255,255,255);
  EsploraTFT.setTextSize(2);
  EsploraTFT.text("Recording\n",0,0);
  EsploraTFT.setTextSize(1);
  EsploraTFT.text("Hold Switch 1 to stop.",0,30); // Start recording
  delay(500); 
  while(1){ // infinite loop, will break if button 1 is pressed
    if (Esplora.readButton(SWITCH_1) == LOW) {  
      dataFile.close();  // Close file 
      st = MENU;    //and return to main menu
      return;
    }
    data = "";                           //+ 
    value = Esplora.readLightSensor();   //| Read Data from sensor - Change Esplora.readLightSensor()
    data += String(value);               //| and store it          - to change the input sensor
    data+= "\r\n";                       //| in the sd card
    dataFile.print(data);                //|  
    delay(STORE_DELAY);                  //+ 
  }
  
}

void display()
{
  int value;
  char tmp[5];
  String tmp_string;
  EsploraTFT.setTextSize(1);
  EsploraTFT.background(0,0,0); //Clear the screen
  EsploraTFT.text("Hold Switch 1 to stop.",0,30);
  while(1) {
    tmp_string=String(Esplora.readLightSensor()); //Read the sensor value
    tmp_string.toCharArray(tmp, 5);    //convert the value into a string
    EsploraTFT.stroke(255,255,255);    
    EsploraTFT.setTextSize(2); 
    EsploraTFT.text(tmp,0,0);         //write the string on the lcd  
    delay(500);
    EsploraTFT.stroke(0,0,0);
    EsploraTFT.text(tmp,0,0);         //erase the old value from the screen
    if (Esplora.readButton(SWITCH_1) == LOW) {
      st = MENU;    	 //return to main menu when button 1 is pressed
      return;
    }
  }
  
}


void graph(){
   int value,index,xPos;
   unsigned long i,points,div,position_index;
   File dataFile; 
   char tmp[6];
   SD.begin(SD_CS);
   dataFile = SD.open("data.txt", FILE_READ);  //open the data file
   String data;
   points = 0;
   xPos=0;
   EsploraTFT.background(0,0,0);  //Clear the screen
   if (!dataFile) {
       EsploraTFT.stroke(255,255,255);    //show an error message
       EsploraTFT.setTextSize(2);         //if the file does not exist
       EsploraTFT.text("SD Error\n",0,0); //or the sd card is not present
       EsploraTFT.setTextSize(1);
       EsploraTFT.text("Plug in your SD\nand restart Esplora.",0,30);
       while(1){        
       }
   }
   for (i=0;i<dataFile.size();i++){		
     if (dataFile.read()=='\n') points++;    // count how many values the file contains
   }
   position_index=0;
   div = points/EsploraTFT.width();            
   if (div == 0){           //points available on screen are more that stored ones
     div = EsploraTFT.width()/points;              //fit the data in the screen and read from file
     dataFile.seek(0);
     for (i = 0 ; i<points;i++){       
        for (index =  0; index<6; index++){
           tmp[index] = dataFile.read();
           if (tmp[index] =='\n'){
             tmp[index-1] = '\0';
             break;
           }
         }
         value = atoi(tmp);         //convert the text into a number
         value = map(value,0,1023,0,EsploraTFT.height());    //map the value
         EsploraTFT.line(i*div, EsploraTFT.height() - value, i*div, EsploraTFT.height()); //draw the value on lcd
       }
     } else {             //all stored data can't be drawed on screen
       dataFile.seek(0);
       for (i = 0 ; i<points;i++){         
         for (index =  0; index<6; index++){
           tmp[index] = dataFile.read();
           if (tmp[index] =='\n'){
             tmp[index-1] = '\0';
             position_index++;
             break;
           }
         }
         if ((position_index-1) % div == 0){      // select only few points 
           value = atoi(tmp);                                // convert 
           value = map(value,0,1023,0,EsploraTFT.height());  // and print values on LCD
           EsploraTFT.line(xPos, EsploraTFT.height() - value, xPos, EsploraTFT.height());
           xPos++;
         }
       }
       
     }
   while(1){
     if (Esplora.readButton(SWITCH_1) == LOW) {
      dataFile.close();   //when button 1 is pressed 
      st = MENU;          //go to main menu
      return;
    }
   }
     
}
