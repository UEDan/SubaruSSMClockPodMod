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
byte case1ReqData[19] = {128, 16, 240, 14, 168, 0, 0, 0, 16, 0, 0, 19, 0, 0, 20, 0, 0, 70, 179};
byte case1ReqDataSize = 19;
//variables for IAM && FBKC
byte case2ReqData[13] = {128, 16, 240, 8, 168, 0, 2, 1, 104, 2, 12, 96, 9};
byte case2ReqDataSize = 13;
//variables for ECT | IAT
byte case3ReqData[13] = {128, 16,  240, 8, 168, 0, 0, 0, 8, 0, 0, 18, 74};
byte case3ReqDataSize = 13;
//variables for AFR
byte case4ReqData[10] = {128, 16,  240, 5, 168, 0, 0, 0, 70, 115}; // AFR
byte case4ReqDataSize = 10;
//variables for Mass Air Flow G/s
byte case5ReqData[13] = {128, 16, 240, 8, 168, 0, 0, 0, 19, 0, 0, 20, 87};
byte case5ReqDataSize = 13;
//variables for Mass Air Flow G/s
byte case6ReqData[22] = {128, 16, 240, 22, 168, 0, 0, 0, 21, 0, 0, 14, 0, 0, 15, 0, 0, 19, 0, 0, 20, 151};
byte case6ReqDataSize = 22;
//4th byte is # of packets(no checksum) you idiot && double check checksum byte you jackass.

int  milli, avgmpgCount = 0, timeUpdateCount = 0, selMode = 1;
byte readBytes;
int ECUbytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prvTime, curTime, pressTime;
double milesPerHour, airFuelR, fbkc, airFlowG, lastMax, milesPerGallon, instantMPG, engineRPM, throttleAngle, calcLoad;
bool lcdBacklight;
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
  lcdBacklight = true;
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

  rtc.setClockMode(false);
  //rtc.setHour(0);
  //rtc.setMinute(12);

  //Zeros lastMax
  lastMax = 0.00;

  selMode = rtc.getYear() - 1; //Using year from RTC to save last set menu.
  lcdChange();
  //lcd.setCursor(0, 1);
  //lcd.print("MPG: ");
  //readBytes = ((case1ReqDataSize - 7) / 3);
  //writeSSM(case1ReqData, case2ReqDataSize, sendSerial); //send intial SSM poll
  ssmWriteSel(); //sends initial SSM poll
}


void loop() {
  curTime = millis();
  milli = curTime - prvTime;

  if (milli > 250) {
    //Serial.println("write");
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
    //Serial.println("read");
    readECU(ECUbytes, readBytes, false);

    prvTime = curTime;

    lcdPrintSel();

  }
  
  while (digitalRead(9) == HIGH) {
    delay(500);
    pressTime = millis();
    while (digitalRead(9) == HIGH) {
      curTime = millis();
      milli = curTime - pressTime;
      if (milli == 1500){
        lcdBacklightToggle();
      }
    }
    if (milli <= 1500) {
    lcdChange();
    }
  }

  //Mode switch read
  //if (digitalRead(9) == 1) {
  //  if (selMode >= 5) {
  //    selMode = 0;
  //  }
  //  lcdChange();
  //}

  if (timeUpdateCount == 0 || timeUpdateCount == 30) {
    updateTimeTemp();
    timeUpdateCount = 0;
  }
  timeUpdateCount++;
}

void lcdBacklightToggle() {
  if (lcdBacklight) {
    lcd.noBacklight();
  }
  else if (!lcdBacklight) {
    lcd.backlight();
  }
  lcdBacklight = !lcdBacklight;
}

void lcdChange() {
  if (selMode >= 6) {
      selMode = 0;
    }
  selMode++;
  rtc.setYear(selMode);
  //Serial.println("Mode plus");
  //printMode(selMode);
  lcd.clear();
  switch (selMode)
  {
    case 1: //Fuel Economy
      lcd.setCursor(0, 1);
      lcd.print("MPG:");
      digitalWrite(13, HIGH);
      readBytes = ((case1ReqDataSize - 7) / 3);
      break;
    case 2: //IAM && Knock
      lcd.setCursor(0, 1);
      lcd.print("IAM: ");
      digitalWrite(13, LOW);
      readBytes = ((case2ReqDataSize - 7) / 3);
      break;
    case 3: //ECT | IAT
      digitalWrite(13, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("ECT: ");
      lcd.setCursor(7, 1);
      lcd.print("C|IAT:");
      lcd.setCursor(15, 1);
      lcd.print("C");
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
    case 5: //MAF G/s
      //lcd.setCursor(5, 0);
      //lcd.print("     FBKC:");
      lcd.setCursor(0, 1);
      lcd.print("MAF: ");
      //lcd.setCursor(10, 1);
      //lcd.print("G/s"); //now printer at case5
    //Zeros lastMax
    lastMax = 0.00;
      readBytes = ((case5ReqDataSize - 7) / 3);
      break;
    case 6: //Calculated Load | Throtle Position
      //Zeros lastMax
      lastMax = 0.00;
      lcd.setCursor(4, 1);
    //division lines and %
      lcd.print("      |    %");
      readBytes = ((case6ReqDataSize - 7) / 3);
      break;
  }
}

void ssmWriteSel() {
  switch (selMode)
  {
    case 1: //Fuel Economy
      writeSSM(case1ReqData, case1ReqDataSize, sendSerial);
      break;
    case 2: //IAM
      writeSSM(case2ReqData, case2ReqDataSize, sendSerial);
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
      airFlowG = (((ECUbytes[1] * 256.00) + ECUbytes[2]) / 100.00); //P12 0x000013 and 0x000014
      airFuelR = ((ECUbytes[3] / 128.00) * 14.7);  //P58 0x000046
      instantMPG = (milesPerHour / 3600.00) / (airFlowG / (airFuelR) / 2800.00);
      mpgAvg(); //Average or 10 mpg polls
      break;
    case 2: //IAM && knock?
      lcd.setCursor(5, 1);
      lcd.print(ECUbytes[0]);
      lcd.print(" | ");
      fbkc = (((ECUbytes[1]) * 0.3515625) - 45);
      if (fbkc >= 0) {
        lcd.print(" ");
      }
      lcd.print(fbkc, 2);
      digitalWrite(13, LOW);
      break;
    case 3: //ECT | IAT
      int engineCoolantTemp;
      //milesPerHour = (ECUbytes[0] * 0.621371192); //P9 0x000010
      //print ECT
      engineCoolantTemp = ECUbytes[0] - 40;
      lcd.setCursor(4, 1);
      if (engineCoolantTemp < 100) {
        lcd.print(" ");
        if (engineCoolantTemp < 10) {
          lcd.print(" ");
        }
      }
      lcd.print(engineCoolantTemp);
      //print IAT
      lcd.setCursor(13, 1);
      if (ECUbytes[1] < 50) {
        lcd.print(" ");
      }
      lcd.print(ECUbytes[1] - 40);
      //lcd.print(milesPerHour, 2);
      digitalWrite(13, HIGH);
      break;
    case 4: //IAM && knock?
      airFuelR = ((ECUbytes[0] / 128.00) * 14.7);  //P58 0x000046
      lcd.setCursor(5, 1);
      lcd.print(airFuelR, 2);
      lcd.print("  ");
      digitalWrite(13, LOW);
      break;
    case 5: //Mass air flow(G/s)
      airFlowG = (((ECUbytes[0] * 256.00) + ECUbytes[1]) / 100.00);
      lcd.setCursor(4, 1);
      if (airFlowG < 100) {
        lcd.print(" ");
      }
      if (airFlowG < 10) {
        lcd.print(" ");
      }
      lcd.print(airFlowG, 1); //G/s
    if (airFlowG >= lastMax) {
      lastMax = airFlowG;
      lcd.print("|");
      lcd.print(lastMax, 0);
      lcd.print("G/s");
    }
      digitalWrite(13, HIGH);
      break;
    case 6: //Calculated Load | Throttle
    //Calucated load = (airFlowG*60)/engineRPM
      airFlowG = (((ECUbytes[3] * 256.00) + ECUbytes[4]) / 100.00); // MAF
    engineRPM = (((ECUbytes[2] * 256.00) + ECUbytes[2]) / 4.00); // RPM
    calcLoad = (airFlowG * 60.00) / engineRPM;
      lcd.setCursor(0, 1);
      lcd.print(calcLoad, 2);
    if (calcLoad >= lastMax) {
      lastMax = calcLoad;
      lcd.print("|");
      lcd.print(lastMax, 2);
    }
    throttleAngle = (ECUbytes[0] * 100.00) / 255.00;
      lcd.setCursor(12, 1);
    if (throttleAngle < 100) {
        lcd.print(" ");
      }
      if (throttleAngle < 10) {
        lcd.print(" ");
      }
    lcd.print(throttleAngle, 0);
      digitalWrite(13, LOW);
      break;
  }
}

void updateTimeTemp() {
  byte theHour, theMinute, theSecond, theTemperature; //DS3231 Parameters
  theMinute = rtc.getMinute();
  theHour = rtc.getHour();
  theSecond = rtc.getSecond();
  theTemperature = ((rtc.getTemperature() * 1.8) + 20 );
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

void mpgAvg() {
  milesPerGallon += instantMPG;
  avgmpgCount++;

  if (avgmpgCount == 3) {
    milesPerGallon /= 3.00; //Average after 3 instant polls

    lcd.setCursor(4, 1);
    if (milesPerGallon < 100) {
      if (milesPerGallon < 10) {
        lcd.print(" ");
      }
      lcd.print(" ");
    }
    lcd.print(milesPerGallon, 2);
    lcd.setCursor(14, 1);
    if (milesPerGallon < 20) {
      lcd.print("=(");
    }
    else if (milesPerGallon > 20) {
      lcd.print("=D");
    }
    avgmpgCount = 0; //Reset counter
    milesPerGallon = 0.00;
  }
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
      //Serial.print(data); // for debugging: shows in-packet data
      //Serial.print(" ");

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
  //Serial.println("");
}