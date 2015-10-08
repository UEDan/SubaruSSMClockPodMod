#include <SoftwareSerial.h>

//TEST UNITS
int inVal = 0;
int selMode = 1;
int ECUbytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
//4th byte is # of packets you idiot.
//double check checksum byte you jackass.
byte ReqData[31] = {128,16,240,26,168,0,0,0,16,0,0,19,0,0,70,0,1,33,0,0,100,2,9,199,2,1,104,0,0,20,130}; // add throttle
byte ReqDataSize = 31;
unsigned long prvTime;
unsigned long curTime;
int milli;
int RET00;
int RET01;
int RET02;
int RET03;
int RET04;
int RET05;
int RET06;
int RET07;
//END TEST UNITS

//Rx/Tx pins used for SSM
SoftwareSerial sendSerial = SoftwareSerial(10, 11); //Rx, Tx

void setup() {
  //TEST SETUP
  pinMode(7, INPUT);
  //END TEST SETUP
  
  //Setup Start
  Serial.begin(115200); //for diagnostics
  Serial.println("Serial Started");
    while (!Serial) {
      // wait
    }
  Serial.println("starting SSM Serial");
  sendSerial.begin(4800); //SSM uses 4800 8N1 baud rate
    while (!sendSerial) {
      //wait
      delay(50);
  }
  Serial.println("Ready!");
  delay(50);
  writeSSM(ReqData, ReqDataSize, sendSerial); //send intial SSM poll
  delay (2);
}

  
void loop() {
  /*TEST LOOP
  inVal = digitalRead(7);
  if (inVal == 0) {
    if (selMode == 10) {
      selMode = 0;
    }
    selMode++;
    //Serial.println("Mode plus");
    //printMode(selMode);
    delay(500);
  }
  */
curTime = millis();
milli=curTime - prvTime;  

if (milli > 250) {
  sendSerial.flush();
  //delay(5);
  writeSSM(ReqData, ReqDataSize, sendSerial);
  //Serial.print("Timer Popped | ");
  //Serial.println(sendSerial.available());
  prvTime=millis();
  }

  if (sendSerial.available()) {  
    readECU(ECUbytes, 8, false);
    
    prvTime = curTime;

    RET00 = (ECUbytes[0] * 0.621371192); //P9 0x000010
    RET01 = (ECUbytes[1]); //12 0x000013 bit 1of2
    RET02 = (ECUbytes[2] / 128 * 14.7); //P58 0x000046
    RET03 = (ECUbytes[3]); //0x000121
    RET04 = (ECUbytes[4]); //0x000064
    RET05 = (ECUbytes[5]); //0x0209C7
    RET06 = (ECUbytes[6]); //0x020168
    RET07 = (ECUbytes[7] / 100); //12 0x000013 bit 2of2
    
    Serial.print("MPH:");
    Serial.print(RET00);
    Serial.print(" | ");
    Serial.print("Mass airflow/s:");
    Serial.print((RET01 | RET07 << 8) / 100);
    Serial.print(" | ");
    Serial.print("AFR: ");
    Serial.print(RET02);
    Serial.print(" | ");
    Serial.print("MPG:");
    Serial.print(RET00/3600)/(RET01/(RET02)/2800);
    Serial.print(" | ");
    Serial.print("Cruise:");
    Serial.print(RET04);
    Serial.print(" | ");
    Serial.print("Defogger:");
    Serial.print(RET04);
    Serial.print(" | ");
    Serial.print("Gear:");
    Serial.print(RET05);
    Serial.print(" | ");
    Serial.print("IAM:");
    Serial.println(RET06);
    
    }
}
//TEST FUNCTION, PIN7
void printMode(int selMode) {
  switch (selMode)
  {
    case 1:
      Serial.print("This is case 1 mode ");
      Serial.println(selMode);
      digitalWrite(13, HIGH);
      break;
    case 2:
      Serial.print("This is case 2 mode ");
      Serial.println(selMode);
      digitalWrite(13, LOW);
      break;
    case 3 ... 5:
      Serial.print("This is case 3 to 5 mode ");
      Serial.println(selMode);
      digitalWrite(13, HIGH);
      break;
    case 6 ... 9:
      Serial.print("This is case 6 to 9 mode ");
      Serial.println(selMode);
      digitalWrite(13, LOW);
      break;
    case 10:
      Serial.print("This is case 10 mode ");
      Serial.println(selMode);
      digitalWrite(13, HIGH);
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
void writeSSM(byte data[], byte length, SoftwareSerial &digiSerial) {
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
        //ClrToSnd = 0;
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
