 //Libraries
#include <Arduino.h>
#include "DFRobot_BME280.h" //BME280 Sensor
#include "sensirion_common.h" //VOC sensor
#include "sgp30.h" // VOC sensor
#include <LCD_I2C.h> //LCD screen
// Cayenne
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP32.h>

// WiFi network info.
char ssid[] = "";
char wifiPassword[] = "";


// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "";
char password[] = "";
char clientID[] = "";

//Cayenne

//Screen
LCD_I2C lcd(0x27, 16, 2); // Default address of most PCF8574 modules, change according

//Screen

// PM2.5/5/10 sensor init
#define LENG 31   //0x42 + 31 bytes equal to 32 bytes
unsigned char buf[LENG];

int PM01Value=0;          //define PM1.0 value of the air detector module
int PM2_5Value=0;         //define PM2.5 value of the air detector module
int PM10Value=0;         //define PM10 value of the air detector module

// Finish PM2.5/5/10 sensor finish

// BME280 Sensor init
typedef DFRobot_BME280_IIC    BME;    // ******** use abbreviations instead of full names ********

/**IIC address is 0x77 when pin SDO is high */
/**IIC address is 0x76 when pin SDO is low */
BME   bme(&Wire, 0x77);   // select TwoWire peripheral and set sensor address

#define SEA_LEVEL_PRESSURE    1013.2f

// show last sensor operate status
void printLastOperateStatus(BME::eStatus_t eStatus)
{
  switch(eStatus) {
  case BME::eStatusOK:    Serial.println("everything ok"); break;
  case BME::eStatusErr:   Serial.println("unknow error"); break;
  case BME::eStatusErrDeviceNotDetected:    Serial.println("device not detected"); break;
  case BME::eStatusErrParameter:    Serial.println("parameter error"); break;
  default: Serial.println("unknow status"); break;
  }
}
// BME280 Sensor finish

//VOC sensor init
//VOC sensor finish


void setup()
{
  //PM2.5/5/10 sensor init
  Serial.begin(9600);   //use serial0
  Serial.setTimeout(1500);    //set the Timeout to 1500ms, longer than the data transmission periodic time of the sensor
  // Finish PM2.5/5/10 sensor finish
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);

  // BME280 Sensor init
  Serial.println("bme read data test");
  while(bme.begin() != BME::eStatusOK) {
    Serial.println("bme begin faild");
    printLastOperateStatus(bme.lastOperateStatus);
    delay(2000);
  }
  Serial.println("bme begin success");
  delay(100);
  // BME280 Sensor finish

  //VOC sensor
    s16 err;
    u32 ah = 0;
    u16 scaled_ethanol_signal, scaled_h2_signal;
    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != STATUS_OK) {
        Serial.println("SGP failed");
        while (1);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);
    if (err == STATUS_OK) {
        Serial.println("get ram signal!");
    } else {
        Serial.println("error reading signals");
    }

    
    err = sgp_iaq_init();
    //
 
    //Screen
    lcd.begin(); 
                 
  //  lcd.backlight();
    //Screen


}

void loop()
{
  static unsigned long wifitimer=millis();
  static unsigned long LCD=millis();

  if (millis() - wifitimer >=1800000)
    {
      wifitimer=millis();
      Cayenne.begin(username, password, clientID, ssid, wifiPassword);
    }
  if (millis() - LCD >=300000){
    lcd.backlight();

  }

  if (millis() - LCD >=600000){
    LCD=millis();
    lcd.noBacklight();

  }
   
  
  //Cayenne
  Cayenne.loop();
  //cayenne


}

CAYENNE_OUT_DEFAULT()
{
    //BME280
  float   temp = bme.getTemperature();
  uint32_t    press = bme.getPressure();
  float   alti = bme.calAltitude(SEA_LEVEL_PRESSURE, press);
  float   humi = bme.getHumidity();
  //BME280

  //VOC
  float Ah = 216.70*((humi/100)*6.112*exp((17.62*temp)/(243.12+temp)))/(273.15+temp);
  sgp_set_absolute_humidity(Ah);
    s16 err = 0;
    u16 tvoc_ppb, co2_eq_ppm;
    err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
  
  //VOC

  //PM2.5/5/10 sensor init
  if(Serial.find(0x42)){    //start to read when detect 0x42
    Serial.readBytes(buf,LENG);

    if(buf[0] == 0x4d){
      if(checkValue(buf,LENG)){
        PM01Value=transmitPM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value=transmitPM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value=transmitPM10(buf); //count PM10 value of the air detector module
      }
    }
  }

  static unsigned long serialTimer=millis();
  static unsigned long LCDTimer=millis();
    if (millis() - serialTimer >=13000)
    {
      serialTimer=millis();
      LCDTimer=millis();
      Cayenne.virtualWrite(0, temp, TYPE_TEMPERATURE, UNIT_CELSIUS);
      Cayenne.virtualWrite(1, press/100.0, TYPE_BAROMETRIC_PRESSURE, UNIT_HECTOPASCAL);
      Cayenne.virtualWrite(2, alti, "Altitude", "m (H20)");
      Cayenne.virtualWrite(3, humi, "Humidity", "%");
      Cayenne.virtualWrite(4, PM01Value, "Particular Material", "ug/m3");
      Cayenne.virtualWrite(5, PM2_5Value, "Particular Material", "ug/m3");
      Cayenne.virtualWrite(6, PM10Value, "Particular Material", "ug/m3");
      Cayenne.virtualWrite(7, tvoc_ppb, "Volatile Organic", "Parts per Billion");
      Cayenne.virtualWrite(8, co2_eq_ppm, "Carbon Dioxide", "Parts per Million");

      Serial.println();
      Serial.println("======== start print ========");
      Serial.print("temperature (unit Celsius): "); Serial.println(temp);
      Serial.print("pressure (unit hpa):        "); Serial.println(press/100.0);
      Serial.print("altitude (unit meter):      "); Serial.println(alti);
      Serial.print("humidity (unit percent):    "); Serial.println(humi);

      Serial.print("PM1.0: ");
      Serial.print(PM01Value);
      Serial.println("  ug/m3");

      Serial.print("PM2.5: ");
      Serial.print(PM2_5Value);
      Serial.println("  ug/m3");

      Serial.print("PM1 0: ");
      Serial.print(PM10Value);
      Serial.println("  ug/m3");

      Serial.print("tVOC  Concentration:");
      Serial.print(tvoc_ppb);
      Serial.println("ppb");

      Serial.print("CO2eq Concentration:");
      Serial.print(co2_eq_ppm);
      Serial.println("ppm");

      Serial.print("Absolute Humidity:");
      Serial.print(Ah);
      


      Serial.println();

      Serial.println("========  end print  ========");
    }
lcd.clear();
  while (millis() - LCDTimer >=0 && millis() - LCDTimer <= 2000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print("Temp(C): "); // You can make spaces using well... spaces
      lcd.setCursor(9, 0);
      lcd.print(temp); // You can make spaces using well... spaces
      lcd.setCursor(0, 1);
      lcd.print("Hum(%): "); // You can make spaces using well... spaces
      lcd.setCursor(8, 1);
      lcd.print(humi); // You can make spaces using well... spaces
      

    }
lcd.clear();
      while (millis() - LCDTimer >2000 && millis() - LCDTimer <= 4000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print("P(hPa): "); // You can make spaces using well... spaces
      lcd.setCursor(8, 0);
      lcd.print(press/100.0); // You can make spaces using well... spaces

      

    }
lcd.clear();
  while (millis() - LCDTimer > 4000 && millis() - LCDTimer <= 6000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print(" PM 1.0-2.5-10 "); // You can make spaces using well... spaces
      lcd.setCursor(4, 1);
      lcd.print("ug/m3"); // You can make spaces using well... spaces

      

    }
lcd.clear();
  while (millis() - LCDTimer > 6000 && millis() - LCDTimer <= 8000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print("1.0: "); // You can make spaces using well... spaces
      lcd.setCursor(5, 0);
      lcd.print(PM01Value); // You can make spaces using well... spaces
      lcd.setCursor(0, 1);
      lcd.print("2.5: "); // You can make spaces using well... spaces
      lcd.setCursor(5, 1);
      lcd.print(PM2_5Value); // You can make spaces using well... spaces
     
    }
lcd.clear();
  while (millis() - LCDTimer > 8000 && millis() - LCDTimer <= 10000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print("10: "); // You can make spaces using well... spaces
      lcd.setCursor(5, 0);
      lcd.print(PM10Value); // You can make spaces using well... spaces
   
    }
lcd.clear();
  while (millis() - LCDTimer > 10000 && millis() - LCDTimer <= 12000)
    {
      
      lcd.setCursor(0, 0);
      lcd.print("tVOC(ppb): "); // You can make spaces using well... spaces
      lcd.setCursor(11, 0);
      lcd.print(tvoc_ppb); // You can make spaces using well... spaces
      lcd.setCursor(0, 1);
      lcd.print("CO2(ppm): "); // You can make spaces using well... spaces
      lcd.setCursor(10, 1);
      lcd.print(co2_eq_ppm); // You can make spaces using well... spaces
     
    }



      // Finish PM2.5/5/10 sensor finish


}

//PM2.5/5/10 sensor init
char checkValue(unsigned char *thebuf, char leng)
{
  char receiveflag=0;
  int receiveSum=0;

  for(int i=0; i<(leng-2); i++){
  receiveSum=receiveSum+thebuf[i];
  }
  receiveSum=receiveSum + 0x42;

  if(receiveSum == ((thebuf[leng-2]<<8)+thebuf[leng-1]))  //check the serial data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
  }

//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module
  return PM10Val;
}
 // Finish PM2.5/5/10 sensor finish