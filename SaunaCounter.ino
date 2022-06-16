/*
 Sauna Counter

This software calculates the cost for the present sauna session.
An LDR connected to analog input 6 must be taped to a lamp which
indicates that electric current is applied to the sauna heater.
Every 1000 ms the state ist evaluated.
If the current is applied the OnCounter is incremented.
The value of OnCounter is then used to calculate the accumulated
cost since power on of the ECU.

Writing data to SD card is prepared but doesn't work yet.
So it's commented out.

 */
#include <OneWire.h>
#include <DS18B20.h>
//#define USE_SD_CARD

#ifdef USE_SD_CARD
  #include <mySD.h>
#endif

#define ONE_WIRE_BUS 27 // 2

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

#include "SSD1306Wire.h"

// Energy cost [EUR / kWh]
#define ECpkWh  0.2243

// Energy tax [EUR / kWh]
#define ETpkWh  0.0205

// Sales tax as factor
#define Tax     1.16

// Power of Sauna heater [kW]
#define Power   4.5

#define F_AppName F("SaunaCounter")

 // Initialize the OLED display using Wire library
 SSD1306Wire  display(0x3c, 21, 22);

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}


void display_text (int x, int y, String line_of_text){
  display.drawString(x, y, line_of_text);
}

float Cost_factor;

void setup() {
  char logString [130];
  float Cost_h;

  Serial.begin(115200);

#ifdef USE_SD_CARD
  Serial.print("Initializing SD card...");
  /* initialize SD library with Soft SPI pins, if using Hard SPI replace with this SD.begin()*/
//   if (!SD.begin(26, 14, 13, 27)) {
   if (!SD.begin(5, 23, 19, 18)) {
//   if (!SD.begin()) {
    Serial.println("SD initialization failed!");
//    return;
  } else {
    Serial.println("SD initialization done.");
  }
#endif

  // Initialise OLED display
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.setLogBuffer(5, 30);

  display_text(0,  0, F_AppName);
  display_text(0, 16, __DATE__);
  display_text(0, 32, __TIME__);
  Cost_factor = ((ECpkWh + ETpkWh) * Tax * Power) / 3600.0;
  Cost_h = ((float) 3600) * Cost_factor;
  sprintf(logString,"Cost_h  %f EUR", Cost_h);
  display_text(0, 48, logString);
  display.display();

  sensor.begin();
  sensor.setResolution(9);
  sensor.requestTemperatures();

  delay(5000);      // time to read the display   

  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 1000000, true);

  // Start an alarm
  timerAlarmEnable(timer);

}

int OnCounter = 0;
int OffCounter = 0;
// Save data to sd card every 5 minutes (= 300s)
#define TAB_LENGTH 300
int   TabIndex = 0;
int   SensTab [TAB_LENGTH] = {0};
float TempTab [TAB_LENGTH] = {0.0};

void loop() {
  char logString [130];
  int  sensorValue;
  float Cost;
  float temperature;

  // If Timer has fired
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
    uint32_t isrCount = 0, isrTime = 0;

    // Read the interrupt count and time
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);

    display.clear();
    display_text(0,  0, F_AppName);

    sensorValue = analogRead(A6); 
    if (sensor.isConversionComplete()){
      temperature = sensor.getTempC();
      sensor.requestTemperatures();
    }else{
      temperature = 999.9;
    }
 
    sprintf(logString,"Heater %d Temp %3.1f°", sensorValue, temperature);
    display_text(0, 16, logString);
    SensTab [TabIndex] = sensorValue;
    TempTab [TabIndex] = temperature;

    if (TabIndex >= TAB_LENGTH){
      TabIndex = 0;
    } else {
      TabIndex++;
    }

    if (sensorValue < 2000){
      OnCounter++;
    } else{
      OffCounter++;
    }
    sprintf(logString,"On %d  Off %d", OnCounter, OffCounter);
    display_text(0, 32, logString);

    Cost = ((float) OnCounter) * Cost_factor;
    sprintf(logString,"Cost  %1.4f EUR", Cost);
    display_text(0, 48, logString);
    display.display();
  }
}
