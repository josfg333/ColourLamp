/* Model of electronics and code to be used for the interactive
 * lamp for my 13DTE project.
 * Created by Joseph Grace 21/04/2020
*/

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include "menu.h"

#define PIXEL_NUM 8

const double TAU = 2*PI;

const int BAUD_RATE = 9600;

//Control buttons.
const int LEFT = 2;
const int UP = 4;
const int RIGHT = 3;
const int DOWN = 5;

//LED strip pin
const int STRIP_PIN = 6;

//Sensors
const int LDR = A0;
const int LDRSeriesResistor = 330;
const int PIR = A2;
const int MIC = A1;

//Initialise LCD and NeoPixel Objects
Adafruit_NeoPixel strip(16, STRIP_PIN, NEO_GRBW+NEO_KHZ800);

//Interacting Color Class
class InteractingColor{
public:
  InteractingColor(int hue){
    this->hue=hue;
  }
  float hue;
  int neighbor_num = 0;
  InteractingColor* neighbors[3];
  float neighbor_weights[3];
  void link(InteractingColor* neighbor, float weight);
  void setAcceleration(){
    hue_acceleration = 0;
    for(int i=0; i<neighbor_num; i++){
      float neighbor_hue = neighbors[i]->hue;
      hue_acceleration+=neighbor_weights[i]*sin(radians(neighbor_hue-hue));
    }
    hue_acceleration*=influence;
      
  }
  void doTick(){
    hue_velocity += hue_acceleration / tick_speed;
    hue += hue_velocity / tick_speed;
  }
private:
  float hue_acceleration;
  float hue_velocity = 0;//10*random(-1,2);
  int influence = 100;
  int tick = 0;
  int tick_speed = 60;
};

void InteractingColor::link(InteractingColor* neighbor, float weight){
    if(neighbor_num>2){return;}
    neighbors[neighbor_num] = neighbor;
    neighbor_weights[neighbor_num] = weight;
    neighbor_num++;
  }

//Driven pendulum class
class DrivenPendulum{
public:
   float friction = 0.01;
   float displacement;
   DrivenPendulum(float displacement){
      this->displacement = displacement;
   }
   void doTick(){
      float drive = force_amplitude * sin(TAU*tick/(tick_speed*force_period));
      float gravity_force = -g*sin(displacement);
      float acceleration = drive + gravity_force - friction*velocity;
      velocity+=acceleration/tick_speed;
      displacement+=velocity/tick_speed;
      tick++;
   }
private:
  float velocity= 0;
  float force_period = 3;
  float force_amplitude= 4;
  float g = 10;
  int tick = 0;
  int tick_speed= 80;//ticks per simulation second;
};

InteractingColor* pixels[PIXEL_NUM+1];

DrivenPendulum driver(random(0,1000)/1000.0-0.5);

const float sensitivity = 129533;
const float offset = 0.77;
//Sensor Functions
int readLDR(){
  //returns brightness on LDR, calibrated to
  //be between 0(dark) and 255(light)
  float resistance = LDRSeriesResistor*(1024.0/analogRead(LDR)-1.0);
  int brightness= round(sensitivity/resistance - offset);
  return min(max(0,brightness),255);
}

class State{
public:
  int pure_colour[3]={0,100,75};
  int global_brightness=100;
  int sound_sens=100;
  int motion_sens=100;
  int light_sens=100;
  int mode=RANDOM;
  int current=MODES;
  bool setting=false;
  void add(int *param, int num){
    *param=max(0,min(100,(*param)+num));
  }
}

state=State();

class LCDScreen: public LiquidCrystal{
public:
  LCDScreen(int rs_pin, int en_pin, int d1, int d2, int d3, int d4): LiquidCrystal(rs_pin, en_pin, d1, d2, d3, d4){
    begin(16,2);
    setCursor(0,0);
    updateState();
  }
  void writeArrow(String s, int line){
    int space = (14-int(s.length()))/2;
    if(space<0){space=0;}
    setCursor(0,line);
    print("<               ");
    setCursor(1+space, line);
    print(s);
    setCursor(15,line);
    print(">");
  }
  void writeCenter(String s, int line){
    int space = (16-int(s.length()))/2;
    if(space<0){space=0;}
    String filler="";
    for(int i=0; i<space; i++){
      filler+=" ";
    }
    setCursor(0,line);
    print("                ");
    setCursor(space, line);
    print(s);
  }
  void updateState(){
    if(state.setting){
      writeCenter(NAME_LIST[state.current],0);
      switch(state.current){
        case(BRIGHTNESS): writeArrow(String(state.global_brightness), 1); break;
        case(SOUND): writeArrow(String(state.sound_sens), 1); break;
        case(LIGHT): writeArrow(String(state.light_sens), 1); break;
        case(MOTION): writeArrow(String(state.motion_sens), 1); break;
        case(HUE): writeArrow(String(state.pure_colour[0]), 1); break;
        case(SATURATION): writeArrow(String(state.pure_colour[1]), 1); break;
        case(VALUE): writeArrow(String(state.pure_colour[2]), 1); break;
        default: state.current=false; break;
      }
    }else{
      writeArrow(NAME_LIST[state.current],1);
      switch(state.current){
        case(MODES): writeCenter("Menu",0); break;
        case(BRIGHTNESS): writeCenter("Menu",0); break;
        case(RANDOM): writeCenter(NAME_LIST[MODES],0); break;
        case(SOUND): writeCenter(NAME_LIST[RANDOM],0); break;
        case(LIGHT): writeCenter(NAME_LIST[RANDOM],0); break;
        case(MOTION): writeCenter(NAME_LIST[RANDOM],0); break;
        case(COLOUR): writeCenter(NAME_LIST[MODES],0); break;
        case(HUE): writeCenter(NAME_LIST[COLOUR],0); break;
        case(SATURATION): writeCenter(NAME_LIST[COLOUR],0); break;
        case(VALUE): writeCenter(NAME_LIST[COLOUR],0); break;
      }
    }
  }
};


LCDScreen lcd(7,8,9,10,11,12);

void setup()
{
  Serial.begin(BAUD_RATE);
  
  
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(STRIP_PIN, OUTPUT);
  pinMode(LDR,INPUT);
  pinMode(PIR,INPUT);
  strip.begin();
  strip.clear();
  strip.show();
  //create pixels
  for(int i=0; i<PIXEL_NUM+1; i++){
    pixels[i] = new InteractingColor(random(0,360));
  }
  //link pixels
  for(int i=0; i<PIXEL_NUM; i++){
    pixels[i]->link(pixels[(i+PIXEL_NUM-1)%PIXEL_NUM],0.42);
    pixels[i]->link(pixels[(i+1)%PIXEL_NUM],0.42);
    pixels[i]->link(pixels[PIXEL_NUM],0.16);
  }
}

void writePixelToSerial(uint32_t rgb_color){
    for(int i=0; i<3; i++){
      Serial.write(rgb_color & 255);
      rgb_color >>= 8;
    }
}

void writeAllToSerial(){
  for(int i=0; i<3; i++){
    Serial.write(0);
  }
  for(int i=0; i<PIXEL_NUM; i++){
    uint32_t rgb_color = strip.ColorHSV(round(pixels[i]->hue*65536/360));
    writePixelToSerial(rgb_color);
  }
}

long long milliseconds_since_last_press=0;

void on_button_press(int button){
  int time_delta;
  if(state.setting){
    if(state.current==HUE) time_delta=13;
    else time_delta=50;
  }
  else time_delta=200;
  if(millis()-milliseconds_since_last_press<time_delta){
    return;
  }
  milliseconds_since_last_press=millis();
  switch(state.current){
    case MODES:
      switch(button){
        case LEFT: state.current=BRIGHTNESS;break;
        case RIGHT: state.current=BRIGHTNESS;break;
        case DOWN: state.current=state.mode;
      }
      break;
    case BRIGHTNESS:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.global_brightness,-1);
          }else{
            state.current=MODES;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.global_brightness,1);
          }else{
            state.current=MODES;
          }
          break;
        case DOWN: state.setting=true;break;
        case UP: state.setting=false;break;
      }
      break;
    case RANDOM:
      switch(button){
        case LEFT: state.current=COLOUR; break;
        case RIGHT: state.current=COLOUR; break;
        case DOWN: state.mode=RANDOM;state.current=SOUND; break;
        case UP: state.current=MODES; break;
      }
      break;
    case COLOUR:
      switch(button){
        case LEFT: state.current=RANDOM; break;
        case RIGHT: state.current=RANDOM; break;
        case DOWN: state.mode=COLOUR; state.current=HUE; break;
        case UP: state.current=MODES; break;
      }
      break;
    case SOUND:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.sound_sens,-1);
          }else{
            state.current=MOTION;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.sound_sens,1);
          }else{
            state.current=LIGHT;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?SOUND:RANDOM; state.setting=false; break;
      }
      break;
    case LIGHT:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.light_sens,-1);
          }else{
            state.current=SOUND;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.light_sens,1);
          }else{
            state.current=MOTION;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?LIGHT:RANDOM; state.setting=false; break;
      }
      break;
    case MOTION:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.motion_sens,-1);
          }else{
            state.current=LIGHT;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.motion_sens,1);
          }else{
            state.current=SOUND;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?MOTION:RANDOM; state.setting=false; break;
      }
      break;
    case HUE:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.pure_colour[0]=(state.pure_colour[0]+359)%360;
          }else{
            state.current=VALUE;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.pure_colour[0]=(state.pure_colour[0]+361)%360;
          }else{
            state.current=SATURATION;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?HUE:COLOUR; state.setting=false; break;
      }
      break;
    case SATURATION:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.pure_colour[1],-1);
          }else{
            state.current=HUE;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.pure_colour[1],1);
          }else{
            state.current=VALUE;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?SATURATION:COLOUR; state.setting=false; break;
      }
      break;
    case VALUE:
      switch(button){
        case LEFT: 
          if(state.setting){
            state.add(&state.pure_colour[2],-1);
          }else{
            state.current=SATURATION;
          }
          break;
        case RIGHT:
          if(state.setting){
            state.add(&state.pure_colour[2],1);
          }else{
            state.current=HUE;
          }
          break;
        case DOWN: state.setting=true; break;
        case UP: state.current=state.setting?VALUE:COLOUR; state.setting=false; break;
      }
      break;
  }
  lcd.updateState();
}

void loop()
{
  driver.doTick();
  pixels[PIXEL_NUM]->hue = degrees(driver.displacement);
  for(int i=0; i<PIXEL_NUM; i++){
    pixels[i]->setAcceleration();
  }
  for(int i=0; i<PIXEL_NUM; i++){
    pixels[i]->doTick();
    uint32_t rgb_color = strip.ColorHSV(round(pixels[i]->hue*65536/360),255,200);
    strip.setPixelColor(i*2,strip.gamma32(rgb_color));
  }
  //writeAllToSerial();
  strip.show();
  if(!digitalRead(LEFT)){
    on_button_press(LEFT);
  }else if(!digitalRead(UP)){
    on_button_press(UP);
  }else if(!digitalRead(DOWN)){
    on_button_press(DOWN);
  }else if(!digitalRead(RIGHT)){
    on_button_press(RIGHT);
  }
  delay(50);
}
