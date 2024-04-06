//constexpr byte OutGate1 = 9;   // OC1A
//constexpr byte OutGate2 = 6;   // OC4D
//constexpr byte OutGate3 = 10;  // OC4B
//constexpr byte OutGate4 = 11;  // OC1C


constexpr byte InBpm = A3;
constexpr byte InDecay = A2;
constexpr byte InMults = A1;
constexpr byte InRate4 = A0;


template<byte TimerNum, char OutputName>
class Gate {
public:
  Gate() {
    pinMode(getPin(), OUTPUT);
  }

  void setDivision(int8_t div) {
    mBeatDivider = div;
  }

  void onBeat() {
    --mBeatsTillNextTrig;
    if (mBeatsTillNextTrig > 0)
      return;

    mVal = 255 << 6;
    mBeatsTillNextTrig = mBeatDivider;
  }

  // called every ms
  void tick(int rawDecay) {
    if (rawDecay > 500) {
      long decay = map(rawDecay, 500, 1024, 240 * 4, 256 * 4 - 1);

      long x = mVal;
      x *= decay;
      mVal = x >> 10;
    }
    else if (rawDecay < 500) {
      long decay = map(rawDecay, 0, 500, 1L << 4, 50L << 4);

      if (mVal >= decay) {
        mVal -= decay;
      }
      else {
        mVal = 0;
      }
    }

    setPwmLevel(mVal >> 6);
  }

  constexpr byte getPin() const {
    if constexpr (TimerNum == 4 && OutputName == 'B') {
      return 10;
    }
    else if constexpr (TimerNum == 4 && OutputName == 'D') {
      return 6;
    }
    else if constexpr (TimerNum == 1 && OutputName == 'A') {
      return 9;
    }
    else if constexpr (TimerNum == 1 && OutputName == 'C') {
      return 11;
    }

    return 0;
  }

  void setPwmLevel(uint16_t t) {
    if constexpr (TimerNum == 4 && OutputName == 'B') {
      TC4H = byte(t >> 8);
      OCR4B = byte(t);
    }
    else if constexpr (TimerNum == 4 && OutputName == 'D') {
      TC4H = byte(t >> 8);
      OCR4D = byte(t);
    }
    else if constexpr (TimerNum == 1 && OutputName == 'A') {
      OCR1A = t << 2;
    }
    else if constexpr (TimerNum == 1 && OutputName == 'C') {
      OCR1C = t << 2;
    }
  }

private:
  int mVal = 0;
  int8_t mBeatDivider = 1;
  int8_t mBeatsTillNextTrig = 1;
};


Gate<1, 'A'> gate1;
Gate<4, 'D'> gate2;
Gate<4, 'B'> gate3;
Gate<1, 'C'> gate4;


void setup() {
  //Serial.begin(115200);
  
  // set up timer 1
  TCCR1A = _BV(COM1A1) | _BV(COM1C1) | _BV(WGM11) | _BV(WGM10);   // enable PWM on 1A & 1C, fast 10bit PWM
  TCCR1B = _BV(WGM12) | _BV(CS10);       // fast 10bit PWM, prescaler 1x
  TCCR1C = 0;

  // set up timer 4
  TCCR4A = _BV(PWM4B) | _BV(COM4A1) | _BV(COM4B1);   // enable PWM on 4B
  TCCR4B = _BV(CS41);       // prescaler 2x
  TCCR4C = _BV(PWM4D) | _BV(COM4B1S) | _BV(COM4D1);   // enable PWM on 4D (and leave 4B enabled)
  TCCR4D = 0;               // fast PWM
  TCCR4E = 0;// _BV(OC4OE1) | _BV(OC4OE3) | _BV(OC4OE5);   // connect the noninverted output pins
}


long lastUs = 0;
long nextBeatUs = 0;
int nextLevelUpdateUs = 0;


void updateMults() {
  int rawMults = analogRead(InMults);

  switch(map(rawMults, 0, 1023, 0, 3)) {
  case 0:
    gate1.setDivision(1);
    gate2.setDivision(2);
    gate3.setDivision(3);
    gate4.setDivision(4);
    break;

  case 1:
    gate1.setDivision(1);
    gate2.setDivision(2);
    gate3.setDivision(4);
    gate4.setDivision(8);
    break;

  case 2:
    gate1.setDivision(1);
    gate2.setDivision(3);
    gate3.setDivision(5);
    gate4.setDivision(7);
    break;

  case 3:
    gate1.setDivision(2);
    gate2.setDivision(3);
    gate3.setDivision(5);
    gate4.setDivision(7);
  default:
    break;
  }
}


void loop() {
  long nowUs = micros();
  uint16_t deltaUs = nowUs - lastUs;
  lastUs = nowUs;

  nextBeatUs -= deltaUs;
  if (nextBeatUs <= 0) {
    updateMults();

    int rawRate = analogRead(InBpm) >> 2;
    constexpr long minuteUs = 60L*1000L*1000L;
    long usPerBeat = minuteUs / long(rawRate);

    nextBeatUs += usPerBeat;

    //Serial.print("new bpm = ");
    //Serial.print(rawRate);
    //Serial.print(", new beat us = ");
    //Serial.println(usPerBeat);
    
    gate1.onBeat();
    gate2.onBeat();
    gate3.onBeat();
    gate4.onBeat();
  }

  // update gate values
  nextLevelUpdateUs -= deltaUs;
  if (nextLevelUpdateUs <= 0) {
    int rawDecay = analogRead(InDecay);

    gate1.tick(rawDecay);
    gate2.tick(rawDecay);
    gate3.tick(rawDecay);
    gate4.tick(rawDecay);

    nextLevelUpdateUs += 1000;

  //    Serial.print(val);
  //    Serial.print(",");
  //    Serial.println();
  }
}
