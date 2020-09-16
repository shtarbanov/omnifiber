#include <bluefruit.h>
#define DEVICE_NAME "FlowIO_strn"// Device Name: Maximum 30 bytes

BLEService strainService;
BLECharacteristic chrStrain;

//##############################################
uint8_t pinA = A1; //3; //A1
uint8_t pinB = A2; //4; //A2
uint8_t pinC = A3; //5; //A3
uint8_t pinD = A5; //29; //A5
uint8_t samplingInterval=30; //interval b/n consecutive samples in the running average.
const uint8_t N=30; //number of samples in running average.
//##############################################

float sampleAfloat;
float sampleBfloat;
float sampleCfloat;
float sampleDfloat;
uint8_t sampleA; //these will hold the final averaged values.
uint8_t sampleB;
uint8_t sampleC;
uint8_t sampleD;
uint8_t lastNsamplesA[N]={0};
uint8_t lastNsamplesB[N]={0};
uint8_t lastNsamplesC[N]={0};
uint8_t lastNsamplesD[N]={0};

uint8_t runningIndex=0;

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

  for(int i=0; i<N; i++){
    lastNsamplesA[i]=get8bitSample(pinA);
    lastNsamplesB[i]=get8bitSample(pinB);
    lastNsamplesC[i]=get8bitSample(pinD);
    lastNsamplesD[i]=get8bitSample(pinD);
  }
  runningIndex=0;
}
void loop(){
  if(runningIndex==N) runningIndex=0;
  lastNsamplesA[runningIndex]=get8bitSample(pinA);
  lastNsamplesB[runningIndex]=get8bitSample(pinB);
  lastNsamplesC[runningIndex]=get8bitSample(pinC);
  lastNsamplesD[runningIndex]=get8bitSample(pinD);
  runningIndex++;

  sampleAfloat=0;
  sampleBfloat=0;
  sampleCfloat=0;
  sampleDfloat=0;
  for(int i=0; i<N; i++){
    sampleAfloat = sampleAfloat + lastNsamplesA[i];
    sampleBfloat = sampleBfloat + lastNsamplesB[i];
    sampleCfloat = sampleCfloat + lastNsamplesC[i];
    sampleDfloat = sampleDfloat + lastNsamplesD[i];
  }
  sampleAfloat = sampleAfloat/N;  
  sampleBfloat = sampleBfloat/N;  
  sampleCfloat = sampleCfloat/N;  
  sampleDfloat = sampleDfloat/N;  
  sampleA = sampleAfloat;
  sampleB = sampleBfloat;
  sampleC = sampleCfloat;
  sampleD = sampleDfloat;
  
  averagedVoltages32bit = convertToUint32(sampleA,sampleB,sampleC,sampleD);
  if(Bluefruit.connected()){
    chrStrain.notify32(averagedVoltages32bit);        
  }
  delay(samplingInterval);
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
