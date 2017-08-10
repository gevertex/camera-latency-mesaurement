#include <Arduino.h>
#include <TM1637Display.h>

//TM1637 setup
#define TMCLK 16
#define TMDIO 10


//Segment mapping
//   A
//  ---
//F|   |B
//G ---
//E|   |C
//  ---
//   D
const uint8_t SEG_ERR[] = {
  0,                                     //Space
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, //E
  SEG_A | SEG_F | SEG_E,                 //R
  SEG_A | SEG_F | SEG_E,                 //R
};

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

//Blank display
const uint8_t SEG_BLANK[] = {
  0,
  0,
  0,
  0
};

//  0GFEDCBA
//0b00000000
uint8_t ANIMATION_DATA[][4] = {
  { 0b00000001, 0b00000001, 0b00000001, 0b00000001 },
  { 0b00000000, 0b00000001, 0b00000001, 0b00000011 },
  { 0b00000000, 0b00000000, 0b00000001, 0b00000111 },
  { 0b00000000, 0b00000000, 0b00000000, 0b00001111 },
  { 0b00000000, 0b00000000, 0b00001000, 0b00001110 },
  { 0b00000000, 0b00001000, 0b00001000, 0b00001100 },
  { 0b00001000, 0b00001000, 0b00001000, 0b00001000 },
  { 0b00011000, 0b00001000, 0b00001000, 0b00000000 },
  { 0b00111000, 0b00001000, 0b00000000, 0b00000000 },
  { 0b00111001, 0b00000000, 0b00000000, 0b00000000 },
  { 0b00110001, 0b00000001, 0b00000000, 0b00000000 },
  { 0b00100001, 0b00000001, 0b00000001, 0b00000000 },
};


//LED and Opto pins
#define LED_PIN 2
//ADC pin for photodiode (using 1M Ohm resistor as a pull up on the circuit)
#define ADC_PIN 0
#define ADC_GROUND_PIN 15

//Defines for operation
TM1637Display display(TMCLK, TMDIO);

//Threshold for change required to trigger sample time calculation
int LIGHT_DELTA_THRESHOLD_HIGH = -20;
int LIGHT_DELTA_THRESHOLD_LOW = -10;

//Time measurements above this are considered invalid
unsigned long TIMEOUT_MS = 500;
unsigned long TIMEOUT_US = TIMEOUT_MS * 1000;

//Bias to subtract from current sensor value.  Used to calculate delta
long light_bias;

// the setup function runs once when you press reset or power the board
void setup() {
  setupDisplay();
  animate();
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_PIN, OUTPUT);

  //Initialize ground pin for photodiode
  pinMode(ADC_GROUND_PIN, OUTPUT);
  digitalWrite(ADC_GROUND_PIN, LOW);
  
  //Start serial
  Serial.begin(115200);
  
}

void setupDisplay(){
  display.setBrightness(7);
//  display.setSegments(SEG_ERR);
}

void animate(){
  int i;

  for(i = 0; i < 12; i++) {
    display.setSegments(ANIMATION_DATA[i]);
    delay(50);
  }
  display.setSegments(ANIMATION_DATA[0]);
  delay(50);

  for(i=7; i >= 0; i--){
    display.setSegments(ANIMATION_DATA[0]);
    display.setBrightness(i);
    delay(100);
//    if (i != 0) {
//      delay(200/i);
//    }else
//      delay(200);
  }
  
  display.setSegments(SEG_BLANK);
  display.setBrightness(7);
}

void printReport(long maxLatency, long minLatency, long avgLatency){

  display.showNumberDec(avgLatency);
  
  Serial.println("");
  Serial.println("----------------Report----------------");
  
  Serial.print("Average Latency: ");
  Serial.print(String(avgLatency));
  Serial.println("ms");

  Serial.print("Max Latency:     ");
  Serial.print(String(maxLatency/1000));
  Serial.println("ms");

  Serial.print("Min Latency:     ");
  Serial.print(String(minLatency/1000));
  Serial.println("ms");

  Serial.println("------------End Report----------------");
  Serial.println("");
  
  delay(5000);
}

//returns -1 if timed out. Returns latency if did not timeout
long getCurrLatencyUs(){
  unsigned long elapsedTimeOn = 0;
  unsigned long elapsedTimeOff = 0;

  ledOn();
  elapsedTimeOn = measuredLEDOnUs();
  ledOff();
  elapsedTimeOff = waitLEDMeasuredOffUs();
//  Serial.print("Delta LED Off: ");
//  Serial.println(String(lightDelta()));

//  Serial.print("Elapsed time on: ");
//  Serial.println(String(elapsedTimeOn));
//  Serial.print("Elapsed time off: ");
//  Serial.println(String(elapsedTimeOff));

  if ((elapsedTimeOn != -1) && (elapsedTimeOff != -1))
    return (elapsedTimeOn + elapsedTimeOff) / 2;
  else
    return -1;
}

unsigned long measuredLEDOnUs(){
  unsigned long startTime = 0;
  unsigned long elapsedTime = 0;
  
  startTime = micros();
  while (true){
    
    //Determine if we timed out
    elapsedTime = micros() - startTime; 
    if (elapsedTime > TIMEOUT_US){
      Serial.println("Wait led on timeout");
      elapsedTime = -1;
      break;
    }

    //Determine if we got a valid measurement
    elapsedTime = micros() - startTime;
    if (lightDelta() < LIGHT_DELTA_THRESHOLD_HIGH){
      break;
    }
  }

//  Serial.print("Delta LED On: ");
//  Serial.println(String(lightDelta()));

  return elapsedTime;
}

unsigned long waitLEDMeasuredOffUs(){
  unsigned long elapsedTime = 0;
  unsigned long startTime = micros();

  while(true){

    elapsedTime = micros() - startTime;
    if (elapsedTime > TIMEOUT_US){
      Serial.println("Wait led off timeout");
      elapsedTime = -1;
      break;
    }

    elapsedTime = micros() - startTime;
    if (lightDelta() > LIGHT_DELTA_THRESHOLD_LOW){
      break;
    }
    
  }

  return elapsedTime;
}

void ledOn(){
  digitalWrite(LED_PIN, HIGH);
}

void ledOff(){
  digitalWrite(LED_PIN, LOW);
}

void calibrateThresholds(){
  ledOn();
  delay(500);
  long on_level = lightDelta();
  ledOff();
  delay(500);
  long off_level = lightDelta();

  LIGHT_DELTA_THRESHOLD_HIGH = on_level * 80 / 100;
  LIGHT_DELTA_THRESHOLD_LOW = on_level * 20 / 100;
//
  Serial.print("High Threshold:" );
  Serial.println(String(LIGHT_DELTA_THRESHOLD_HIGH));
  Serial.print("Low Threshold:" );
  Serial.println(String(LIGHT_DELTA_THRESHOLD_LOW));
}

long averageSamples(long samples, long bias=0){
  long total = 0;

  for (int i=0; i<samples; i++){
    total += (analogRead(ADC_PIN) - bias);
  }

  return total / samples;
}

void calibrateBias(){
  light_bias = averageSamples(30);
  calibrateThresholds();
  Serial.print("Calaulated Light Bias:" );
  Serial.println(String(light_bias));
  Serial.print("Current Light Sensor Value: ");
  Serial.println(analogRead(ADC_PIN));
}

int lightDelta(){
  long samples = 30;
//  long startTime = millis();
  long light_delta = averageSamples(samples, light_bias);
//  long elapsedTime = millis() - startTime;
//  Serial.print("Time: ");
//  Serial.print(String(elapsedTime));
//  Serial.println("ms");
  return light_delta;
}

// the loop function runs over and over again forever
void loop() {
  
  int latencySamples = 30;
  long currentLatency = 0;
  unsigned long totalLatencyUs = 0;
  unsigned long samplesMeasured = 0;
  unsigned long minLatency = 100000;
  unsigned long maxLatency = 0;

  calibrateBias();
  Serial.println("Starting Measurement");
  for (int i=0; i<latencySamples; i++){
    currentLatency = getCurrLatencyUs();
   
    Serial.print("Current Latency: ");
    Serial.print(String(currentLatency/1000));
    Serial.println("ms");

    if (currentLatency > 0){
      minLatency = min(currentLatency, minLatency);
      maxLatency = max(currentLatency, maxLatency);
      totalLatencyUs += currentLatency;
      samplesMeasured++;
    }
  }

  unsigned long averageLatency = totalLatencyUs/samplesMeasured/1000;
  printReport(maxLatency, minLatency, averageLatency);
}

