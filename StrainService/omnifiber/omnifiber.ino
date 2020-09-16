#include <bluefruit.h>
#define DEVICE_NAME "FlowIO_strn"// Device Name: Maximum 30 bytes

BLEService strainService;
BLECharacteristic chrStrain;

//##############################################
uint8_t pinA = A0;
uint8_t pinB = A1;
uint8_t pinC = A2;
uint8_t pinD = A3;
uint8_t samplingInterval=10;
uint8_t numberOfSamples=30;
//##############################################

float sampleAf;
float sampleBf;
float sampleCf;
float sampleDf;
uint8_t sampleA; //these will hold the final averaged values.
uint8_t sampleB;
uint8_t sampleC;
uint8_t sampleD;

uint32_t averagedVoltages32bit; //this holds the combination of the final 4 samples.

void setup(){
  Serial.begin(115200);
  analogReference(AR_INTERNAL_3_0);
  analogReadResolution(12);
  Bluefruit.autoConnLed(true);   // Setup the BLE LED to be enabled on CONNECT
  //All Bluefruit.config***() function must be called before Bluefruit.begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX); // Config the peripheral connection with maximum bandwidth
  Bluefruit.configUuid128Count(15); //by default this is 10, and we have more than 10 services & characteristics on FlowIO
  Bluefruit.begin();
  Bluefruit.setTxPower(4); // Set max power. Accepted values: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setName(DEVICE_NAME);
  Bluefruit.Periph.setConnectCallback(connect_callback);

  createStrainService(); //this is defined in "strainService.ino"
  startAdvertising();   // Set up and start advertising
}
void loop(){
  readAveragedSamplesOnPins();
  averagedVoltages32bit = convertToUint32(sampleA,sampleB,sampleC,sampleD);
  if(Bluefruit.connected()){
    chrStrain.notify32(averagedVoltages32bit);        
  }
  delay(2);
}

void createStrainService(){
  const uint8_t strainServiceUUID[16]     = {0x10,0xaa,0x00,0x00,0x00,0x00,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b}; //"0b0b0b0b-0b0b-0b0b-0b0b-00000000aa01"
  const uint8_t chrStrainUUID[16] = {0x10,0xaa,0x00,0x00,0x00,0xc1,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b}; //"0b0b0b0b-0b0b-0b0b-0b0b-c1000000aa01"

  strainService = BLEService(strainServiceUUID);
  strainService.begin();

  chrStrain = BLECharacteristic(chrStrainUUID);
  chrStrain.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY); 
  chrStrain.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    //For the permission, the first parameter is the READ, second the WRITE permission
  chrStrain.setFixedLen(4);
  chrStrain.begin();
}

void startAdvertising(void){
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);   // Advertising packet
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.restartOnDisconnect(true); //Restart advertising on disconnect.
  Bluefruit.ScanResponse.addName();   // Secondary Scan Response packet (optional)
  Bluefruit.Advertising.addService(strainService);   //advertise service uuid

  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode

  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void connect_callback(uint16_t conn_handle){ // callback invoked when central connects
  chrStrain.notify32(averagedVoltages32bit);      
  //Serial.println("Strain value sent.");
}
uint32_t convertToUint32(uint8_t a3, uint8_t a2, uint8_t a1, uint8_t a0){
  uint32_t var;
  var = a3;   //byte 3
  var <<= 8;
  var |= a2;  //byte 2
  var <<= 8;
  var |= a1;  //byte 1
  var <<= 8;
  var |= a0;  //byte 0
  return var;
}
void readAveragedSamplesOnPins(){
  for(int i=0; i<numberOfSamples; i++){
    sampleAf = sampleAf + get8bitSample(pinA);
    sampleBf = sampleBf + get8bitSample(pinB);
    sampleCf = sampleCf + get8bitSample(pinC);
    sampleDf = sampleDf + get8bitSample(pinD);
    delay(samplingInterval);
  }
  sampleAf = sampleAf/numberOfSamples;
  sampleBf = sampleBf/30.0;
  sampleCf = sampleCf/30.0;
  sampleDf = sampleDf/30.0;
  sampleA = sampleAf;
  sampleB = sampleBf;
  sampleC = sampleCf;
  sampleD = sampleDf;
//  Serial.print(sampleAf);
//  Serial.print("\t <---> \t");
//  Serial.println(sampleA);
//  Serial.println("-------------------");
}

uint8_t get8bitSample(uint8_t pin){
  uint16_t sample16 = analogRead(pin);
  uint8_t sample8 = mapto8bit_val850to3200(sample16);
//  Serial.print(sample16);
//  Serial.print("\t <---> \t");
//  Serial.println(sample8);
//  delay(500);
  return sample8;
}

uint8_t mapto8bit_val850to3200(uint16_t val850to3200){ //the input is assumed to be b/n 0 and 4095 (12-bit only)
  float val0to255float = (val850to3200-850)*(255-0)/(3200-850)+0;
  return (uint8_t) (val0to255float+0.5); //casting to 8bit with rounding.
}
