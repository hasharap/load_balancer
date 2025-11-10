const { SerialPort } = require('/usr/lib/node_modules/serialport');
const { ByteLengthParser } = require('/usr/lib/node_modules/@serialport/parser-byte-length');
const { crc16 } = require('easy-crc');
const conv = require('hex2dec');
const EventEmitter = require('events');

const { DisplayManager, liveDMGLeft, liveDMGRight, netDMGLeft, netDMGRight, portL2 } = require('./display.js');
const display = new DisplayManager();
const parserFixLenL2 = portL2.pipe(new ByteLengthParser({ length: 1 }));

const portFC = new SerialPort({ path: '/dev/ttyS1', baudRate: 9600 });
const parserFixLenFC = portFC.pipe(new ByteLengthParser({ length: 1 }));

const { logger } = require('./log.js');
const { PipeLog } = require('./pipelogs');
const config = require('../config.json');
const { exec } = require("child_process");
const pipe = new PipeLog();

const stateNames = {
  0: 'POWER ON',
  1: 'A1',
  2: 'A2',
  3: 'B1',
  4: 'B2',
  5: 'C1',
  6: 'C2',
  7: 'D',
  8: 'F',
  9: 'TEMP_STATE_F'
};

const errNames = {
  0: "",
  1: "Stuck Contact",
  2: "GFI Test Error",
  3: "GFI Error",
  4: "Modbus Error",
  5: "Over Current Fault",
  6: "Phase Loss",
  7: "Over Temp",
  8: "CP Fault",
  9: "Diode failure",
  10: "Over Voltage",
  11: "Under Voltage",
  12: "Freq Fault",
  13: "Diode Faliure",
  20: "MCU Param Missmatch",
  21: "Emergency Stop"
};

const netRequestNames = [
  "",
  "Update Alarm Complete",
  "Update Complete",
  "Charge Pause",
  "Vehicle Check",
  "Shedule Charge",
  "Stop Charge",
  "Start"
];

const genErrNames = [
  "L2 is not communicating via serial1 bus",
  "Some error",
  "Network Unavilable"
];

class MCUManager extends EventEmitter {
  constructor() {
    super();
    this.setup();

    this.intervalIdL2 = null;
    this.intervalIdFC = null;

    this.toggleValueL2 = 0;
    this.toggleValueFC = 0;
    this.messageBufferL2 = Buffer.alloc(0);
    this.messageBufferFC = Buffer.alloc(0);
    this.isAccumulatingL2 = false;
    this.isAccumulatingFC = false;
    this.isAccumulatingL2Second = false;
    this.isAccumulatingFCSecond = false;

    this.voltL2 = 0;
    this.currL2 = 0;
    this.powrL2 = 0;
    this.kwhL2 = 0;
    this.v1L2 = 0;
    this.v2L2 = 0;
    this.v3L2 = 0;
    this.tempL2 = 0;
    this.frqL2 = 0;
    this.cpval = 0;
    this.em1L2 = 0;
    this.em2L2 = 0;
    this.em3L2 = 0;
    this.em4L2 = 0;

    this.voltFC = 0;
    this.currFC = 0;
    this.powrFC = 0;
    this.kwhFC = 0;
    this.socFC = 0;

    this.en_1 = 0;
    this.en_2 = 0;
    this.uv_upper = 0;
    this.uv_lower = 0;
    this.ov_upper = 0;
    this.ov_lower = 0;
    this.freq_upper = 0;
    this.freq_lower = 0;
    this.max_current = 0;
    this.controller ;
    this.update_decision_path;
    this.paramVerifiedG1 = false;
    this.paramVerifiedG2 = false;
    this.paramVerifiedG3 = false;
    this.parmAllVerified = false;
    this.enableConfigFlagReceived = false;
    this.parmCheckComplete = false 

    this.stateL2 = "IDLE";
    this.errorL2 = "";
    this.stateFC = "IDLE";
    this.errorFC = "";
    this.powerErrorL2 = 0;
    this.powerErrorFC = 0;
    this.totalBufOut = "";

    this.pageL2 = 0;
    this.pageFC = 0;
    
    this.prevPageL2 = 0;
    this.prevPageFC = 0;
    
    this.estop = false;

  };

  startTimer(parserType) {
    if (this[parserType]) {
      clearInterval(this[parserType]);
    }

    this[parserType] = setInterval(() => {
      this.emit('noData', { parserType });
    }, 30000);
  }

  dec2bin(dec) {
    return dec.toString(2).padStart(8, '0');
  };

  bin2dec(bin) {
    return parseInt(bin, 2);
  };

  mcuMsgDecodeL2(packet) {
    this.accumulateMessageL2(packet);
  };

  mcuMsgDecodeFC(packet) {
    this.accumulateMessageFC(packet);
  };


  extractKWH(buffer, index) {
    if (index + 2 >= buffer.length) {
      console.error('Index out of bounds');
      return buffer;
    }

    /*Swap the bytes*/
    const temp = buffer[index];
    buffer[index] = buffer[index + 2];
    buffer[index + 2] = temp;
    return buffer;

  };

  mapData(topic, state, activityState, netRequest, powerError, generalError, volt, curr, powr, energy, soc, cpval, tempL2, frqL2, v1L2, v2L2, v3L2 , netCmd,msgId, message, bufOut) {
    const stateName = stateNames[state] || 'F';
    const activityStateName = {
      "ConnectorState": activityState[0],
      "PWMState": activityState[1],
      "ChargingState": activityState[2]
    };
    let indexes = [...netRequest].map((e, i) => e === '1' ? i : -1).filter(i => i !== -1);
    const netRequestName = indexes.map(index => netRequestNames[index]);

    const errName = errNames[powerError] || "";

    const genPosition = generalError.indexOf('1');
    const genName = genErrNames[genPosition] || "";

    this.emit(topic, {
      "volt": volt,
      "cpval": cpval,
      "curr": curr,
      "powr": powr,
      "energy": energy,
      "state": stateName,
      "activityState": activityStateName,
      "netRequest": netRequestName,
      "powerError": errName,
      "generalError": genName,
      "soc": soc,
      "msgId": msgId,
      "activityStateRaw": activityState,
      "message": message,
      "bufOut": bufOut,
      "tempL2":tempL2,
      "frqL2":frqL2,
      "v1L2":v1L2,
      "v2L2":v2L2,
      "v3L2":v3L2,
      "netCmd":netCmd
    });
  };

  async extractDataFromMessage(message) {
    const sender = message[1];
    const decVal = message.readUInt8(2);
    const binVal = this.dec2bin(decVal);

    const chargerId = String.fromCharCode(sender);
    const msgId = message.readUInt8(9);
    const state = this.bin2dec(binVal.slice(3, 8));
    const activityState = binVal.slice(0, 3);
    const netRequest = this.dec2bin(message.readUInt8(4));
    const generalError = '000';
    
    console.log("************************************");
    console.log(chargerId,liveDMGLeft.page,this.prevPageL2,liveDMGRight.page,this.prevPageFC,this.estop,this.powerErrorL2,message.readUInt8(8),this.parmCheckComplete);
    console.log("************************************");
    
    
   

    switch (chargerId) {
      case 'C': 
        switch (msgId) {
          case 0:
            const voltL2V1 = message.readUInt16LE(10);
            const voltL2V2 = message.readUInt16LE(12);
            const voltL2V3 = message.readUInt16LE(14);
            this.voltL2 = Math.max(voltL2V1, voltL2V2, voltL2V3);
            this.v1L2 = voltL2V1;
            this.v2L2 = voltL2V2;
            this.v3L2 = voltL2V3;
            break;

          case 1:
            const currL2I1 = message.readUInt16LE(10);
            const currL2I2 = message.readUInt16LE(12);
            const currL2I3 = message.readUInt16LE(14);
            this.currL2 = Math.max(currL2I1, currL2I2, currL2I3);
            break;

          case 2:
            this.powrL2 = message.readUInt8(10);
            this.kwhL2 = conv.hexToDec(this.extractKWH(message.slice(11, 14), 0).toString('hex'));
            this.tempL2 = message.readUInt8(14);
            this.frqL2 = message.readUInt8(15);
            break;
          
          case 3:
            this.cpval = message.readUInt16LE(10);
            this.em1L2 = message.readUInt8(12);
            this.em2L2 = message.readUInt8(13);
            this.em3L2 = message.readUInt8(14);
            this.em4L2 = message.readUInt8(15);
            break;

          case 45:
            this.enableConfigFlagReceived = true;
            break;
        
          case 46:
            const en_1 = message.readUInt16LE(10);
            const en_2 = message.readUInt16LE(12);
            const uv_upper = message.readUInt16LE(14);
            console.log(`  en_1 \x1b[${en_1 == this.en_1 ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.en_1} mcu:${en_1}\x1b[00m`);
            console.log(`  en_2 \x1b[${en_2 == this.en_2 ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.en_2} mcu:${en_2}\x1b[00m`);
            console.log(`  uv_upper \x1b[${uv_upper == this.uv_upper ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.uv_upper} mcu:${uv_upper}\x1b[00m`);
            this.checkParameters(1,en_1,en_2,uv_upper);
            break;
            
          case 47:
            const uv_lower = message.readUInt16LE(10);
            const ov_upper = message.readUInt16LE(12);
            const ov_lower = message.readUInt16LE(14);
            console.log(`  uv_lower \x1b[${uv_lower == this.uv_lower ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.uv_lower} mcu:${uv_lower}\x1b[00m`);
            console.log(`  ov_upper \x1b[${ov_upper == this.ov_upper ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.ov_upper} mcu:${ov_upper}\x1b[00m`);
            console.log(`  ov_lower \x1b[${ov_lower == this.ov_lower ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.ov_lower} mcu:${ov_lower}\x1b[00m`);
            this.checkParameters(2,uv_lower,ov_upper,ov_lower);
            break;
            
          case 48:
            const freq_upper = message.readUInt16LE(10);
            const freq_lower = message.readUInt16LE(12);
            const max_current = message.readUInt8(14);
            console.log(`  freq_upper \x1b[${freq_upper== this.freq_upper ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.freq_upper} mcu:${freq_upper}\x1b[00m`);
            console.log(`  freq_lower \x1b[${freq_lower == this.freq_lower ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.freq_lower} mcu:${freq_lower}\x1b[00m`);
            console.log(`  max_current \x1b[${max_current == this.max_current ? '32m verified \u2714' : '31m not verified \u2716'} con:${this.max_current} mcu:${max_current}\x1b[00m`);
            this.checkParameters(3,freq_upper,freq_lower,max_current);
            break;
        
          default:
            logger.info(`Unknown MID L2 ${msgId}`);
        }
        
        
        /*checking parameters on progress*/
        if(!this.parmCheckComplete){
            
          this.parmAllVerified = (this.paramVerifiedG1 && this.paramVerifiedG2 && this.paramVerifiedG3) ? true : false ;
  
          console.log(`\x1b[33mChecking Parameters ${this.parmAllVerified}....\x1b[00m`);
          logger.info(`Checking Parameters.... ${this.parmAllVerified}`);

          if(this.parmAllVerified){
            console.log(`\x1b[33mAll MCU Parameters Verified !!\x1b[00m`);
            logger.info(`All MCU Parameters Verified !!`);
            this.enableConfigFlagReceived = false;
          }
        }
        else{
          if(!this.parmAllVerified){
            console.log(`\x1b[31mMCU Parameters Verification fail !!\x1b[00m`);
            logger.info(`MCU Parameters Verification fail !!`);
            this.enableConfigFlagReceived = false;
            this.powerErrorL2 = 20;
          }
        }

        /*check emergency button pressed flag and L2 parameter unverified flag are enabled
        20 - Parameter missmatch affect to only L2 side, check from L2 error message
        21 - emergency pressed affect to bothside, check from FC error message
        */
        this.powerErrorL2 = this.powerErrorFC == 21 ? 21 : this.powerErrorL2 == 20 ? message.readUInt8(6) == 0 ? 20 : message.readUInt8(6) : message.readUInt8(6);
        
        // console.log("L2Data", state, activityState, netRequest, powerError, generalError, this.voltL2, this.currL2, this.powrL2, this.kwhL2);
        this.mapData("L2Data", state, activityState, netRequest, this.powerErrorL2, generalError, this.voltL2, this.currL2, this.powrL2, this.kwhL2, 0, this.cpval, this.tempL2, this.frqL2, this.v1L2, this.v2L2, this.v3L2, this.stateL2, msgId, message.toString('hex'), this.totalBufOut);
        break;

      case 'c':
        switch (msgId) {
          case 0:
            this.voltFC = message.readUInt16LE(10);
            this.currFC = message.readUInt16LE(12);
            this.powrFC = message.readUInt16LE(14);
            break;
          case 1:
            this.kwhFC = message.readUInt16LE(10);
            this.t1FC = message.readUInt8(12);
            this.t2FC = message.readUInt8(13);
            this.t3FC = message.readUInt8(14);
            break;
          case 2:
            this.socFC = message.readUInt8(13);
            break;
        
          default:
            logger.info(`Unknown MID FC ${msgId}`);
            break;
        }
        
        /*addressing emergency press comes with FC data*/ 
        if(message.readUInt8(8) == 1){
            
            if(!this.estop){
                this.prevPageL2 = this.pageL2;
                this.prevPageFC = this.pageFC;
                this.estop = true;
                }
            
            this.powerErrorL2 = 21;
            this.powerErrorFC = 21;
        }
        else{
            if(this.estop){
                this.estop = false;
                liveDMGLeft.page = this.prevPageL2 ;
                liveDMGRight.page = this.prevPageFC ;
                this.pageL2 = this.prevPageL2 ;
                this.pageFC = this.prevPageFC ;
                await display.pageUpdateDMG('L', this.prevPageL2);
                await this.delay(200);
                await display.pageUpdateDMG('R', this.prevPageFC);
                await this.delay(200);
                
                }
            this.powerErrorFC = message.readUInt8(6);
        }
        
        // console.log("FCData", state, activityState, netRequest, powerError, generalError, this.voltFC, this.currFC, this.powrFC, this.kwhFC, this.socFC);
        this.mapData("FCData", state, activityState, netRequest, this.powerErrorFC, generalError, this.voltFC, this.currFC, this.powrFC, this.kwhFC, this.socFC,0,0,0,0,0,0,this.stateFC, msgId, message.toString('hex'), this.totalBufOut);
        break;
    }
  }

  checkParameters(parmGroup,p1,p2,p3){
    switch (parmGroup){
      case 1:
          this.paramVerifiedG1 = (p1 == this.en_1) && (p2 == this.en_2) && (p3 == this.uv_upper);
          break;
      case 2:
          this.paramVerifiedG2 = (p1 == this.uv_lower) && (p2 == this.ov_upper) && (p3 == this.ov_lower);
          break;
      case 3:
          this.paramVerifiedG3 = (p1 == this.freq_upper) && (p2 == this.freq_lower) && (p3 == this.max_current);
          break;
    }
      
    }

  accumulateMessageL2(packet) {
    
    if ((packet[0] === 0x23) && (this.isAccumulatingL2 === false)) {

      this.messageBufferL2 = packet; 
      this.isAccumulatingL2 = true;
      this.isAccumulatingL2Second = true;
    }

    else if (this.isAccumulatingL2Second) {
      if (packet[0] === 0x43) {
        this.messageBufferL2 = Buffer.concat([this.messageBufferL2, packet]);
      }
      else { 
        this.isAccumulatingL2 = false
        this.messageBufferL2 = Buffer.alloc(0);
      }
      this.isAccumulatingL2Second = false;
    }

    else if (this.isAccumulatingL2) {
      if (this.messageBufferL2.length < 20) {
        this.messageBufferL2 = Buffer.concat([this.messageBufferL2, packet]);
      }
      else {
        logger.info(`L2 buffer filled with packes more than 20 : ${this.messageBufferL2}`);
        this.isAccumulatingL2 = false;
        this.messageBufferL2 = Buffer.alloc(0);
      }
    }

    else {
      logger.info(`L2 Defected packet at elase :${packet.toString('hex')}`);
    }

    if (this.isAccumulatingL2 && this.messageBufferL2.length >= 2 && this.messageBufferL2.slice(-2).toString('hex') === '2a0a') {
      this.processCompleteMessageL2(this.messageBufferL2);
      this.messageBufferL2 = Buffer.alloc(0); 
      this.isAccumulatingL2 = false;
      this.isAccumulatingL2Second = false;
      logger.info(`Complete packet :${this.messageBufferL2.toString('hex')} `);
    }
  };

  accumulateMessageFC(packet) {
    if ((packet[0] === 0x23) && (this.isAccumulatingFC === false)) {
      this.messageBufferFC = packet; // Start with the current packet
      this.isAccumulatingFC = true; // Set the flag to true to indicate we're accumulating a message
      this.isAccumulatingFCSecond = true;
    }

    else if (this.isAccumulatingFCSecond) {//if we are accumilating lets check the second
      // If we are already accumulating a message, check the seoncd byte before accumilating
      if (packet[0] === 0x63) {//if the second packet is okay keep filling packets
        this.messageBufferFC = Buffer.concat([this.messageBufferFC, packet]);
      }
      else { //if not a reset it, junk has arrived. refill the packets
        this.isAccumulatingFC = false;
        this.messageBufferFC = Buffer.alloc(0);
      }
      this.isAccumulatingFCSecond = false;
    }

    else if (this.isAccumulatingFC) {
      // If we are already accumulating a message, append the packet

      if (this.messageBufferFC.length < 20) {
        this.messageBufferFC = Buffer.concat([this.messageBufferFC, packet]);
      }
      else {//if length > 20 resetting
        this.isAccumulatingFC = false;
        this.messageBufferFC = Buffer.alloc(0);
      }
    }

    // Check if the buffer ends with 0x2a, 0x0a indicating the end of a message
    if (this.isAccumulatingFC && this.messageBufferFC.length >= 2 && this.messageBufferFC.slice(-2).toString('hex') === '2a0a') {
      // Process the complete message
      this.processCompleteMessageFC(this.messageBufferFC);
      this.messageBufferFC = Buffer.alloc(0); // Reset the buffer for the next message
      this.isAccumulatingFC = false; // Reset the flag as the message is complete
    }
  };

  processCompleteMessageL2(message) {
    if (message.length === 20 && message[0] === 0x23 && message[message.length - 2] === 0x2a && message[message.length - 1] === 0x0a) {
      // Extract the CRC from the message (assuming CRC is at a specific position)
      const crcFromMessage = message.slice(-4, -2); // Assuming the CRC is just before the 0x2a, 0x0a
      const dataForCrcCalculation = message.slice(1, -4); // Exclude start, CRC, and end bytes

      // Calculate CRC
      const calculatedCrc = crc16('MODBUS', dataForCrcCalculation);

      // Compare CRCs
      if (crcFromMessage.equals(Buffer.from(calculatedCrc.toString(16).padStart(4, '0'), 'hex').swap16())) {
        console.log("Complete Message: ", message);
        console.log("CRC check passed");
        logger.info(`CRC PASSED L2 ${message.toString('hex')}`);
        pipe.write(`CRC PASSED L2 ${message.toString('hex')}`);
        // Use the separate method to extract data
        this.extractDataFromMessage(message);
      } else {
        console.log("CRC check failed");
        logger.error(`Complete message but CRC failed L2 ${message.toString('hex')}`);
        pipe.write(`Complete message but CRC failed L2 ${message.toString('hex')}`);
      }
    }
    else {
      logger.error(`Incomplete msg from MCU L2, either diffrent length, invalid start and end packets ${message.toString('hex')}`);
      pipe.write(`Incomplete msg from MCU L2, ${message.toString('hex')}`);
    }
  };

  processCompleteMessageFC(message) {
    if (message.length === 20 && message[0] === 0x23 && message[message.length - 2] === 0x2a && message[message.length - 1] === 0x0a) {
      // Extract the CRC from the message (assuming CRC is at a specific position)
      const crcFromMessage = message.slice(-4, -2); // Assuming the CRC is just before the 0x2a, 0x0a
      const dataForCrcCalculation = message.slice(1, -4); // Exclude start, CRC, and end bytes

      // Calculate CRC
      const calculatedCrc = crc16('MODBUS', dataForCrcCalculation);

      // Compare CRCs
      if (crcFromMessage.equals(Buffer.from(calculatedCrc.toString(16).padStart(4, '0'), 'hex').swap16())) {
        console.log("Complete Message: ", message);
        console.log("CRC check passed");
        logger.info(`CRC PASSED FC ${message.toString('hex')}`);
        //pipe.write(`CRC PASSED  FC ${message.toString('hex')}`);
        // Use the separate method to extract data
        this.extractDataFromMessage(message);
      } else {
        console.log("CRC check failed");
        logger.error(`Complete message but CRC failed FC ${message.toString('hex')}`);
        pipe.write(`Complete message but CRC failed FC ${message.toString('hex')}`);
      }
    }

    else {
      logger.error(`Incomplete msg from MCU L2, either diffrent length, invalid start and end packets ${message.toString('hex')}`);
      pipe.write(`Incomplete msg from MCU L2, ${message.toString('hex')}`);
    }
  };

  mcuMsgEncodeL2(controller, state, stopC, errorC, port) {
    return new Promise((resolve, reject) => {
      this.toggleValueL2 = this.toggleValueL2 === 3 ? 0 : this.toggleValueL2 + 1;
      // this.toggleValueL2 = (this.toggleValueL2 + 1) % 3;
      const controllerMap = { 'M': 0x4D, 'm': 0x6D };
      const stateMap = {
        'IDLE': 0x02, 'PRE_START': 0x04, 'START': 0x05, 'STOP': 0x06
      };
      const errorMap = {
        'GF': 0x01, 'OCF': 0x02, 'GFI': 0x03, 'SC': 0x04, 'UV': 0x05, 'OV': 0x06
      };

      const selectContBufOut = Buffer.from([controllerMap[controller] || 0x4D]);
      const stateCommand = Buffer.from([stateMap[state] || 0x00]);
      const stopCharge = Buffer.from([stopC === 1 ? 0x01 : 0x00]);
      const errorCommand = Buffer.from([errorMap[errorC] || 0x00]);
      const changeDBuf = Buffer.from([this.toggleValueL2]);

      const dataBufOut = Buffer.concat([
        stateCommand,
        stopCharge,
        errorCommand,
        Buffer.alloc(4), // Four bytes of 0x00
        changeDBuf,
        Buffer.alloc(6)  // Six bytes of 0x00
      ]);

      const messageWithoutCrc = Buffer.concat([selectContBufOut, dataBufOut]);
      const crcValue = crc16('MODBUS', messageWithoutCrc);
      const crcBuffer = Buffer.from(crcValue.toString(16).padStart(4, '0'), 'hex').swap16();

      const totalBufOut = Buffer.concat([
        Buffer.from([0x23]),
        messageWithoutCrc,
        crcBuffer,
        Buffer.from([0x2a, 0x0a])
      ]);
      this.totalBufOut = totalBufOut.toString('hex');

      console.log(`\x1b[44mSendinf Packet L2 :" ${totalBufOut.toString('hex')} \x1b[00m`);

      try {
        port.write(totalBufOut, (err) => {
          if (err) {
            console.log('Error on write: ', err.message);
          }
          logger.info(`L2 Data Request msg ${this.totalBufOut}`);
          //pipe.write(`L2 Data Request msg ${this.totalBufOut}`);
        });
        // setTimeout(() => {
        //   display.pageUpdateDMG('L', this.pageL2);
        // }, 200);
        resolve();
      } catch (error) {
        console.error(error.message);
        resolve();
      }
    });
  };

  mcuMsgEncodeFC(controller, state, stopC, errorC, port) {
    return new Promise((resolve, reject) => {
      this.toggleValueFC = this.toggleValueFC === 2 ? 0 : this.toggleValueFC + 1;
      // this.toggleValueFC = (this.toggleValueFC + 1) % 3;
      const controllerMap = { 'M': 0x4D, 'm': 0x6D };
      const stateMap = {
        'IDLE': 0x02, 'PRE_START': 0x04, 'START': 0x05, 'STOP': 0x06
      };
      const errorMap = {
        'GF': 0x01, 'OCF': 0x02, 'GFI': 0x03, 'SC': 0x04, 'UV': 0x05, 'OV': 0x06
      };

      const selectContBufOut = Buffer.from([controllerMap[controller] || 0x4D]);
      const stateCommand = Buffer.from([stateMap[state] || 0x00]);
      const stopCharge = Buffer.from([stopC === 1 ? 0x01 : 0x00]);
      const errorCommand = Buffer.from([errorMap[errorC] || 0x00]);
      const changeDBuf = Buffer.from([this.toggleValueFC]);

      const dataBufOut = Buffer.concat([
        stateCommand,
        stopCharge,
        errorCommand,
        Buffer.alloc(4), // Four bytes of 0x00
        changeDBuf,
        Buffer.alloc(6)  // Six bytes of 0x00
      ]);

      const messageWithoutCrc = Buffer.concat([selectContBufOut, dataBufOut]);
      const crcValue = crc16('MODBUS', messageWithoutCrc);
      const crcBuffer = Buffer.from(crcValue.toString(16).padStart(4, '0'), 'hex').swap16();

      const totalBufOut = Buffer.concat([
        Buffer.from([0x23]),
        messageWithoutCrc,
        crcBuffer,
        Buffer.from([0x2a, 0x0a])
      ]);
      this.totalBufOut = totalBufOut.toString('hex');
      console.log(`\x1b[45mSendinf Packet FC :" ${totalBufOut.toString('hex')} \x1b[00m`);

      try {
        port.write(totalBufOut, (err) => {
          if (err) {
            console.log('Error on write: ', err.message);
          }
          logger.info(`FC Data Request msg ${this.totalBufOut}`);
          //pipe.write(`FC Data Request msg ${this.totalBufOut}`);
        });
        // setTimeout(() => {
        //   display.pageUpdateDMG('R', this.pageFC);
        // }, 200);

        resolve();
      } catch (error) {
        console.error(error.message);
        resolve();
      }
    });
  };

  async checkmcu(controller){
    /*This is where it hits first*/
    console.log(` Loading config for ${controller}`);
    return new Promise(async (resolve, reject) => {
        
        switch(controller){
            case 'M1':
                  /*
  
                  Default Values 
             
                  "uv_en":"01",     0-2           
                  "ov_en":"01",     0-2 
                  "sc_en":"01",     0-2
                  "cpf_en":"01",    0-2
                  "gfi_en":"01",    0-2    
                  "gfit_en":"01",   0-2
                  "pl_en":"01",     0-2
                  "freq_en":"01",   0-2
                  "modbus_en":"01", 0-2
                  "penf_en":"00",   0-2
                  "ot_en":"00",     0-2
                  "uv_upper": "2005",    2000-2150 
                  "uv_lower": "2000",    1800-2000
                  "ov_upper": "2600",    2500-2650
                  "ov_lower": "2550",    2400-2500
                  "freq_upper": "5060",  5000-5100
                  "freq_lower": "4940",  4900-5000
                  "max_current": "30"   8-32
                  */
                  
                  
                const en1_string =  config.M1.mcuConfig.freq_en + config.M1.mcuConfig.pl_en + config.M1.mcuConfig.gfit_en + config.M1.mcuConfig.gfi_en + config.M1.mcuConfig.cpf_en + config.M1.mcuConfig.sc_en + config.M1.mcuConfig.ov_en + config.M1.mcuConfig.uv_en ; 
                console.log("STRING 1 :",en1_string);
                this.en_1 = parseInt(en1_string,2);
                
                const en2_string = "0000000000" + config.M1.mcuConfig.ot_en +  config.M1.mcuConfig.penf_en + config.M1.mcuConfig.modbus_en;
                console.log("STRING 2 :",en2_string);
                this.en_2 = parseInt(en2_string,2) ;
                
                this.uv_upper = parseInt(config.M1.mcuConfig.uv_upper);
                this.uv_lower = parseInt(config.M1.mcuConfig.uv_lower);
                this.ov_upper = parseInt(config.M1.mcuConfig.ov_upper);
                this.ov_lower = parseInt(config.M1.mcuConfig.ov_lower);
                this.freq_upper = parseInt(config.M1.mcuConfig.freq_upper) ;
                this.freq_lower = parseInt(config.M1.mcuConfig.freq_lower);
                this.max_current = parseInt(config.M1.mcuConfig.max_current) ;
                this.update_decision_path = '/charger-flags/config-update-M1';  
                this.controller = 'M';
                break;
            case 'M2':
                this.en_1 = parseInt(config.M2.mcuConfig.en_1);
                this.en_2 = parseInt(config.M2.mcuConfig.en_2);
                this.uv_upper = parseInt(config.M2.mcuConfig.uv_upper);
                this.uv_lower = parseInt(config.M2.mcuConfig.uv_lower);
                this.ov_upper = parseInt(config.M2.mcuConfig.ov_upper);
                this.ov_lower = parseInt(config.M2.mcuConfig.ov_lower);
                this.freq_upper = parseInt(config.M2.mcuConfig.freq_upper) ;
                this.freq_lower = parseInt(config.M2.mcuConfig.freq_lower);
                this.max_current = parseInt(config.M2.mcuConfig.max_current) ;
                this.update_decision_path = '/charger-flags/config-update-M2'; 
                this.controller = 'm'; 
                break;
            
            }
        
        
        
        /*Check wheather mcu should be updated or not*/
        if((await this.execCommand(`cat ${this.update_decision_path}`) === '1'))
            {console.log(`\x1b[33m command : update mcu\x1b[0m`);
            
            /*45 is the updating allowing command that is sent from som in msg id 45*/
            await this.mcuupdate(45);
            
            /*reset updating terms*/
            console.log(`\x1b[33m command : ${controller} set to 0 \x1b[0m`)
            await this.execCommand(`echo 0 > ${this.update_decision_path}`);
            console.log("");
            return resolve();
            }
        else{
            console.log(`\x1b[33m command : do not upate\x1b[0m`);
            
            /*0 is the updating NOT allowing command that is sent from som in msg id 45*/
            await this.mcuupdate(0);
            
            return resolve();}
    })
  }

  mcuupdate(writeVal){
    return new Promise(async (resolve,reject) => {
        
        while(!this.enableConfigFlagReceived){
            console.log(await this.send('writeOrNot',45,[writeVal]));
            await this.delay(1000);
            }
        this.parmCheckComplete = false;
        console.log(await this.send('config',46,[this.en_1,this.en_2,this.uv_upper]));
        await this.delay(1000);
        console.log(await this.send('config',47,[this.uv_lower,this.ov_upper,this.ov_lower]));
        await this.delay(1000);
        console.log(await this.send('config',48,[this.freq_upper,this.freq_lower,this.max_current]));
        await this.delay(1000);
        this.parmCheckComplete = true;
        resolve();
        })
    }
  
    send(msgtype,mid,values){ 
        
      return new Promise((resolve, reject) =>{
          const controllerMap = { 'M': 0x4D, 'm': 0x6D };
          var dataBufOut = Buffer.alloc(15);
          
          switch(msgtype){
              case 'ota':
                  dataBufOut = Buffer.concat([
                    Buffer.from([controllerMap[this.controller] || 0x4D]),
                    Buffer.alloc(7),  // Seven bytes of 0x00
                    Buffer.from(mid.toString(16),'hex'),
                    Buffer.from(values[0].toString(16).padStart(2, '0'),'hex'),
                    Buffer.from(values[1].toString(16).padStart(2, '0'),'hex'),
                    Buffer.from(values[2].toString(16).padStart(2, '0'),'hex'),
                    Buffer.alloc(3, 0xFF)]);
                  break;
                  
              case 'config':
                  
                  dataBufOut = Buffer.concat([
                    Buffer.from([controllerMap[this.controller] || 0x4D]),
                    Buffer.alloc(7),  // Seven bytes of 0x00
                    Buffer.from(mid.toString(16),'hex'),
                    Buffer.from(values[0].toString(16).padStart(4, '0'),'hex').swap16(),
                    Buffer.from(values[1].toString(16).padStart(4, '0'),'hex').swap16(),
                    Buffer.from(values[2].toString(16).padStart(4, '0'),'hex').swap16()]);
                  break;
              
              case 'writeOrNot':
                  dataBufOut = Buffer.concat([
                    Buffer.from([controllerMap[this.controller] || 0x4D]),
                    Buffer.alloc(7),  // Seven bytes of 0x00
                    Buffer.from(mid.toString(16),'hex'),
                    Buffer.from(values[0].toString(16).padStart(4, '0'),'hex').swap16(),
                    Buffer.from((0).toString(16).padStart(4, '0'),'hex').swap16(),
                    Buffer.from((0).toString(16).padStart(4, '0'),'hex').swap16()]);
                  break;
              
              default:
                  console.log("Message type is not identified!!");
                  break;
                  
              
              }
          
          
          const crcValue = crc16('MODBUS', dataBufOut );
          const crcBuffer = Buffer.from(crcValue.toString(16).padStart(4, '0'), 'hex').swap16();

          const totalBufOut = Buffer.concat([
            Buffer.from([0x23]),
            dataBufOut ,
            crcBuffer,
            Buffer.from([0x2a, 0x0a])]);
          
          try {
            console.log("Sendinf Packet :",totalBufOut);
            //return resolve(` mid ${mid} values updated \u2714`);
            
            
            portL2.write(totalBufOut, (err) => {
                if (err) {
                  console.log('Error on write: ', err.message);
                  return resolve(`mid ${mid} values update unsuccessful`);
                }
                return resolve(` mid ${mid} values sent`);
              });
            
            } catch (error) {
              console.error(error.message);
              return resolve(` mid ${mid} values update unsuccessful`);
            }
          
          })
    }
    
    execCommand(command) {
        return new Promise((resolve, reject) => {
              exec(command, (error, stdout, stderr) => {
                if (error || stderr) {
                  console.error(`Error executing command '${command}':`, error || stderr);
                  reject(error || new Error(stderr));
                  return;
                }
                resolve(stdout.trim());
              });
            });
    }

  writeMCUL2Data(state = "IDLE", error = '') {
    this.stateL2 = state;
    this.errorL2 = error;
    console.log(`
			#############################################################
			${this.stateL2}
			#############################################################
		`);

  };

  writeMCUFCData(state = "IDLE", error = '') {
    this.stateFC = state;
    this.errorFC = error;
    console.log(`
			#############################################################
			${this.stateFC}
			#############################################################
		`);

  };

  delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  async communicateWithMCUs() {
    try {
        if(this.parmCheckComplete) {
            await this.mcuMsgEncodeL2('M', this.stateL2, 0, this.errorL2, portL2);
            await this.delay(200);
            await display.pageUpdateDMG('L', this.pageL2);
            }
        else{
            //this.pageL2 = 9;
            await this.delay(200);
            //await display.pageUpdateDMG('L', this.pageL2);
            }
      
        await this.mcuMsgEncodeFC('m', this.stateFC, 0, this.errorFC, portFC);
        await this.delay(200);
        await display.pageUpdateDMG('R', this.pageFC);
    } catch (error) {
      console.error(error);
    }

    // Wait for a fixed interval before starting the next communication cycle
    await this.delay(500); // Adjust the delay as needed

    // Call the function again to continue the communication cycle
    this.communicateWithMCUs.call(this);
  }

  readMCUL2() {
    console.log('opened L2');
    this.writeMCUL2Data();

    // Reset timer on receiving data
    parserFixLenL2.on('data', async (data) => {
      this.mcuMsgDecodeL2(data);
      this.startTimer('timerL2');
    });

    // Start the timer initially
    this.startTimer('timerL2');
  };

  readMCUFC() {
    console.log('opened FC');
    this.writeMCUFCData();

    // Reset timer on receiving data
    parserFixLenFC.on('data', async (data) => {
      this.mcuMsgDecodeFC(data);
      this.startTimer('timerFC');
    });

    // Start the timer initially
    this.startTimer('timerFC');
  };

  pageChange(side, page = 0) {
    if (side == 'L') {
      this.pageL2 = page;
    } else if (side == 'R') {
      this.pageFC = page;
    }
  };

  setup() {
    // Read from MCU L2
    portL2.on('open', this.readMCUL2.bind(this));
    // Read from MCU FC
    portFC.on('open', this.readMCUFC.bind(this));
    // Call the communication function after ports are setup
    this.communicateWithMCUs();
  };
};

module.exports = { MCUManager, liveDMGLeft, liveDMGRight, netDMGLeft, netDMGRight };
