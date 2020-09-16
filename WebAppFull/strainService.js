/*
  This example reports strain changes. It uses an event listener that triggers whenver
  the BLE device sends a notification from the strain characteristic. It also shows the
  time when the event has occurred. There are many other googies you can find in the 'event' object,
  which is returned when the event fires.
*/
'use strict'
const strainServiceUUID = '0b0b0b0b-0b0b-0b0b-0b0b-00000000aa10';
const chrStrainUUID     = '0b0b0b0b-0b0b-0b0b-0b0b-c1000000aa10';

let strainService;
let chrStrain;
let isIdle = true;
let areInitialValuesStored = false;
//let a5V0,a3V0,a2V0,a1V0; //initial voltages
let a5V0,a3V0,a2V0,a1V0;
let a5dv,a3dv,a2dv,a1dv;
let a5dl,a3dl,a2dl,a1dl;

let L0=35; //mm

function toVoltage(rawVal){
  let volts=0;
  volts = 0.6226 + rawVal*(2.34375-0.6226)/255
  return volts;
}
function to_dL(dv,v0){
  let dL_over_L0 = 0;
  dL_over_L0 = 8.033*(Math.exp(dv/v0)-1);
  return dL_over_L0*L0;
}
async function initStrainService(){
  try{
    strainService = await bleServer.getPrimaryService(strainServiceUUID);
    chrStrain = await strainService.getCharacteristic(chrStrainUUID);
    //Subscribe to receive notifications characteristic
    await chrStrain.startNotifications();
    chrStrain.addEventListener('characteristicvaluechanged', event => { //an event is returned
      let a5 = event.target.value.getUint8(0);//A5
      let a3 = event.target.value.getUint8(1); //A3
      let a2 = event.target.value.getUint8(2); //A2
      let a1 = event.target.value.getUint8(3); //A1
      if(areInitialValuesStored==false){
        areInitialValuesStored=true;
        //a5V0=toVoltage(a5); a3V0=toVoltage(a3); a2V0=toVoltage(a2); a1V0=toVoltage(a1);
        a5V0=toVoltage(a5).toFixed(4);
        a3V0=toVoltage(a3).toFixed(4);
        a2V0=toVoltage(a2).toFixed(4);
        a1V0=toVoltage(a1).toFixed(4);
      }
      //Step1: convert to voltage readings.
      //Step2: subtract the rest state.
      a5dv=(toVoltage(a5)-a5V0).toFixed(4);
      a3dv=(toVoltage(a3)-a3V0).toFixed(4);
      a2dv=(toVoltage(a2)-a2V0).toFixed(4);
      a1dv=(toVoltage(a1)-a1V0).toFixed(4);


      a5dl=to_dL(a5dv,a5V0);
      a3dl=to_dL(a3dv,a3V0);
      a2dl=to_dL(a2dv,a2V0);
      a1dl=to_dL(a1dv,a1V0);

      document.querySelector('#strainVal').innerHTML = 'pin.29.(A5): ' + a5 + '<br>pin.05.(A3): ' + a3 + '<br>pin.04.(A2): ' + a2 + '<br>pin.03.(A1): ' + a1
      document.querySelector('#voltageVal').innerHTML = 'A5: ' + toVoltage(a5).toFixed(4) + '<br>A3: ' + toVoltage(a3).toFixed(4) + '<br>A2: ' + toVoltage(a2).toFixed(4) + '<br>A1: ' + toVoltage(a1).toFixed(4);
      document.querySelector('#dvVal').innerHTML = 'A5: ' + a5dv + '<br>A3: ' + a3dv + '<br>A2: ' + a2dv +  '<br>A1: ' + a1dv;
      document.querySelector('#dlVal').innerHTML = 'A5: ' + a5dl.toFixed(4) + '<br>A3: ' + a3dl.toFixed(4) + '<br>A2: ' + a2dl.toFixed(4) +  '<br>A1: ' + a1dl.toFixed(4);
      //document.querySelector('#lVal').innerHTML = 'A5: ' + a5dl+L0 + '<br>A3: ' + a3dl+L0 + '<br>A2: ' + a2dl+L0 +  '<br>A1: ' + a1dl+L0;


      try{
        //commandArray = new Uint8Array(3); this is defined in the controlService.js
        if(a1>100 && a1<200){
          commandArray[0] = 0x2b; //'+'
          commandArray[1] = 0x01;
          commandArray[2] = a1;
          //chrCommand.writeValue(commandArray);
          console.log("sending PWM command");
          isIdle=false;
        }
        else{
          if(isIdle==false){
            commandArray[0] = 0x21; //'!' stop
            commandArray[1] = 0xff; //all ports
            commandArray[2] = 0xff;
            console.log("sending STOP command");
            //chrCommand.writeValue(commandArray);
            isIdle=true;
          }
        }

      }catch(error){
        if(error.message!="GATT operation already in progress.") log(error);
      }



    });
    log("Strain Service Initialized");
    //To print the strain, we simply make a read request, and that triggers
    //a notification to be sent by the device. So we don't even need to capture the
    //returned value to display it manually; just reading it is enough.
    getStrain();
  }
  catch(error){
    log("Strain Error: " + error); //NOTE: When we catch the error, execution will
    //continue and this error will not be visible by anyone who called initStrainService().
    //Thus, to make the caller aware that initStrainService() gave an arror, we must
    ///raise owr own error here.
    throw "ERROR: initStrainService() failed.";
    //Anything we put here after "throw" line will not get executed.
  }
}

async function getStrain(){
  if (bleDevice && bleDevice.gatt.connected){
    await chrStrain.readValue(); //returns a DataView but we don't need it
    //because it also triggers the 'characteristicvaluechanged' notification.
  }
  else log("Device not connected");
}
