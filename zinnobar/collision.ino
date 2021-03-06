
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
//  Serial.println("FL bounced");
  FL->lastDebounceTime = millis();
}

void FC_bumper_event() {
  FC->lastDebounceTime = millis();
}

void FR_bumper_event() {
  FR->lastDebounceTime = millis();
}

void B_bumper_event() {
  B->lastDebounceTime = millis();
}

// function to service the FL bumper when it is triggered individually
void service_FL() {
  // if the service time hasn't expired, service here
  if (millis() < FL->timeTriggered + FL->serviceTime) {
    switch(MasterSequence[MasterSequenceNum]) {
      case LOCKED:
        FL->serviceTime = 100;
        Serial.println("LOCK_LEFT");
        digitalWrite(alertRed, HIGH);
        delay(100);
        digitalWrite(alertRed, LOW);
        userCombo[lockInputNum++] = LOCK_LEFT;
        delay(FL->serviceTime + 10);
        break;
      case SETTINGS:
        FL->serviceTime = 100;
        Serial.println("Selected SCARLET_WITCH");
        BotType = SCARLET_WITCH;
        MasterSequenceNum++;
        delay(FL->serviceTime + 10);
        break;
      case FIND_WALL: {
        Serial.println("collided with wall");
        FL->serviceTime = 1000;
        int reverseTime = 100;
        if (millis() < FL->timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        } else if (millis() < FL->timeTriggered + time180 + reverseTime) {
          if (BotType == SCARLET_WITCH) {
            Serial.println("turnLeftInPlace");
            turnLeftInPlace();
          } else {
            Serial.println("turnRightInPlace");
            turnRightInPlace();
          }
        } else {
          Serial.println("delay, move to next state");
          halt();
          drive();
          MasterSequenceNum++;  // finished with this service, move on to the next collision
          delay(FL->serviceTime - reverseTime + time180 + 10); // delay to expire service time
        }
      } break;
      default:
        FL->serviceTime = 400;
        reverseLeft();
    }
  } else {  // the service is done. change states.
//    Serial.println("Service time for FL is expried");
    halt();
    FL->state = UP;
  }
}

void service_FC() {
  if (millis() < FC->timeTriggered + FC->serviceTime) { // service here
    switch(MasterSequence[MasterSequenceNum]) {
      case LOCKED:
        FC->serviceTime = 100;
        Serial.println("LOCK_CENTER");
        digitalWrite(alertYellow, LOW);
        delay(100);
        digitalWrite(alertYellow, HIGH);
        userCombo[lockInputNum++] = LOCK_CENTER;
        delay(FC->serviceTime + 10);
        break;
      case SETTINGS:
        FC->serviceTime = 100;
        Serial.println("Selected NIGHTWING");
        BotType = NIGHTWING;
        MasterSequenceNum++;
        delay(FC->serviceTime + 10);
        break;
      case FIND_WALL: {
        Serial.println("collided with wall");
        FC->serviceTime = 1000;
        int reverseTime = 100;
        if (millis() < FC->timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        } else if (millis() < FC->timeTriggered + time180 + reverseTime) {
          if (BotType == SCARLET_WITCH) {
            Serial.println("turnLeftInPlace");
            turnLeftInPlace();
          } else {
            Serial.println("turnRightInPlace");
            turnRightInPlace();
          }
        } else {
          Serial.println("delay, move to next state");
          halt();
          drive();
          MasterSequenceNum++; // finished with this service, move on to the next collision
          delay(FL->serviceTime - reverseTime + time180 + 10); // delay to expire service time
        }
      } break;
      case FOLLOW_PATH_2: {
        FC->serviceTime = 300;
        Serial.println("Collided with final wall");
        int reverseTime = 200;
        if (millis() < FC->timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        }
        MasterSequenceNum++;
        delay(FC->serviceTime - reverseTime + 10);
      } break;
      default:
        FC->serviceTime = 400;
        turnLeftInPlace();
    }
  } else { // the service is done. change states.
    halt();
    FC->state = UP;
  }
}

void service_FR() {
  if (millis() < FR->timeTriggered + FR->serviceTime) { // service here
    switch(MasterSequence[MasterSequenceNum]) {
      case LOCKED:
        FC->serviceTime = 100;
        Serial.println("LOCK_RIGHT");
        digitalWrite(alertBlue, HIGH);
        delay(100);
        digitalWrite(alertBlue, LOW);
        userCombo[lockInputNum++] = LOCK_RIGHT;
        delay(FR->serviceTime + 10);
        break;
      case SETTINGS:
        FC->serviceTime = 100;
        Serial.println("Selected DANCER");
        BotType = DANCER;
        MasterSequenceNum++;
        delay(FR->serviceTime + 10);
        break;
      case FIND_WALL: {
        Serial.println("collided with wall");
        FC->serviceTime = 1000;
        int reverseTime = 100;
        if (millis() < FR->timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        } else if (millis() < FR->timeTriggered + time180 + reverseTime) {
          if (BotType == SCARLET_WITCH) {
            Serial.println("turnLeftInPlace");
            turnLeftInPlace();
          } else {
            Serial.println("turnRightInPlace");
            turnRightInPlace();
          }
        } else {
          Serial.println("delay, move to next state");
          halt();
          drive();
          MasterSequenceNum++; // finished with this service, move on to the next collision
          delay(FL->serviceTime - reverseTime + time180 + 10); // delay to expire service time
        }
      } break;
      default:
        FC->serviceTime = 400;
        reverseRight();
    }
  } else { // the service is done. change states.
    halt();
    FR->state = UP;
  }
}

void service_B() {
  if (millis() < B->timeTriggered + B->serviceTime) { // service here
    switch(MasterSequence[MasterSequenceNum]) {
      case LOCKED:
        B->serviceTime = 100;
        check_combo();
        delay(B->serviceTime + 10);
        break;
      case SETTINGS:
        B->serviceTime = 100;
        MasterSequenceNum++;
        Serial.println("Selected TEST_BOT");
        BotType = TEST_BOT;
        delay(B->serviceTime + 10);
        break;
      case TEST_TRANSMITTER:
        B->serviceTime = 100;
        MasterSequenceNum++;
        delay(B->serviceTime + 10);
        break;
      case TEST_RECEIVER:
        B->serviceTime = 100;
        MasterSequenceNum--;
        delay(B->serviceTime + 10);
        break;
      default:
        FC->serviceTime = 400;
        turnRight();
    }
  } else { // the service is done. change states.
    halt();
    B->state = UP;
  }
}

// double bumpers

void service_FL_FC() {
  // the timeTriggered is the timeTriggered of the child that has the largest (most recent) timeTriggered
  long timeTriggered = FL->timeTriggered > FC->timeTriggered ? FL->timeTriggered : FC->timeTriggered;
  
  if (millis() < timeTriggered + FL_FC->serviceTime) { // service here
    switch(MasterSequence[MasterSequenceNum]) {
      case FIND_WALL: {
        FL_FC->serviceTime = 1000;
        int reverseTime = 100;
        if (millis() < timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        } else if (millis() < timeTriggered + time180 + reverseTime) {
          if (BotType == SCARLET_WITCH) {
            Serial.println("turnLeftInPlace");
            turnLeftInPlace();
          } else {
            Serial.println("turnRightInPlace");
            turnRightInPlace();
          }
        } else {
          Serial.println("delay, move to next state");
          FL_FC->serviceTime = 500;
          halt();
          drive();
          MasterSequenceNum++; // finished with this service, move on to the next collision
          delay(FL_FC->serviceTime - reverseTime + time180 + 10); // delay to expire service time
        }
      } break;
      case FOLLOW_PATH_2: {
        Serial.println("Collided with final wall");
        FL_FC->serviceTime = 300;
        int reverseTime = 200;
        if (millis() < timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        }
        MasterSequenceNum++;
        delay(FL_FC->serviceTime - reverseTime + 10);
      } break;
      default:
        FL_FC->serviceTime = 400;
        reverseLeft();
    }
  } else { // the service is done. change states (children too).
    halt();
    FL_FC->state = SERVICED;
    FL->state = UP;
    FC->state = UP;
  }
}

void service_FC_FR() {
  // the timeTriggered is the timeTriggered of the child that has the largest (most recent) timeTriggered
  long timeTriggered = FC->timeTriggered > FR->timeTriggered ? FC->timeTriggered : FR->timeTriggered;
  
  if (millis() < timeTriggered + FC_FR->serviceTime) { // service here
    switch(MasterSequence[MasterSequenceNum]) {
      case FIND_WALL: {
        FC_FR->serviceTime = 1000;
        int reverseTime = 100;
        if (millis() < timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        } else if (millis() < timeTriggered + time180 + reverseTime) {
          if (BotType == SCARLET_WITCH) {
            Serial.println("turnLeftInPlace");
            turnLeftInPlace();
          } else {
            Serial.println("turnRightInPlace");
            turnRightInPlace();
          }
        } else {
          Serial.println("delay, move to next state");
          halt();
          drive();
          MasterSequenceNum++; // finished with this service, move on to the next collision
          delay(FC_FR->serviceTime - reverseTime + time180 + 10); // delay to expire service time
        }
      } break;
      case FOLLOW_PATH_2: {
        Serial.println("Collided with final wall");
        FC_FR->serviceTime = 300;
        int reverseTime = 200;
        if (millis() < timeTriggered + reverseTime) {
          Serial.println("reverse");
          reverse();
        }
        MasterSequenceNum++;
        delay(FC_FR->serviceTime - reverseTime + 10);
      } break;
      default:
        FC_FR->serviceTime = 400;
        reverseRight();
    }
  } else { // the service is done. change states (children too).
    FC_FR->state = SERVICED;
    FC->state = UP;
    FR->state = UP;
  }
}
