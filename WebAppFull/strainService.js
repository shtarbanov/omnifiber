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
let a5l, a3l, a2l, a1l;
let p5equiv, p3equiv, p2equiv, p1equiv;
let p5internal=14.7, p3internal=14.7, p2internal=14.7, p1internal=14.7;

let L0=30; //mm

function toVoltage(rawVal){
  let volts=0;
  volts = 0.6226 + rawVal*(2.34375-0.6226)/255
  return volts;
}

function to_dL(dv,v0){
  let dL_over_L0 = 0;
  dL_over_L0 = 0.743185924088565*Math.log(dv/v0+ 1.0) * L0;
  return dL_over_L0;
}

function to_L(dv,v0){
  let L_final = 0;
  L_final = ( 0.743185924088565*Math.log(dv/v0+ 1.0) + 1) * L0;
  if(L_final > L0){
    return L_final;
  }
  else{
    return L0;
  }
}
function to_P(L){
  let p = -287.520387619287 + 22.0592080448425*L - 0.552621663730145*Math.pow(L,2) + 0.00467055346117551*Math.pow(L,3)+11.5;
  return p;
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

      a5dl=to_dL(a5dv,a5V0).toFixed(4);
      a3dl=to_dL(a3dv,a3V0).toFixed(4);
      a2dl=to_dL(a2dv,a2V0).toFixed(4);
      a1dl=to_dL(a1dv,a1V0).toFixed(4);

      a5l=to_L(a5dv,a5V0).toFixed(4);
      a3l=to_L(a3dv,a3V0).toFixed(4);
      a2l=to_L(a2dv,a2V0).toFixed(4);
      a1l=to_L(a1dv,a1V0).toFixed(4);

      p5equiv = to_P(a5l);
      p3equiv = to_P(a3l);
      p2equiv = to_P(a2l);
      p1equiv = to_P(a1l);

      document.querySelector('#strainVal').innerHTML = 'pin.29.(A5): ' + a5 + '<br>pin.05.(A3): ' + a3 + '<br>pin.04.(A2): ' + a2 + '<br>pin.03.(A1): ' + a1
      document.querySelector('#voltageVal').innerHTML = 'A5: ' + toVoltage(a5).toFixed(4) + '<br>A3: ' + toVoltage(a3).toFixed(4) + '<br>A2: ' + toVoltage(a2).toFixed(4) + '<br>A1: ' + toVoltage(a1).toFixed(4);
      document.querySelector('#dvVal').innerHTML = 'A5: ' + a5dv + '<br>A3: ' + a3dv + '<br>A2: ' + a2dv +  '<br>A1: ' + a1dv;
      document.querySelector('#dlVal').innerHTML = 'A5: ' + a5dl + '<br>A3: ' + a3dl + '<br>A2: ' + a2dl +  '<br>A1: ' + a1dl;
      document.querySelector('#lVal').innerHTML = 'A5: ' + a5l + '<br>A3: ' + a3l + '<br>A2: ' + a2l +  '<br>A1: ' + a1l;
      document.querySelector('#pVal').innerHTML = 'A5: ' + p5equiv + '<br>A3: ' + p3equiv + '<br>A2: ' + p2equiv +  '<br>A1: ' + p1equiv;

      try{
        if(p1equiv>(p1internal+0.25)){
          startInflationOnPort(0x01, 0xff);
          p1internal = pressureValueFloat;
          log("P1: ");
          log(p1internal);
        }
        else if(p3equiv>(p3internal+0.25)){
          startInflationOnPort(0x03, 0xff);
          p3internal = pressureValueFloat;
          log("P3: ");
          log(p3internal);
        }
        else if(p1equiv<(p1internal-0.25)){
          startReleaseOnPort(0x01);
          p1internal = pressureValueFloat;
        }
        else if(p3equiv<(p3internal-0.25)){
          startReleaseOnPort(0x03);
          p3internal = pressureValueFloat;
        }
        else{
          stopAllActions();
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
