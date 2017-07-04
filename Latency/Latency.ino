//PIN for Red LED
int LED_PIN = 7;
//ADC pin for photodiode
int ADC_PIN = 0;
//Threshold for change required to trigger sample time calculation
int LIGHT_DELTA_THRESHOLD = 20;
//Time measurements above this are considered invalid
int TIME_OUT_MS = 1000;

//Bias to subtract from current sensor value.  Used to calculate delta
int light_bias;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_PIN, OUTPUT);

  //Start serial
  Serial.begin(115200);
  
}

// the loop function runs over and over again forever
void loop() {
  int latencySamples = 10;
  int currentLatency = 0;
  unsigned long totalLatencyMs = 0;

  Serial.println("Starting Measurement");
  calibrate();
  for (int i=0; i<latencySamples; i++){
    totalLatencyMs += getCurrLatencyMs();
  }
  
  Serial.print("average Latency: ");
  Serial.println(String(totalLatencyMs/latencySamples));
}

//returns -1 if timed out. Returns latency if did not timeout
int getCurrLatencyMs(){
  int startTime = 0;
  int endTime = 0;
  int elapsedTime = 0;
  int result;
  
  startTime = millis();
  ledOn();
  while (true){
    //Determine if we timed out
    elapsedTime = millis() - startTime; 
    if (elapsedTime > TIME_OUT_MS){
      result = -elapsedTime;
      break;
    }

    //Determine if we got a valid measurement
    if (lightDelta() > LIGHT_DELTA_THRESHOLD){
      result = elapsedTime;
      break;
    }
  }

  ledOff();

  if (result > 0){
    waitLEDMeasuredOff();
  }
  
  return result;
}

void waitLEDMeasuredOff(){
  int elapsedTime;
  int startTime = millis();

  while(true){
    elapsedTime = millis() - startTime;
    
    if (elapsedTime > TIME_OUT_MS){
      break;
    }

    if (lightDelta() < LIGHT_DELTA_THRESHOLD){
      break;
    }
    
  }
}

void ledOn(){
  digitalWrite(LED_PIN, HIGH);
}

void ledOff(){
  digitalWrite(LED_PIN, LOW);
}

int lightDelta(){
  int delta = analogRead(ADC_PIN) - light_bias;
  return abs(delta);
}

void calibrate(){
  light_bias = averageSamples(50);
  Serial.print("Calaulated Light Bias:" );
  Serial.println(String(light_bias));
  Serial.print("Current Light Sensor Value: ");
  Serial.println(analogRead(ADC_PIN));
}

int averageSamples(int samples){
  unsigned long total = 0;

  for (int i=0; i<samples; i++){
    total += analogRead(ADC_PIN);
    delay(10);
  }

  return total / samples;
}

