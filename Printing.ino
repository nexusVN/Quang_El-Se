#define DIR_SEPARATOR 10
#define STEP_SEPARATOR 9
#define ST_ENABLE_SEPARATOR 8

#define DIR_ELEVATOR 7
#define STEP_ELEVATOR 6
#define ST_ENABLE_ELEVATOR 5

#define STEPENABLE LOW
#define STEPDISABLE HIGH

#define STEPUP    LOW
#define STEPDOWN  HIGH

#define TASK_BEGIN 0
#define TASK_RUN 1
#define TASK_END 2
#define ELEVATOR_UP 11
#define ELEVATOR_DOWN 12
#define ELEVATOR_ZEROING 13
#define SEPARATOR_UP 14
#define SEPARATOR_DOWN 15
#define WAIT_FOR_SETTLING 16
#define WAITING_FOR_COMMAND 10

/*===================================
 Timer variables
=====================================*/
unsigned long currentMicros;

unsigned long previousMicros_50Hz = 0;       
const long interval_50Hz = 20000;

unsigned long previousMicros_100Hz = 0;        
const long interval_100Hz = 10000; 


unsigned long previousMicros_800Hz = 0; 
const long interval_800Hz = 1250; 

unsigned long previousMicros_1000Hz = 0;       
const long interval_1000Hz = 1000; 
//======================================

const int ini_stepNum_SEPARATOR = 2*200; 
unsigned int stepNum_SEPARATOR = 0;
unsigned char stepDir_SEPARATOR = 0;

unsigned int ini_stepNum_ELEVATOR = 1*200/8; // = 1mm layer thickness
unsigned int stepNum_ELEVATOR = 0;
unsigned char stepDir_ELEVATOR = 0;

unsigned int current_layer = 1; 
unsigned int init_layer = 5;

const unsigned int settling_time = 10;
unsigned int settling_timer = 0;

boolean st_SEPARATOR = 0; // pulse variable for separator
boolean st_ELEVATOR = 0; // pulse variable for elevator

boolean ini_ELEVATOR_flag = 0; // flag for elevator to move the first layer
boolean done_flag = 0; // done printing

int zeroing_sensor = 0;
//=========================================
String command = "";
int commandID = 0;
int value = 0;
boolean value_flag = 0;
char cmd;
//=========================================
struct {
  unsigned char taskStt;
  unsigned char taskID;
} print_status;
//=========================================
void setup() {
  // put your setup code here, to run once:  
  pinMode(ST_ENABLE_SEPARATOR, OUTPUT);
  digitalWrite(ST_ENABLE_SEPARATOR, STEPDISABLE);//high: disenable | low: enable;
  pinMode(DIR_SEPARATOR, OUTPUT);
  pinMode(STEP_SEPARATOR, OUTPUT);
  digitalWrite(DIR_SEPARATOR, HIGH);

  pinMode(ST_ENABLE_ELEVATOR, OUTPUT);
  digitalWrite(ST_ENABLE_ELEVATOR, STEPDISABLE);//high: disenable | low: enable;
  pinMode(DIR_ELEVATOR, OUTPUT);
  pinMode(STEP_ELEVATOR, OUTPUT);
  digitalWrite(DIR_ELEVATOR, HIGH);
  
  Serial.begin(9600);
  print_status.taskID = WAITING_FOR_COMMAND;
  Serial.println("INPUT YOUR COMMAND: ");
  print_status.taskStt = TASK_BEGIN;
  stepNum_SEPARATOR = 2*ini_stepNum_SEPARATOR;
  stepNum_ELEVATOR = 2*ini_stepNum_ELEVATOR;
}


void loop() {
  // put your main code here, to run repeatedly:
  currentMicros = micros();

  //============================================================================= 50 Hz task
  if (currentMicros - previousMicros_50Hz >= interval_50Hz)
  {
    previousMicros_50Hz = currentMicros;
    if(Serial.available()){
      cmd = char(Serial.read());
      if (cmd != 10 && cmd != 13){command += cmd;}
      if (print_status.taskID == WAITING_FOR_COMMAND){
        if (cmd == 13 && value_flag == 0){
          if (command == "PRINT"){
            command = "";
            print_status.taskID = ELEVATOR_ZEROING;
            print_status.taskStt = TASK_BEGIN;
          }
          else if (command == "UP"){
            Serial.println("INPUT THE DISTANCE YOU WANT THE ELEVATOR TO MOVE UP");
            commandID = ELEVATOR_UP;
            value_flag = 1;        
          }
          else if (command == "DOWN"){
            Serial.println("INPUT THE DISTANCE YOU WANT THE ELEVATOR TO MOVE DOWN");
            commandID = ELEVATOR_DOWN;
            value_flag = 1;        
          }
          else {
            Serial.println("UNDEFINED COMMAND! PLEASE RE-ENTER!"); Serial.println();
            command = "";
          }
        }
        
        if (value_flag == 1){
          if (cmd >= 48 && cmd <= 57) {value = value*10 + (cmd-48);}   
          if (cmd == 13 && value != 0) {
            command = "";
            switch (commandID){
              case ELEVATOR_UP: 
                Serial.print ("COMMAND: ELEVATOR UP "); Serial.print(value); Serial.println(" mm"); Serial.println();
                print_status.taskID = ELEVATOR_UP;
                print_status.taskStt = TASK_BEGIN;
                ini_stepNum_ELEVATOR = value*200/8;
                value = 0;
                break;
              case ELEVATOR_DOWN:
                Serial.print ("COMMAND: ELEVATOR DOWN "); Serial.print(value); Serial.println(" mm"); Serial.println();
                print_status.taskID = ELEVATOR_DOWN;
                print_status.taskStt = TASK_BEGIN;
                ini_stepNum_ELEVATOR = value*200/8;
                value = 0;
                break;
            default: break;
            }
          } 
        }
      }
      else if (print_status.taskID != WAITING_FOR_COMMAND && command != "" && cmd == 13) {
        Serial.println("ANOTHER TASK IS OPERATING AT THE MOMENT, PLEASE WAIT!");
      }
    }
  }
  //============================================================================= 100 Hz task
  if (currentMicros - previousMicros_100Hz >= interval_100Hz)
  {
    previousMicros_100Hz = currentMicros;
    switch (print_status.taskID) {
        case WAIT_FOR_SETTLING: //===================================== WAIT FOR SETTLING
          switch (print_status.taskStt) {
            case TASK_BEGIN:
              Serial.print("Layer "); Serial.print(current_layer); Serial.println(" is being printed");
              print_status.taskStt = TASK_RUN;
              break;
            case TASK_RUN:
              settling_timer++;
              if (settling_timer >= settling_time){
                print_status.taskStt = TASK_END;
                settling_timer = 0;
              }
              break;
            case TASK_END:
              print_status.taskID = SEPARATOR_DOWN;
              print_status.taskStt = TASK_BEGIN;
              break;
            default: break;
         }
         break;
    }
  }
  //============================================================================== 800 Hz task 
  if (currentMicros - previousMicros_800Hz >= interval_800Hz)
  {
    previousMicros_800Hz = currentMicros;
     switch (print_status.taskID) {
        case SEPARATOR_DOWN: //======================================== SEPARATOR DOWN
          switch (print_status.taskStt) {
            case TASK_BEGIN:
              print_status.taskStt = TASK_RUN;
              stepNum_SEPARATOR = 2*ini_stepNum_SEPARATOR;
              digitalWrite(DIR_SEPARATOR,STEPDOWN);
              digitalWrite(STEP_SEPARATOR, LOW);
              digitalWrite(ST_ENABLE_SEPARATOR, STEPENABLE);
              break;
            case TASK_RUN:
              if (stepNum_SEPARATOR != 0){
                stepNum_SEPARATOR--;
                st_SEPARATOR ^= 1;
                digitalWrite(STEP_SEPARATOR,st_SEPARATOR);
              }
              else {
                print_status.taskStt = TASK_END;
              }
              break;
            case TASK_END:
              digitalWrite(ST_ENABLE_SEPARATOR, STEPDISABLE);
              st_SEPARATOR = 0;
              print_status.taskID = ELEVATOR_UP;
              print_status.taskStt = TASK_BEGIN;
              break;
            default: break;
         }
         break;
       case SEPARATOR_UP: //============================================ SEPARATOR UP
         switch (print_status.taskStt) {
            case TASK_BEGIN:
              print_status.taskStt = TASK_RUN;
              stepNum_SEPARATOR = 2*ini_stepNum_SEPARATOR + 2;
              digitalWrite(DIR_SEPARATOR,STEPUP);
              digitalWrite(STEP_SEPARATOR, LOW);
              digitalWrite(ST_ENABLE_SEPARATOR, STEPENABLE);
              break;
            case TASK_RUN:
              if (stepNum_SEPARATOR != 0){
                stepNum_SEPARATOR--;
                st_SEPARATOR ^= 1;
                digitalWrite(STEP_SEPARATOR,st_SEPARATOR);
              }
              else {
                print_status.taskStt = TASK_END;
              }
              break;
            case TASK_END:
              digitalWrite(ST_ENABLE_SEPARATOR, STEPDISABLE);
              st_SEPARATOR = 0;
              current_layer += 1;
              if(current_layer > init_layer){
                print_status.taskID = ELEVATOR_UP;
                ini_stepNum_ELEVATOR = 2000 - current_layer * (1*200/8); // = max pos. - current pos.
                Serial.println("PRINTING PROCESS IS DONE! ELEVATOR IS MOVING TO THE HIGHEST POSITION");
                done_flag = 1;
              }
              else {print_status.taskID = WAIT_FOR_SETTLING;}
              print_status.taskStt = TASK_BEGIN;
              break;
            default: break;
         }
       break;
     }
  }
  //================================================================================= 1000 Hz task
  if (currentMicros - previousMicros_1000Hz >= interval_1000Hz)
  {
    previousMicros_1000Hz = currentMicros;
    zeroing_sensor = analogRead(A0);
     switch (print_status.taskID) {
        case ELEVATOR_DOWN: //========================================== ELEVATOR DOWN
          switch (print_status.taskStt) {
            case TASK_BEGIN:
              print_status.taskStt = TASK_RUN;
              stepNum_ELEVATOR = 2*ini_stepNum_ELEVATOR;
              digitalWrite(DIR_ELEVATOR,STEPDOWN);
              digitalWrite(STEP_ELEVATOR, LOW);
              digitalWrite(ST_ENABLE_ELEVATOR, STEPENABLE);
              break;
            case TASK_RUN:
              if (zeroing_sensor == 0) {
                if (stepNum_ELEVATOR != 0){
                  stepNum_ELEVATOR--;
                  st_ELEVATOR ^= 1;
                  digitalWrite(STEP_ELEVATOR,st_ELEVATOR);
                }
                else {
                  print_status.taskStt = TASK_END;
                }
                break;
              }
              if (zeroing_sensor > 0) {
                if (stepNum_ELEVATOR != 0) {Serial.println("CANNOT FINISH THE DISTANCE!");}
                Serial.println("ELEVATOR IS AT THE ZERO POSITION");
                print_status.taskStt = TASK_END;
              }
              case TASK_END:
                digitalWrite(ST_ENABLE_ELEVATOR, STEPDISABLE);
                Serial.println("DONE!"); Serial.println(); Serial.println("INPUT YOUR COMMAND: ");
                ini_stepNum_ELEVATOR = 0;
                st_ELEVATOR = 0;
                value_flag = 0;
                command = "";
                print_status.taskID = WAITING_FOR_COMMAND;
                print_status.taskStt = TASK_BEGIN;
                break;
            default: break;
         }
         break;
       case ELEVATOR_UP: //============================================= ELEVATOR UP
         switch (print_status.taskStt) {
            case TASK_BEGIN:
              print_status.taskStt = TASK_RUN;
              stepNum_ELEVATOR = 2*ini_stepNum_ELEVATOR;
              digitalWrite(DIR_ELEVATOR,STEPUP);
              digitalWrite(STEP_ELEVATOR, LOW);
              digitalWrite(ST_ENABLE_ELEVATOR, STEPENABLE);
              break;
            case TASK_RUN:
              if (stepNum_ELEVATOR != 0){
                stepNum_ELEVATOR--;
                st_ELEVATOR ^= 1;
                digitalWrite(STEP_ELEVATOR,st_ELEVATOR);
              }
              else {
                print_status.taskStt = TASK_END;
              }
              break;
            case TASK_END:
              digitalWrite(ST_ENABLE_SEPARATOR, STEPDISABLE);
              st_SEPARATOR = 0;
              if (value_flag == 0){
                if (done_flag == 0) {
                  if (ini_ELEVATOR_flag == 1) {                   // not first layer
                    print_status.taskID = SEPARATOR_UP;
                  }
                  if (ini_ELEVATOR_flag == 0) {                   // first layer
                    print_status.taskID = WAIT_FOR_SETTLING;
                    ini_ELEVATOR_flag = 1;
                  }
                }
                else {
                  Serial.println("DONE!"); Serial.println(); Serial.println("INPUT YOUR COMMAND: ");
                  print_status.taskID = WAITING_FOR_COMMAND;
                  ini_stepNum_ELEVATOR = 1*200/8;
                  current_layer = 1;
                  ini_ELEVATOR_flag = 0;
                  done_flag = 0;
                  command = "";
                }
              }
              else {
                Serial.println("DONE!"); Serial.println(); Serial.println("INPUT YOUR COMMAND: ");
                print_status.taskID = WAITING_FOR_COMMAND;
                ini_stepNum_ELEVATOR = 1*200/8;
                value_flag = 0;
                command = "";
              }
              print_status.taskStt = TASK_BEGIN;
              break;
            default: break;
         }
       break;

       case ELEVATOR_ZEROING: //======================================== ELEVATOR ZEROING
         switch (print_status.taskStt) {
            case TASK_BEGIN:
              print_status.taskStt = TASK_RUN;
              Serial.println("ELEVATOR IS MOVING TO ZERO POSITION!");
              digitalWrite(DIR_ELEVATOR,STEPDOWN);
              digitalWrite(STEP_ELEVATOR, LOW);
              digitalWrite(ST_ENABLE_ELEVATOR, STEPENABLE);
              break;
            case TASK_RUN:
              st_ELEVATOR ^= 1;
              digitalWrite(STEP_ELEVATOR,st_ELEVATOR);
              if (zeroing_sensor > 0){              
                print_status.taskStt = TASK_END;
              }
              break;
            case TASK_END:
              digitalWrite(ST_ENABLE_ELEVATOR, STEPDISABLE);
              st_ELEVATOR = 0;
              print_status.taskID = ELEVATOR_UP;
              print_status.taskStt = TASK_BEGIN;
              break;
            default: break;
         }
       break;
    }
  }
}
