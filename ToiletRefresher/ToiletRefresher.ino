//Include libraries
#include <LiquidCrystal.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

//LCD init
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

//Temperature sensor init
#define ONE_WIRE_BUS A5 //A5 is the temperature pin
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

//Magnetic sensor
const int MagnetPin = 12;

//Distance sensor
const int DistanceEchoPin, DistanceTrigPin;

//Light sensor
const int LightPin = A1;

//RGB light
const int RedPin = A2, GreenPin = A3, BluePin = A4;

//LCD Text variables
String lcdtext1 = "", lcdtext2 = "";

//Timers
unsigned long temptimer = 0; //timer for temperature
unsigned long timer = 0; //timer in millis() after motion detected
unsigned long SprayTimer; //store millis() into this variable
unsigned long notsuretimer = 0;
unsigned long LastMovementTimer = 0;
unsigned long DistanceResetTimer = 0;

//PossibleActions array
bool PossibleActions[3] = {true, true, true}; //in order: Pee, Poo, Cleaning (or other)

//Other
const float MaxPeeTime = 120; //Max pee time in seconds. If it takes longer than x seconds it's not peeing anymore
const float NoMovementActivateTime = 60; //If there is x seconds no movement then activate something
float NormalDistance;
int PeeSprayCount = 1;
int PooSprayCount = 2;
bool SeatWasClosed = false;


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Setup function
void setup() {
  //Begin some stuff
  Serial.begin(9600);
  sensors.begin();
  lcd.begin(16,2);

  //pinModes
  pinMode(ButtonPin1, INPUT);
  pinMode(ButtonPin2, INPUT);
  pinMode(ButtonPin3, INPUT);
  
  pinMode(MotionPin, INPUT);
  
  pinMode(MotorPin, OUTPUT);

  pinMode(MagnetPin, INPUT);

  pinMode(DistanceEchoPin, INPUT);
  pinMode(DistanceTrigPin, OUTPUT);

  pinMode(LightPin, INPUT);

  pinMode(RedPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);

  //Other
  MaxPeeTime += NoMovementActivateTime;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Loop Function
void loop() {

  Serial.print(PossibleActions[0]);
  Serial.print(PossibleActions[1]);
  Serial.print(PossibleActions[2]);
  Serial.println(" " + String(millis()/1000-notsuretimer) + "  " + String(NoMovement()));
  
  // -- Change State based on inputs
  //SettingsMode
  if(ButtonPressed(1)){
    State = 6;
    settingstimer = millis()/1000;
    if(settings == 0) settings = 1;
    else{
      settings = settings%4+1; //increase x in %x to add another setting (and add it in settings function)
    }
    Reset();
  }

  if(ButtonPressed(3)){
    State = 5;
  }

  if(MotionDetected()){
    LastMovementTimer = millis()/1000;
  }

    
  if(millis()/1000-DistanceResetTimer > 5){
    NormalDistance = GetDistance();
    DistanceResetTimer = millis()/1000;
  }

  //temperature updater (every 1000ms)
  if(millis()/1000-temptimer >= 1){
    GetTemperature();
    temptimer = millis()/1000;
  }

  // -- Loop based on State
  if(State == 0){
    NormalMode();
  }
  else if(State == 1){
    NotSure();
  }
  else if(State == 2){
    Number1();
  }
  else if(State == 3){
    Number2();
  }
  else if(State == 4){
    CleaningToilet();
  }
  else if(State == 5){
    Trigger();
  }
  else if(State == 6){
    Settings();
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When nothing is happening
void NormalMode(){
  SetColorLight(255,0,0);
  String mystate = GetState();
  ShowMessage(mystate, 0);

  //Check if motion is detected and change to active
  if(MotionDetected()){
    State = 1;
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//in use but not sure what person is doing
void NotSure(){
  SetColorLight(255,255,255);
  String mystate = GetState();
  ShowMessage(mystate, 0);
  //ShowMessage(" ", 1);
  
  if(notsuretimer == 0) notsuretimer = millis()/1000;
  if(millis()/1000-notsuretimer > MaxPeeTime){
    PossibleActions[0] = false;
  }

  if(GetMagneticState() == false) SeatWasClosed = true;

  if(GetMagneticState() && SeatWasClosed){
    PossibleActions[1] = false;
  }

  if(NoMovement() > NoMovementActivateTime){
    PossibleActions[2] = false;
    if(PossibleActions[0]) PossibleActions[1] = false;
  }

  for(int i = 0; i < 3; i++){
    if(PossibleActions[i] == true && PossibleActions[(i+1)%3] == false && PossibleActions[(i+2)%3] == false){
      State = i+2;
    }
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When a number1 is detected (pee)
void Number1(){
  SetColorLight(0,255,0);
  String mystate = GetState();
  ShowMessage(mystate, 0);
  //ShowMessage(" ", 1);
  
  if(NoMovement() > NoMovementActivateTime){
    State = 5;
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When a number2 is detected (poo)
void Number2(){
  SetColorLight(0,0,255);
  String mystate = GetState();
  ShowMessage(mystate, 0);
  //ShowMessage(" ", 1);

  if(NoMovement() > NoMovementActivateTime){
    State = 5;
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When the toilet is being cleaned or anything else which is not a nmr1 nor nmr2
void CleaningToilet(){
  SetColorLight(255,255,0);
  String mystate = GetState();
  ShowMessage(mystate, 0);
  //ShowMessage(" ", 1);

  if(NoMovement() > NoMovementActivateTime){
    State = 5;
  }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When Spray should be triggered
void Trigger(){
  SetColorLight(255,0,255);
  String mystate = GetState();
  ShowMessage(mystate, 0);
  //ShowMessage(" ", 1);

  if(PossibleActions[0] == true){
    Spray(1);
  }
  else if(PossibleActions[1] == true){
    Spray(2);
  }
  else if(PossibleActions[2] == true){
    Spray(0);
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//When in settings mode
void Settings(){
  SetColorLight(0,255,255);
  
  //Make sure motor doesn't accidentally get triggered
  digitalWrite(MotorPin, LOW);
  SprayTimer = 0;

  //Change back to normal mode after 5 seconds of not pressing any settings button
  if(millis()/1000-settingstimer > 5){
    settings = 0;
    settingstimer = 0;
    Reset();
  }

  if(settings == 1){
    ShowMessage("Reset Spray-shots",0);
    ShowMessage(String(SpraysLeft),1);
    
    if(ButtonPressed(2)){
      SpraysLeft = 2400;
      settingstimer = millis()/1000;
    }
  }
  else if(settings == 2){
    ShowMessage("Spraying delay",0);
    ShowMessage(String(SprayingDelay+15) + " seconds", 1);

    if(ButtonPressed(2)){
      SprayingDelay = (SprayingDelay+1)%10;
      settingstimer = millis()/1000;
    }
  }

  //if you want to add more settings increase it in the loop
  else if(settings == 3){
    ShowMessage("Pee Spray Count", 0);
    ShowMessage(String(PeeSprayCount),1);

    if(ButtonPressed(2)){
      PeeSprayCount = (PeeSprayCount+1)%4;
      settingstimer = millis()/1000;
    }
  }

  else if(settings == 4){
    ShowMessage("Poo Spray Count", 0);
    ShowMessage(String(PooSprayCount),1);

    if(ButtonPressed(2)){
      PooSprayCount = (PooSprayCount+1)%4;
      settingstimer = millis()/1000;
    }
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Function to spray x times
void Spray(int amount){
  if(SprayTimer == 0) SprayTimer = millis()/1000;
  if(millis()/1000 > SprayTimer + SprayingDelay){
    digitalWrite(MotorPin, HIGH);
  }
  if(millis()/1000 > SprayTimer + 18*amount +  SprayingDelay){
    digitalWrite(MotorPin, LOW);
    SpraysLeft -= amount;
    Reset();
  }
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Get the temperature of the room
float GetTemperature(){
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  ShowMessage(String(temperature) + char(223) +"C  " + String(SpraysLeft),1);
  return temperature;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Check if motion is detected
bool MotionDetected(){ 
  int pirvalue = digitalRead(MotionPin);
  if(pirvalue == HIGH) return true;
  return false;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Get distance
float GetDistance(){
  digitalWrite(DistanceTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(DistanceTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(DistanceTrigPin, LOW);
  float duration = pulseIn(DistanceEchoPin, HIGH);
  float distance = duration*0.034/2;
  return distance;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Is light on or off
bool GetLight(){
  if(digitalRead(LightPin) == HIGH){
    return true;
  }
  return false;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//How much light
float GetLightValue(){
  return analogRead(LightPin);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Door or Toilet seat open or closed. true is open. false is closed
bool GetMagneticState(){
  if(digitalRead(MagnetPin) == HIGH){
    return true;
  }
  return false;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Set color of the RGB light
void SetColorLight(int R, int G, int B){
  analogWrite(RedPin, R);
  analogWrite(GreenPin, G);
  analogWrite(BluePin, B);
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Return time in seconds without movement in the toilet
unsigned long NoMovement(){
  return millis()/1000-LastMovementTimer;
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
//Reset Function
void Reset(){
  SeatWasClosed = false;

  for(int i=0; i < 3; i++){
    PossibleActions[i] = true;
  }

  notsuretimer = 0;
  SprayTimer = 0;
  State = 0;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Return current state as String
String GetState(){
  if(State == 0) return "not in use";
  else if(State == 1) return "in use-unkown";
  else if(State == 2) return "in use-pee";
  else if(State == 3) return "in use-poo";
  else if(State == 4) return "in use-cleaning";
  else if(State == 5) return "triggered";
  else if(State == 6) return "Settings";
  else return "Something went wrong";
}
