//
//  ES1 CanSat-i tarkvara
//  Ergo Adams
//  2020
//
#define RADIOPIN A1
 
#include <string.h>
#include <util/crc16.h>
#include <Adafruit_MPL115A2.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SdFat.h>

// Temperatuuri ja õhurõhu sensor ja muutujad temperatuuri ja rõhu jaoks
Adafruit_MPL115A2 mpl115a2;
float temperature = 0;
float pressure = 0;

// GPS
TinyGPSPlus gps;
SoftwareSerial ss(0, 1);
float gpslatitude, gpslongitude;
float gpsaltitude;
float gpsspeed;
int gpsday, gpsmonth, gpsyear;
int gpshour, gpsminute, gpssecond;

// SD
const int PROGMEM CS_Pin = 10;
SdFat sd;
SdFile logFile;

// Gyro
const int PROGMEM MPU=0x68;
int16_t AcX,AcY,AcZ,GyX,GyY,GyZ;
float pitch,roll;

// Raadioside, millegipärast kood ei toimi, kui muutujad tõsta funktsiooni endasse. Pole aimugi, miks nii on...
const char *radiomsg = "ES1 %d.%d.%d %d:%d:%d N %s E %s %sm %skmph %s *C %skPa";
char chrTemperature[15];
char chrPressure[15];
char chrLatitude[10];
char chrLongitude[10];
char chrAltitude[10];
char chrSpeed[10];
char chrPitch[10];
char chrRoll[10];
char charBuffer[80];

void rtty_txstring (char * string)
{
  char c;
  c = *string++;
  
  while ( c != '\0')
  {
    rtty_txbyte (c);
    c = *string++;
  }
}

void rtty_txbyte (char c)
{
  int i;
 
  rtty_txbit (0); // Start bitt

  for (i=0;i<7;i++) // 7/8 vastavalt sellele, kas kasutatakse ASCII-7 / ASCII-8
  {
    if (c & 1) rtty_txbit(1); 
    else rtty_txbit(0); 
    c = c >> 1;
  }
 
  rtty_txbit (1); // Stopp bitt
  rtty_txbit (1); // Stopp bitt
}

void rtty_txbit (int bit)
{
  if (bit)
  {
    digitalWrite(RADIOPIN, HIGH);
  }
  else
  {
    digitalWrite(RADIOPIN, LOW);
 
  }
 
  //delayMicroseconds(3370); // 300 baud rate
  delayMicroseconds(10000); // 50 baud rate 
  delayMicroseconds(10150); 
}

uint16_t gps_CRC16_checksum (char *string) {
  size_t i;
  uint16_t crc;
  uint8_t c;
 
  crc = 0xFFFF;
 
  // Kontrollsumma arvutamine, esimesed kaks $s ei loe
  for (i = 2; i < strlen(string); i++) {
    c = string[i];
    crc = _crc_xmodem_update (crc, c);
  }
 
  return crc;
}

void sendRTTY(char *datastring) {
  unsigned int CHECKSUM = gps_CRC16_checksum(datastring);  // Kontrollsumma
  char checksum_str[6];
  sprintf(checksum_str, "*%04X\n", CHECKSUM);
  strcat(datastring,checksum_str);
 
  rtty_txstring(datastring); //24852, 1558
}

// Kuidas RTTY funktsioonid üldse töötavad? Pisikene kokkuvõte (loodetavasti)
// 1. CRC16 checksum, liidetakse datastringi lõppu -> kuidas checksum tehakse?, kas saab efektiivsemalt datastringi juurde panna
// 2. kuni pole jõudnud null täheni (andmete lõpp), võtame tähe, saadame start biti, iga biti tähes (alustades lsb-st) ja 2 stopp bitti
// saatmine on vastavalt bitile, kui on 1 siis radiopin high, muidu low

static void smartDelay(unsigned long ms) {
  ss.begin(9600);
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);

  ss.end();
}

void getAngle(int Vx,int Vy,int Vz) {
  double x = Vx;
  double y = Vy;
  double z = Vz;
  
  pitch = atanf(x/sqrtf((y*y) + (z*z)));
  roll = atanf(y/sqrtf((x*x) + (z*z)));
  //convert radians into degrees
  pitch = pitch * 57;
  roll = roll * 57;
}

void readGyro() {
  // Loeme kiirendussensorit
  AcX = Wire.read()<<8|Wire.read();
  AcY = Wire.read()<<8|Wire.read();
  AcZ = Wire.read()<<8|Wire.read();
  
  // Loeme güroskoopi
  GyX = Wire.read()<<8|Wire.read();
  GyY = Wire.read()<<8|Wire.read();
  GyZ = Wire.read()<<8|Wire.read();
  getAngle(AcX, AcY, AcZ);
}

void readPT() {
  mpl115a2.getPT(&pressure,&temperature);
}

void readGPS() {
  gpslatitude = gps.location.lat();
  gpslongitude = gps.location.lng();
  gpsspeed = gps.speed.kmph();
  gpsaltitude = gps.altitude.meters();
  TinyGPSDate gpsdate = gps.date;
  TinyGPSTime gpstime = gps.time;
  gpsday = gpsdate.day();
  gpsmonth = gpsdate.month();
  gpsyear = gpsdate.year();
  gpshour = gpstime.hour();
  gpsminute = gpstime.minute();
  gpssecond = gpstime.second();

  if (gpshour + 2 >= 24) {
    gpshour = (gpshour + 2) % 24;
    gpsday = gpsday + 1;
  } else {
    gpshour = (gpshour + 2);
  }
}

void writeData() {
  // Avame logifaili, kui ei õnnestu avada, siis error
  if (!logFile.open("log.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening log.txt for write failed");
  }

  // Kirjutame andmed kaardile
  // See on väga kole viis, kuidas andmeid kirjutada, aga kuidagi on vaja mälu säästa :)
  logFile.print(gpsday);
  logFile.print(F("."));
  logFile.print(gpsmonth);
  logFile.print(F("."));
  logFile.print(gpsyear);
  logFile.print(F(" "));
  
  logFile.print(gpshour);
  logFile.print(F(":"));
  logFile.print(gpsminute);
  logFile.print(F(":"));
  logFile.print(gpssecond);
  logFile.print(F(" "));

  logFile.print(F("N "));
  logFile.print(gpslatitude);
  logFile.print(F(" E "));
  logFile.print(gpslongitude);
  logFile.print(F(" "));

  logFile.print(gpsaltitude);
  logFile.print(F("m "));
  logFile.print(gpsspeed);
  logFile.print(F("kmph "));

  logFile.print(temperature);
  logFile.print(F("*C "));
  logFile.print(pressure);
  logFile.print(F("kPa "));

  logFile.print(pitch);
  logFile.print(F("* pitch "));
  logFile.print(roll);
  logFile.println(F("* roll "));
  
  // Paneme logifaili kinni
  logFile.close();
}

void sendData() {
  
  dtostrf(temperature, 5, 2, chrTemperature);
  dtostrf(pressure, 5, 1, chrPressure);
  dtostrf(gpslatitude, 7, 3, chrLatitude);
  dtostrf(gpslongitude, 7, 3, chrLongitude);
  dtostrf(gpsaltitude, 5, 1, chrAltitude);
  dtostrf(gpsspeed, 5, 1, chrSpeed);
  
  snprintf(charBuffer, 80, radiomsg, gpsday, gpsmonth, gpsyear, gpshour, gpsminute, gpssecond, chrLatitude, chrLongitude, chrAltitude, chrSpeed, chrTemperature, chrPressure);
  sendRTTY(charBuffer);
  
}

void setup() {
  // Siin oli bootup protsess, mis kontrollis, kas sensor läks tööle, aga see võtab liiga palju mälu, et alles jätta.
  // Tuleb lihtsalt loota, et hakkab õigesti tööle
  
  pinMode(RADIOPIN,OUTPUT);
  Serial.begin(9600);

  // SD kaart
  if (!sd.begin(CS_Pin, SPI_HALF_SPEED)) sd.initErrorHalt();

  // MPL115A2 (temp ja õhurõhk)
  mpl115a2.begin();
  delay(1000);

  // GPS init, ärme siin veel alusta, lülitame ainult andmete lugemise ajaks sisse, muidu segab teisi funktsioone
  //ss.begin(9600);

  // Gyro init
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("ES1 started!");
}

void loop() {
  smartDelay(60000);
  readGyro();
  readPT();
  readGPS();
  writeData();
  sendData();
}
