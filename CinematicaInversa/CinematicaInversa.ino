#include <Braccio.h>
#include <Servo.h>

// Include the library InverseK.h
#include <InverseK.h>


const int sensorPin = 22;

Servo base;
Servo shoulder;
Servo elbow;
Servo wrist_rot;
Servo wrist_ver;
Servo gripper;

enum Action {
  INITIAL =  0,
  MOVE_INITIAL,
  GRABBING,
  SCANNING,
  SCANNING_RETURN,
  CHECK,
  DROP_ITEM
};

struct State {
  Action action;
  int x;
  int y;
  int z;
};

State state = { .action = MOVE_INITIAL, .x = -100, .y = 240, .z = 240 };

void printStatePosition() {
    Serial.print("(");
    Serial.print(state.x);
    Serial.print(",");
    Serial.print(state.y);
    Serial.print(",");
    Serial.print(state.z);
    Serial.print(") ");
}

void printAngles(float a0, float a1, float a2, float a3) {
    Serial.print(a2b(a0)); Serial.print(',');
    Serial.print(a2b(a1)); Serial.print(',');
    Serial.print(a2b(a2)); Serial.print(',');
    Serial.println(a2b(a3));
}

void setup() {  
  pinMode(sensorPin, INPUT_PULLUP);

  Braccio.begin();
  
  // Setup the lengths and rotation limits for each link
  Link base, upperarm, forearm, hand;

  base.init(75, b2a(0.0), b2a(180.0));
  upperarm.init(125, b2a(15.0), b2a(165.0));
  forearm.init(125, b2a(0.0), b2a(180.0));
  hand.init(195, b2a(0.0), b2a(180.0));

  // Attach the links to the inverse kinematic model
  InverseK.attach(base, upperarm, forearm, hand);
  
  Serial.begin(9600); 
}  

void loop() {
  if(state.action == MOVE_INITIAL) {
    state.x = -100;
    state.y = -150;
    state.z = 240;
    printStatePosition();

    float a0, a1, a2, a3;
    if(InverseK.solve(state.x, state.y, state.z, a0, a1, a2, a3)) {
      printAngles(a0, a1, a2, a3);
  
      Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  10);    

      state.action = SCANNING;
      Serial.println("New state.action: SCANNING");
    } else {
      Serial.println("No solution found!");
    }
  } else if(state.action == SCANNING) {
    if(state.y >= 150) {
      state.action = SCANNING_RETURN;
      Serial.println("New state.action: SCANNING_RETURN");

      return;
    }

    state.x = -100;
    state.y += 25;
    state.z = 240;

    printStatePosition();
    
    float a0, a1, a2, a3;
    if(InverseK.solve(state.x, state.y, state.z, a0, a1, a2, a3)) {
      printAngles(a0, a1, a2, a3);
  
      Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  10);    

      state.action = CHECK;
      Serial.println("New state.action: CHECK");

      delay(500);
    } else {
      Serial.println("No solution found!");

      state.action = MOVE_INITIAL;
      Serial.println("New state.action: MOVE_INITIAL");
    }
  } else if(state.action == SCANNING_RETURN) {
    {
      state.x = -100;
      state.y = 0;
      state.z = 240;
      printStatePosition();
      
      float a0, a1, a2, a3;
      if(InverseK.solve(state.x, state.y, state.z, a0, a1, a2, a3)) {
        printAngles(a0, a1, a2, a3);
    
        Braccio.ServoMovement(50, 146, a2b(a1), a2b(a2), a2b(a3), 83,  10);    
      } else {
        Serial.println("No solution found!");

        state.action = MOVE_INITIAL;
        Serial.println("New state.action: MOVE_INITIAL");
        return;
      }
    }

    {
      state.x = -100;
      state.y = -150;
      state.z = 240;
      printStatePosition();
      
      float a0, a1, a2, a3;
      if(InverseK.solve(state.x, state.y, state.z, a0, a1, a2, a3)) {
        printAngles(a0, a1, a2, a3);
    
        Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  10);    

        state.action = CHECK;
        Serial.println("New state.action: CHECK");

        delay(500);
      } else {
        Serial.println("No solution found!");

        state.action = MOVE_INITIAL;
        Serial.println("New state.action: MOVE_INITIAL");
        return;
      }
    }
  } else if(state.action == GRABBING) {
    state.x = -200;
    float a0, a1, a2, a3;

    printStatePosition();
    
    if(InverseK.solve(state.x, state.y, state.z, a0, a1, a2, a3)) {
      printAngles(a0, a1, a2, a3);
  
      Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  10);    
      Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  73); 
      Braccio.ServoMovement(50, a2b(a0), a2b(a1), a2b(a2), a2b(a3), 83,  10);    

      state.action = SCANNING_RETURN;
      Serial.println("New state.action: SCANNING_RETURN");

      delay(1000);
    } else {
      Serial.println("No solution found!");
    }
  } else if(state.action == CHECK) {   
    if(digitalRead(sensorPin) == HIGH) {
      Serial.println("Distance: OK");

      state.action = GRABBING;
      Serial.println("New state.action: GRABBING");
    } else {
      Serial.println("More than  20cm");
      
      state.action = SCANNING;
      Serial.println("New state.action: SCANNING");
    }

    delay(3000);// This needs to be large because otherwise the cable to the sensor gets a lot of noise
  } else if(state.action == DROP_ITEM) {   
    // TODO
  }
}

// Quick conversion from the Braccio angle system to radians
float b2a(float b){
  return b / 180.0 * PI - HALF_PI;
}

// Quick conversion from radians to the Braccio angle system
float a2b(float a) {
  return (a + HALF_PI) * 180 / PI;
}
