
#include <Wire.h> //Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

long lastTime = 0; //Simple local timer. Limits amount if I2C traffic to u-blox module.

typedef struct{
  long latitude ;
  long longitude ;
  long altitude ;
} Coordinates;

Coordinates error;

long latitude = 0;
long longitude = 0;
long altitude = 0;
int n = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for user to open terminal
  Serial.println("SparkFun u-blox Example");

  Wire.begin();

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }
}

void loop()
{
  //Query module only every second. Doing it more often will just cause I2C traffic.
  //The module only responds when a new position is available
  if (millis() - lastTime > 1000)
  {


    lastTime = millis(); //Update the timer
    
    latitude = myGNSS.getLatitude();
    longitude = myGNSS.getLongitude();
    altitude = myGNSS.getAltitude();
    
    Serial.print(F("Lat: "));
    Serial.print(latitude);
    Serial.print(F(" Long: "));
    Serial.print(longitude);

    getCoordsError();
    fixErrorRT(latitude, longitude, altitude);

    long speed = myGNSS.getGroundSpeed();
    Serial.print(F(" Speed: "));
    Serial.print(speed);
    Serial.print(F(" (mm/s)"));

    long heading = myGNSS.getHeading();
    Serial.print(F(" Heading: "));
    Serial.print(heading);
    Serial.print(F(" (degrees * 10^-5)"));

    int pDOP = myGNSS.getPDOP();
    Serial.print(F(" pDOP: "));
    Serial.print(pDOP / 100.0, 2); // Convert pDOP scaling from 0.01 to 1

    byte SIV = myGNSS.getSIV();
    Serial.print(F(" SIV: "));
    Serial.print(SIV);

    Serial.println();

    Serial.print("Average latitude: ");
    Serial.print(lastAverageLat);
    Serial.print(" Average longitude: ");
    Serial.print(lastAverageLon);
    Serial.print(" n: ");
    Serial.print(n);

    Serial.println();
  }
}

void getCoordsError(){
  error.latitude = 0;
  error.longitude = 0;
  error.altitude = 0;
}

void fixErrorRT(long latitude, long longitude, long altitude){
  latitude -= error.latitude;
  longitude -= error.longitude;
  altitude -= error.altitude;
}