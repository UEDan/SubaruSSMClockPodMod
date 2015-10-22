#include <DS3231.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//TEST UNITS

//4th byte is # of packets you idiot.
//double check checksum byte you jackass.
byte ReqData[31] = {128, 16, 240, 26, 168, 0, 0, 0, 16, 0, 0, 19, 0, 0, 70, 0, 1, 33, 0, 0, 100, 2, 9, 199, 2, 1, 104, 0, 0, 20, 130}; // add throttle
byte ReqDataSize = 31;
//END TEST UNITS

//variables for MPG:
byte mpgReqData[19] = {128, 16, 240, 14, 168, 0, 0, 0, 16, 0, 0, 19, 0, 0, 20, 0, 0, 70, 188};
byte mpgReqDataSize = 19;
//variables for IAM
byte iamReqData[13] = {128, 16, 240, 8, 168, 0, 2, 1, 104, 0, 0,  34, 186};
byte iamReqDataSize = 13;
//variables for MPH
byte case3ReqData[10] = {128, 16, 240, 5, 168, 0, 0, 0, 16, 61};
byte case3ReqDataSize = 10;
//variables for test
byte case4ReqData[13] = {128, 16,  240, 8, 168, 0, 0, 0, 70, 2, 12, 96, 228}; // AFR && FBKC
byte case4ReqDataSize = 13;
//variables for Knock
byte case5ReqData[13] = {128, 16, 240, 8, 168, 0, 0, 0, 19, 0, 0, 20, 64};
byte case5ReqDataSize = 13;
//4th byte is # of packets(no checksum) you idiot && double check checksum byte you jackass.

int theHour, theMinute, theSecond, theTemperature; //DS3231 Parameters
byte timeUpdateCount = 0;
byte swtVal = 0;
byte selMode = 1;
byte readBytes;
int ECUbytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int milli;
unsigned long prvTime, curTime;
double milesPerHour, airFuelR, fbkc, airFlowG, milesPerGallon;
DS3231 rtc;

//Declare LCD as lcd and I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
//Rx/Tx pins used for SSM
SoftwareSerial sendSerial = SoftwareSerial(10, 11); //Rx, Tx

void setup() {
  //TEST SETUP
  pinMode(13, OUTPUT);
  //END TEST SETUP

  pinMode(9, INPUT); //mode selector switch
  //Setup Start
  Serial.begin(115200); //for diagnostics
  Serial.println("Serial Started");
  while (!Serial) {
    // wait
  }
  lcd.begin(16, 2); //Start LCD
  lcd.backlight(); //Set LCD Backlight ON
  lcd.setCursor(0, 0); //start at col 1 row 1
  lcd.print("Hello you!");
  delay(1500);

  Serial.println("starting SSM Serial");
  sendSerial.begin(4800); //SSM uses 4800 8N1 baud rate
  while (!sendSerial) {
    //wait
    delay(50);
  }
  //Serial.println("Ready!");
  delay(50);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("MPG: ");
  readBytes = ((mpgReqDataSize - 7) / 3);
  writeSSM(mpgReqData, mpgReqDataSize, sendSerial); //send intial SSM poll
  delay (2);

}


void loop() {
  curTime = millis();
  milli = curTime - prvTime;

  if (milli > 250) {
    sendSerial.flush();
    //delay(5);
    //  Serial.print("SentTime:");
    //  Serial.println(milli);
    ssmWriteSel();
    //writeSSM(ReqData, ReqDataSize, sendSerial);
    //Serial.print("Timer Popped | ");
    //Serial.println(sendSerial.available());
    prvTime = millis();
  }

  if (sendSerial.available()) {
    readECU(ECUbytes, readBytes, false);

    prvTime = curTime;

    lcdPrintSel();
    /*
      milesPerHour = (ECUbytes[0] * 0.621371192); //P9 0x000010
      airFuelR = ((ECUbytes[2] / 128.00) * 14.7);  //P58 0x000046
      airFlowG = (((ECUbytes[1] * 256.00) + ECUbytes[7]) / 100.00); //P12 0x000013 and 0x000014
      milesPerGallon = (milesPerHour / 3600.00) / (airFlowG / (airFuelR) / 2800.00);

      Serial.print("MPH:");
      Serial.print(milesPerHour, 0);
      Serial.print(" | ");
      Serial.print("Mass airflow/s:");
      Serial.print(airFlowG);
      Serial.print(" | ");
      Serial.print("AFR: ");
      Serial.print(airFuelR);
      Serial.print(" | ");
      Serial.print("MPG:");
      Serial.print(milesPerGallon);
      Serial.print(" | ");
      Serial.print("Cruise:"); //0x000121
      Serial.print(ECUbytes[3], BIN);
      Serial.print(" | ");
      Serial.print("Defogger:");
      Serial.print(ECUbytes[4], BIN); //0x000064
      Serial.print(" | ");
      Serial.print("Gear:"); //0x0209C7
      Serial.print(ECUbytes[5]);
      Serial.print(" | ");
      Serial.print("IAM:"); //0x020168
      Serial.println(ECUbytes[6]);
    */

  }
  //Mode switch read
  if (digitalRead(9) == 1) {
    if (selMode == 5) {
      selMode = 0;
    }
    selMode++;
    //Serial.println("Mode plus");
    //printMode(selMode);
    delay(500);
    lcd.clear();
    switch (selMode)
    {
      case 1: //Fuel Economy
        lcd.setCursor(0, 1);
        lcd.print("MPG:");
        digitalWrite(13, HIGH);
        readBytes = ((mpgReqDataSize - 7) / 3);
        break;
      case 2: //IAM
        lcd.setCursor(0, 1);
        lcd.print("IAM: ");
        digitalWrite(13, LOW);
        readBytes = ((iamReqDataSize - 7) / 3);
        break;
      case 3: //Miles per hour
        digitalWrite(13, HIGH);
        lcd.setCursor(0, 1);
        lcd.print("MPH: ");
        readBytes = ((case3ReqDataSize - 7) / 3);
        break;
      case 4: //Air:fuel Ratio
        //lcd.setCursor(5, 0);
        //lcd.print("     FBKC:");
        lcd.setCursor(0, 1);
        lcd.print("AFR: ");
        digitalWrite(13, LOW);
        readBytes = ((case4ReqDataSize - 7) / 3);
        break;
      case 5: //Air:fuel Ratio
        //lcd.setCursor(5, 0);
        //lcd.print("     FBKC:");
        lcd.setCursor(0, 1);
        lcd.print("G/s: ");
        readBytes = ((case5ReqDataSize - 7) / 3);
        break;
    }
  }

  if (timeUpdateCount == 0 || timeUpdateCount == 255) {
    updateTimeTemp();
    timeUpdateCount = 0;
  }
  timeUpdateCount++;
}

void ssmWriteSel() {
  switch (selMode)
  {
    case 1: //Fuel Economy
      writeSSM(mpgReqData, mpgReqDataSize, sendSerial);
      break;
    case 2: //IAM
      writeSSM(iamReqData, iamReqDataSize, sendSerial);
      break;
    case 3: //Miles per hour
      writeSSM(case3ReqData, case3ReqDataSize, sendSerial);
      break;
    case 4: //Air:fuel Ratio
      writeSSM(case4ReqData, case4ReqDataSize, sendSerial);
      break;
    case 5: //Fuel Economy
      writeSSM(case5ReqData, case5ReqDataSize, sendSerial);
      break;
  }
}

void lcdPrintSel() {
  switch (selMode)
  {
    case 1: //Fuel Economy
      milesPerHour = (ECUbytes[0] * 0.621371192); //P9 0x000010
      airFuelR = ((ECUbytes[3] / 128.00) * 14.7);  //P58 0x000046
      airFlowG = (((ECUbytes[1] * 256.00) + ECUbytes[2]) / 100.00); //P12 0x000013 and 0x000014
      milesPerGallon = (milesPerHour / 3600.00) / (airFlowG / (airFuelR) / 2800.00);
      lcd.setCursor(4, 1);
      if (milesPerGallon < 100) {
        lcd.print(" ");
      }
      if (milesPerGallon == 0) {
        lcd.print("0");
      }
      lcd.print(milesPerGallon, 2);
      lcd.setCursor(14, 1);
      if (milesPerGallon < 20) {
        lcd.print("=(");
      }
      else if (milesPerGallon > 20) {
        lcd.print("=D");
      }
      break;
    case 2: //IAM
      lcd.setCursor(5, 1);
      lcd.print(ECUbytes[0]);
      lcd.print(" | ");
      fbkc = ((ECUbytes[1]) - 128) / 2;
      lcd.print(fbkc); //Knock
      lcd.print(" ");
      digitalWrite(13, LOW);
      break;
    case 3: //Miles per hour
      milesPerHour = (ECUbytes[0] * 0.621371192); //P9 0x000010
      lcd.setCursor(5, 1);
      lcd.print(milesPerHour, 2);
      lcd.print("     ");
      digitalWrite(13, HIGH);
      break;
    case 4: //Air:fuel Ratio
      airFuelR = ((ECUbytes[0] / 128.00) * 14.7);  //P58 0x000046
      fbkc = ((ECUbytes[0] * 0.3515625) - 45);
      lcd.setCursor(5, 1);
      lcd.print(airFuelR, 2);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      lcd.print(fbkc, 2);
      digitalWrite(13, LOW);
      break;
    case 5: //Test case 5
      airFlowG = (((ECUbytes[0] * 256.00) + ECUbytes[1]) / 100.00);
      lcd.setCursor(4, 1);
      lcd.print(airFlowG); //G/s
      digitalWrite(13, HIGH);
      break;
    }
  }

  void updateTimeTemp() {
    theMinute = rtc.getMinute();
    theHour = rtc.getHour();
    theSecond = rtc.getSecond();
    theTemperature = ((rtc.getTemperature() * 1.8) + 15 );
    lcd.setCursor(0, 0);
    if (theHour < 10) {
      lcd.print("0");
    }
    lcd.print(theHour);
    lcd.print(":");
    if (theMinute < 10) {
      lcd.print("0");
    }
    lcd.print(theMinute);
    lcd.print(":");
    if (theSecond < 10) {
      lcd.print("0");
    }
    lcd.print(theSecond);
    lcd.setCursor(11, 0);
    lcd.print(theTemperature);
    lcd.print("*F");
  }

  /* returns the 8 least significant bits of an input byte*/
  byte CheckSum(byte sum) {
    byte counter = 0;
    byte power = 1;
    for (byte n = 0; n < 8; n++) {
      counter += bitRead(sum, n) * power;
      power = power * 2;
    }
    return counter;
  }

  /*writes data over the software serial port
  the &digiSerial passes a reference to the external
  object so that we can control it outside of the function*/
  void writeSSM(byte data[], byte length, SoftwareSerial & digiSerial) {
    //Serial.println(F("Sending packet... "));
    for (byte x = 0; x < length; x++) {
      digiSerial.write(data[x]);
    }
    //Serial.println(F("done sending."));
  }

  //this will change the values in dataArray, populating them with values respective of the poll array address calls
  boolean readECU(int* dataArray, byte dataArrayLength, boolean nonZeroes)
  {
    byte data = 0;
    boolean isPacket = false;
    byte sumBytes = 0;
    byte checkSumByte = 0;
    byte dataSize = 0;
    byte bytePlace = 0;
    byte zeroesLoopSpot = 0;
    byte loopLength = 20;
    for (byte j = 0; j < loopLength; j++)
    {
      data = sendSerial.read();
      delay(2);

      if (data == 128 && dataSize == 0) { //0x80 or 128 marks the beginning of a packet
        isPacket = true;
        j = 0;
        //Serial.println("Begin Packet");
      }

      //terminate function and return false if no response is detected
      if (j == (loopLength - 1) && isPacket != true)
      {
        return false;
      }

      if (isPacket == true && data != -1) {
        Serial.print(data); // for debugging: shows in-packet data
        Serial.print(" ");

        if (bytePlace == 3) { // how much data is coming
          dataSize = data;
          loopLength = data + 6;
        }

        if (bytePlace > 4 && bytePlace - 5 < dataArrayLength && nonZeroes == false)
        {
          dataArray[bytePlace - 5] = data;
        }
        else if (bytePlace > 4 && zeroesLoopSpot < dataArrayLength / 2 && nonZeroes == true && data != 0 && bytePlace < dataSize + 4)
        {
          dataArray[zeroesLoopSpot] = data;
          dataArray[zeroesLoopSpot + (dataArrayLength / 2)] = bytePlace;
          zeroesLoopSpot++;
        }

        bytePlace += 1; //increment bytePlace

        //once the data is all recieved, checksum and re-set counters
        // Serial.print("byte place: ");
        // Serial.println(bytePlace);
        if (bytePlace == dataSize + 5) {
          checkSumByte = CheckSum(sumBytes);  //the 8 least significant bits of sumBytes

          if (data != checkSumByte) {
            Serial.println(F("checksum error"));
            return false;
          }
          //        Serial.println("Checksum is good");

          isPacket = false;
          sumBytes = 0;
          bytePlace = 0;
          checkSumByte = 0;
          dataSize = 0;
          return true;
        }
        else {
          sumBytes += data; // this is to compare with the checksum byte
          //Serial.print(F("sum: "));
          //Serial.println(sumBytes);
        }
      }
    }
    Serial.println("");
  }

