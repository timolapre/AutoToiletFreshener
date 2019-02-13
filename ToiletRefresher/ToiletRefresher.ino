//Include libraries
#include <LiquidCrystal.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

//LCD init
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

//Temperature sensor init
#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

//Main variables
int State = 0;              //Current state
int SprayingDelay = 3;      //The delay of the spray
int SpraysLeft = 2400;      //The amount of sprays left

//Setting variables
int settings = 0;           //0 = not in settings mode. >0 = in settings mode.
int settingstimer = 0;      //Auto turn off settings when button is not pressed for x seconds

//Buttons
const int ButtonPin1 = 8, ButtonPin2 = 9, ButtonPin3 = 10;
bool Button1Pressed = false, Button2Pressed = false, Button3Pressed = false;

//motion sensor
const int MotionPin = 11;

//Motor
const int MotorPin = 13;

//LCD Text variables
String lcdtext1 = "", lcdtext2 = "";

//Timer
int temptimer = 2000;
int timer = 0; //timer in milis()


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Setup function
void setup() {
  Serial.begin(9600);
  sensors.begin();
  pinMode(ButtonPin1, INPUT);
  pinMode(ButtonPin2, INPUT);
  pinMode(ButtonPin3, INPUT);
  lcd.begin(16,2);

  //pinMode(MotionPin, INPUT);
  //pinMode(MotionLight, OUTPUT);
  pinMode(MotorPin, OUTPUT);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Loop Function
void loop() {
  if(ButtonPressed(1)){
    if(settings == 0) settings = 1;
    else settings = settings%3+1;
    settingstimer = 0;
  }
  
  if(settings > 0) Settings();
  else NormalMode();

}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When a number1 is detected (pee)
void Number1(){
  State = 2;
  ShowMessage("Peeing",0);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When a number2 is detected (poo)
void Number2(){
  State = 3;
  ShowMessage("Pooing",0);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When the toilet is being cleaned or anything else which is not a nmr1 nor nmr2
void CleaningToilet(){
  State = 4;
  ShowMessage("Cleaning Toilet",0);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When nothing is happening
void NormalMode(){
  ShowMessage(GetState(),0);
  GetTemperature();

  if(MotionDetected()){
    State = 1;
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When in settings mode
void Settings(){
  State = 6;
  Serial.println(settings);
  settingstimer++;
  
  if(settingstimer > 3000){
    settings = 0;
    ShowMessage(" ",1);
  }

  if(settings == 1){
    ShowMessage("Reset Spray-shots",0);
    ShowMessage(String(SpraysLeft),1);
    
    if(ButtonPressed(2)){
      SpraysLeft = 2400;
    }
  }
  else if(settings == 2){
    ShowMessage("Change Spraying delay",0);
    ShowMessage(String(SprayingDelay), 1);

    if(ButtonPressed(2)){
      SprayingDelay = (SprayingDelay+1)%10;
    }
  }
    
  else if(settings == 3){
    ShowMessage("Settings3", 0);
    ShowMessage(" ",1);

    if(ButtonPressed(3)){
      //do something
    }
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Function to spray x times
void Spray(int amount){
  if(amount == 1){
    
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Get the temperature of the room
float GetTemperature(){
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  Serial.println(temperature);
  ShowMessage(String(temperature),1);
  return temperature;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Check if motion is detected
bool MotionDetected(){
  int pirvalue = digitalRead(MotionPin);
  if(pirvalue == 1) return true;
  return false;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Function to show message on the LCD
void ShowMessage(String x, int line){ //Show message on the LCD
  bool reset = false;
  if(line == 0 && lcdtext1 != x){
    lcdtext1 = x;
    reset = true;
  }
  if(line == 1 && lcdtext2 != x){
    lcdtext2 = x;
    reset = true;
  }
  if(reset){
    lcd.clear();
    lcd.print(lcdtext1);
    lcd.setCursor(0,1);
    lcd.print(lcdtext2);
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Check if a certain button is pressed (there are 3 buttons)
bool ButtonPressed(int button){
  if(button == 1){
    if(digitalRead(ButtonPin1) == HIGH){
      Button1Pressed = true;
    }
    if(Button1Pressed && digitalRead(ButtonPin1) == LOW){
      Button1Pressed = false;
      return true;
    }
  }
  else if(button == 2){
    if(digitalRead(ButtonPin2) == HIGH){
      Button2Pressed = true;
    }
    if(Button2Pressed && digitalRead(ButtonPin2) == LOW){
      Button2Pressed = false;
      return true;
    }
  }
  else if(button == 3){
    if(digitalRead(ButtonPin3) == HIGH){
      Button3Pressed = true;
    }
    if(Button3Pressed && digitalRead(ButtonPin3) == LOW){
      Button3Pressed = false;
      return true;
    }
  }
  else{
    return false;
  }
  return false;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//GetState
String GetState(){ //return current state
  if(State == 0) return "not in use";
  else if(State == 1) return "in use - unkown";
  else if(State == 2) return "in use - pee";
  else if(State == 3) return "in use - poo";
  else if(State == 4) return "in use - cleaning";
  else if(State == 5) return "triggered";
  else if(State == 6) return "operator menu active";
  else return "Something went wrong";
}
