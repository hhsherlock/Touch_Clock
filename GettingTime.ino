//NTP-Server Tutorial: https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
#include <movingAvg.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define button D4
#define buzzer D5
#define potio A0 //Potentiometer Pin

//for Motor
#define IN1 D6 
#define IN2 D7
#define PWM D8

const char* ssid       = "XXX";
const char* password   = "XXX";

const long utcOffsetInSeconds = 7200; //7200 for Wintertime, else 3600 for Summertime
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

int spikeRows = 720;
int textureAll = 900;

int currentPosition = 0;
int targetPosition = 0;

bool alarmOn = false;
bool wasAlarmOnToday = false;

int alarmHour = 12;
int alarmMinutes = 4;

movingAvg mySensor(5);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  mySensor.begin();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(PWM, OUTPUT);
  analogWriteRange(1023);
  
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), handleInterruptButton, CHANGE);
  
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  pinMode(potio, INPUT_PULLUP);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi verbunden");
  Serial.println("IP Addresse: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  timeClient.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  int val = analogRead(potio);
  int sensorMovingAvg = mySensor.reading(val);
  Serial.println(val);
  timeClient.update();

  int timediff = timeDifference();
  if (timediff <= 120){
    targetPosition = map(timediff, 30, 120, textureAll, spikeRows);
    moving();
  }

  if(timediff > 120){
    targetPosition = textureAll;
    moving();
  }

  if (timeClient.getHours() == alarmHour && !wasAlarmOnToday){
    if(timeClient.getMinutes() == alarmMinutes){
      wasAlarmOnToday = true;
      alarmOn = true;
    }
  }

  if(wasAlarmOnToday && timeClient.getMinutes() != alarmMinutes){
    wasAlarmOnToday = false;
  }

  while(alarmOn && wasAlarmOnToday){
    tone(buzzer, 440, 1000);
    Serial.println("ALARM!!!111!!!111!!");
    yield();
    delay(1000);
  }
  noTone(buzzer);

  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());

  delay(500);

}

int timeDifference(){ //calculate the remaining minutes until alarm rings
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinutes = timeClient.getMinutes();
  int timeDifference = 0;
  if (currentHour < alarmHour){
    timeDifference = 60 - currentMinutes;
    currentMinutes = 0;
    currentHour = currentHour+1;
  }
  while(currentHour < alarmHour){
    timeDifference = timeDifference + 60;
    currentHour = currentHour + 1;
  }
  if(currentHour == alarmHour){
    int minuteDiff = alarmMinutes - currentMinutes;
    timeDifference = timeDifference + minuteDiff;
  }
  if(currentHour > alarmHour){
    timeDifference = 500;
  }
  return timeDifference;
}

void getCurrentPosition(){
  int val = 0;
  for(int i = 0; i < 10; i++){
    val = val + analogRead(potio);
  }
  val = val/10;
  currentPosition = val;
  Serial.print("Current position: ");
  Serial.println(currentPosition);
}

void checkDirection(){ //left ground and right 3.3V
  getCurrentPosition();
  if (currentPosition < (targetPosition -2)){
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH); //for pushing
  }
  if(currentPosition > (targetPosition + 2)){
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW); //for draw near (heranziehen)
  }
}

void moving(){
  getCurrentPosition();
  while(currentPosition < (targetPosition-2) || currentPosition > (targetPosition+2)){
    checkDirection();
    analogWrite(PWM, 250);
    delay(20);
    getCurrentPosition();
  }
}

ICACHE_RAM_ATTR void handleInterruptButton(){
  alarmOn = false;
}
