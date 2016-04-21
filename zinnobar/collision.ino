
// ******************* COLLISION CONTROL ******************* //
// 1. The bumpers (switches) are checked for their voltage level (HIGH or LOW)
// 2. Then the bumpers' states are updated based on that and their previous state
// 3. Then any actions required by the present states are taken

// https://www.arduino.cc/en/Tutorial/Debounce
void poll_bumpers() {
  for(int i = 0; i < BumperCount; i++) {
//    Serial.print("polling Bumper: ");
//    Serial.println(Bumpers[i]->name);

    // if the input has settled and existed for long enough as the same value, use it!
    if ((millis() - Bumpers[i]->lastDebounceTime) > BumperDebounceDelay) {
      Bumpers[i]->pinState = digitalRead(Bumpers[i]->pin) == HIGH ? true : false;
    }
  }
}

void service_collisions() {
  updateStates();

  // service single bumpers
  for (int i = 0; i < BumperCount; i++) {
    switch (Bumpers[i]->state) {
      case UP:
        // nothing to service, do nothing
        break;
      case DOWN:  // we just had impact. start servicing.
        Bumpers[i]->state = DOWN_SERVICING;

        // if the double bumper that encompansses this single bumper is SERVICING, then don't service the single bumper
        if (((FL_FC->children[0] == Bumpers[i] || FL_FC->children[1] == Bumpers[i]) && FL_FC->state == SERVICING)
          || ((FC_FR->children[0] == Bumpers[i] || FC_FR->children[1] == Bumpers[i]) && FC_FR->state == SERVICING)) {
          continue;
        } else {
          Bumpers[i]->service();  
        }
        break;
      case UP_SERVICING:    // cool, we're servicing. keep servicing.
        
        // if the double bumper that encompansses this single bumper is SERVICING, then don't service the single bumper
        if (((FL_FC->children[0] == Bumpers[i] || FL_FC->children[1] == Bumpers[i]) && FL_FC->state == SERVICING)
          || ((FC_FR->children[0] == Bumpers[i] || FC_FR->children[1] == Bumpers[i]) && FC_FR->state == SERVICING)) {
          continue;
        } else {
          Bumpers[i]->service();  
        }
        break;
      case DOWN_SERVICING:  // cool, we're servicing. keep servicing.
        
        // if the double bumper that encompansses this single bumper is SERVICING, then don't service the single bumper
        if (((FL_FC->children[0] == Bumpers[i] || FL_FC->children[1] == Bumpers[i]) && FL_FC->state == SERVICING)
          || ((FC_FR->children[0] == Bumpers[i] || FC_FR->children[1] == Bumpers[i]) && FC_FR->state == SERVICING)) {
          continue;
        } else {
          Bumpers[i]->service();  
        }
        break;
    }
  }

  // service double bumpers
  for (int i = 0; i < DoubleBumperCount; i++) {
    switch (DoubleBumpers[i]->state) {
      case SERVICING:
        DoubleBumpers[i]->service();     // service!
        break;
      case SERVICED:
        // we're not serving now, so do nothing
        break;
    }
  }
}

void updateStates() {
  // update bumper states based on switch levels (pin states)
  for(int i = 0; i < BumperCount; i++) {     

//    Serial.print(Bumpers[i]->name);

    switch (Bumpers[i]->state) {
      case UP:
        if (Bumpers[i]->pinState == HIGH) {
          // you hit a wall after not being on a wall.
          Bumpers[i]->state = DOWN;
          Bumpers[i]->timeTriggered = millis();

//          Serial.println(" >> DOWN");
        } else {
          // stay in this state
//          Serial.println(" >> stay in UP");
        }
        break;
      case DOWN:
        // this case should never happen
        Serial.print(Bumpers[i]->name);
        Serial.println(" - *** ERROR IN BUMPER STATE TRANSITIONS ***");
        break;
      case DOWN_SERVICING:
        if (Bumpers[i]->pinState == HIGH) {
          // stay in this state

//          Serial.println(" >> stay in DOWN_SERVICING");
        } else {
          // we've moved off of the wall
          Bumpers[i]->state == UP_SERVICING;     
          
//          Serial.println(" >> UP_SERVICING");
        }
        break;
      case UP_SERVICING:
        if (Bumpers[i]->pinState == HIGH) {
          // you hit a wall after not being on a wall.
          Bumpers[i]->state = DOWN;

//          Serial.println(" >> DOWN");
        } else {
          // stay in this state

//          Serial.println(" >> stay in UP_SERVICING");
        }
        break;
    }
  }

  // if two adjacent bumpers are down, service the collision with a double bumper instead of the single bumpers
  if ((FL->state == DOWN || FL->state == DOWN_SERVICING) && (FC->state == DOWN || FC->state == DOWN_SERVICING)) {
    FL_FC->state = SERVICING;
    Serial.print(FL_FC->name);
    Serial.println(" >> SERVICING");
  }
  if ((FC->state == DOWN || FC->state == DOWN_SERVICING) && (FR->state == DOWN || FR->state == DOWN_SERVICING)) {
    FC_FR->state = SERVICING;
    Serial.print(FC_FR->name);
    Serial.println(" >> SERVICING");
  }
}

// interrupts to update the time of the last voltage change on each switch
void FL_bumper_event() {
  if (MasterSequence == LOCKED) {
    UserCombo[lockInputNum++] = LEFT;
  } else if (MasterSequence == SETTINGS) {
    BotType = SCARLET_WITCH;
    MasterSequenceNum++;
  } else {
    Serial.println("FL bounced");
    FL->lastDebounceTime = millis();
  }
}

void FC_bumper_event() {
  if (MasterSequence == LOCKED) {
    UserCombo[lockInputNum++] = CENTER;
  } else if (MasterSequence == SETTINGS) {
    BotType = NIGHTWING;
    MasterSequenceNum++;
  } else {
    FC->lastDebounceTime = millis();
  }
}

void FR_bumper_event() {
  if (MasterSequence == LOCKED) {
    UserCombo[lockInputNum++] = RIGHT;
  } else if (MasterSequence == SETTINGS) {
    BotType = TEST_BOT;
    MasterSequenceNum++;
  } else {
    FR->lastDebounceTime = millis();
  }
}

void B_bumper_event() {
  if (MasterSequence == LOCKED) {
    check_combo();
  } else if (MasterSequence == SETTINGS) {
    // this is not a valid option for settings
  } else if (MasterSequence[MasterSequenceNum] == TEST_TRANSMITTER {
    MasterSequenceNum++;
  } else if (MasterSequence[MasterSequenceNum] == TEST_RECEIVER {
    MasterSequenceNum--
  } else {
    FC->lastDebounceTime = millis();
    B->lastDebounceTime = millis();
  }
}

// function to service the FL bumper when it is triggered individually
void service_FL() {
  
  // if the service time hasn't expired, service here
  if (millis() < FL->timeTriggered + FL->serviceTime) {
//    Serial.println("Servcing FL");
    reverseLeft();
    
  } else {  // the service is done. change states.
    Serial.println("Service time for FL is expried");
    FL->state = UP;
  }
}

void service_FC() {
  if (millis() < FC->timeTriggered + FC->serviceTime) { // service here
    if (MasterSequence == FINISH_LINE) {
      MasterSequence = END;
    } else {
      reverse();
    }
  
  } else { // the service is done. change states.
    halt();
    FC->state = UP;
  }
}

void service_FR() {
  if (millis() < FR->timeTriggered + FR->serviceTime) { // service here
    reverseRight();
    
  } else { // the service is done. change states.
    digitalWrite(FR->ledPin, LOW);
    FR->state = UP;
  }
}

void service_B() {
  if (millis() < B->timeTriggered + B->serviceTime) { // service here
   forward_slow();
   
  } else { // the service is done. change states.
    digitalWrite(B->ledPin, LOW);
    B->state = UP;
  }
}

// double bumpers

void service_FL_FC() {
  // the timeTriggered is the timeTriggered of the child that has the largest (most recent) timeTriggered
  long timeTriggered = FL->timeTriggered > FC->timeTriggered ? FL->timeTriggered : FC->timeTriggered;
  
  if (millis() < timeTriggered + FL_FC->serviceTime) { // service here
    reverseLeft();
    
  } else { // the service is done. change states (children too).
    FL_FC->state = SERVICED;
    FL->state = UP;
    FC->state = UP;
  }
}


void service_FC_FR() {
  // the timeTriggered is the timeTriggered of the child that has the largest (most recent) timeTriggered
  long timeTriggered = FC->timeTriggered > FR->timeTriggered ? FC->timeTriggered : FR->timeTriggered;
  
  if (millis() < timeTriggered + FC_FR->serviceTime) { // service here
    reverseRight();
    
  } else { // the service is done. change states (children too).
    FC_FR->state = SERVICED;
    FC->state = UP;
    FR->state = UP;
  }
}
