/*
file: ota.js
 * Update the mcu firmware over the air from SOM
 * Date : 02/05/2024

testing 
ACK packet from mcu      : Ox79

*/
const {exec} = require("child_process"); 
const {SerialPort} = require('/usr/lib/node_modules/serialport');
const {ByteLengthParser} = require('/usr/lib/node_modules/@serialport/parser-byte-length');
const fs = require('fs');
const port = new SerialPort({ path: '/dev/ttyS1', baudRate: 115200});
const parser = port.pipe(new ByteLengthParser({ length: 1 }));
const { SingleBar } = require('cli-progress');
const progressBar = new SingleBar({
    format: 'Progress [{bar}] {percentage}% | {value}/{total}',
    barCompleteChar: '\u2588',
    barIncompleteChar: '\u2591',
    hideCursor: true
});

/*HW Pins*/
const BOOTPIN = 13;
const RESETPIN  = 12;
let bootPin;
let resetPin;
var count = 0;
var TRYAGAIN = 0;
const ota_flag_path = '/chargerFiles/node/features/ChargeNET-Connect/flags/ota';



/*Size constants*/
/*

const NO_OF_SLOTS = 2;
const SLOT_MAX_SIZE = 20*1024;
const OTA_DATA_MAX_SIZE = 16;
const OTA_DATA_OVERHEAD = 7;
const OTA_PACKET_MAX_SIZE = OTA_DATA_MAX_SIZE + OTA_DATA_OVERHEAD;
const DATA_PCKTLEN = 16;
const ACK_BUF_SIZE = 8;
 */

const ADDRESS = 0x08000000;
const PCKTSIZE = 200;

const MAX_BLOCK_SIZE = 1024;
const MAX_FW_SIZE = MAX_BLOCK_SIZE * 100;
const MAX_REPETITION = 20;

const CMD_DELAY = 100; // in ms
const DATA_DELAY =15; // in ms 
//var BYPASS = 0; //bypass ack

/*Allocating main buffers*/
var APP_BIN ;

var binName = process.argv[2];
var scriptLocation = process.argv[1].split("/");
var binLocation = "";
var ACK = 0;
//var PWRFAIL = 0; 
//var count = 0; 
//var takein = 0;
var app_size = 0;

//var lenbuf = Buffer.alloc(2);
// cmdbuf = Buffer.alloc(2);


const colour = {
	red    : 0,
	green  : 1,
	yellow : 2,
	blue   : 3,
	purple : 4, 
}


/*file location*/
for(let i=1; i<scriptLocation.length-1; i++) { 
	binLocation = binLocation + "/" + scriptLocation[i];
}
binLocation = binLocation + "/" + binName;
console.log(`Start Firmware update: ${binLocation}`);

/*
serial port data sendinf receiving and decoding 
*/
port.on('open', function() {
	console.log(`Serial connection open ..`);
	parser.on('data', function(data) {
        
		
		
		/* 0x79 : Ack*/
		if(data[0] === 0x79){
            //console.log(` << ${count} `,data) ;
			ACK = 1;
		}else if(data[0] === 0x1F){
            //console.log(` << ${count} `,data) ;
			ACK = 0;
        }

	});
});

function execCommand(command) {
    return new Promise((resolve, reject) => {
        exec(command, (error, stdout, stderr) => {  
            if (error) {
                console.error(`Error executing command '${command}':`, error);
                reject(error);
                return;
            }
            if (stderr) {
                console.error(`Command error output: ${stderr}`);
                reject(new Error(stderr));
                return;
            }
            resolve(stdout.trim());
        });
    });
}


/*GPIO on SOM*/
class GPIO {
    constructor(pin, dir, val) {
      this.pin = pin;
      this.dir = dir;
      this.val = val;
      this.timeoutId = null;
      this.create();
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
  
    async create() {
      try {
        // await this.execCommand(`echo ${this.pin} > /sys/class/gpio/unexport`);
        await this.execCommand(`echo ${this.pin} > /sys/class/gpio/export`);
        await new Promise(resolve => setTimeout(resolve, 100)); // Wait 100ms
        await this.execCommand(`echo ${this.dir} > /sys/class/gpio/gpio${this.pin}/direction`);
        await new Promise(resolve => setTimeout(resolve, 100)); // Wait 100ms
        await this.execCommand(`echo ${this.val} > /sys/class/gpio/gpio${this.pin}/value`);
      } catch (error) {
        console.error('Error in GPIO setup:', error);
      }
    }
  
    async checkExported() {
      try {
        await this.execCommand(`cat /sys/class/gpio/gpio${this.pin}/direction`);
        return true; // No error means it's already exported
      } catch {
        return false; // Error means it's not exported
      }
    }
  
    async on() {
      return await this.execCommand(`echo 1 > /sys/class/gpio/gpio${this.pin}/value`);
    }
  
    async off() {
      return await this.execCommand(`echo 0 > /sys/class/gpio/gpio${this.pin}/value`);
    }
  
    async isOn() {
      const value = await this.execCommand(`cat < /sys/class/gpio/gpio${this.pin}/value`);
      return parseInt(value, 10) === 1;
    }
  
    async unexport() {
      return await this.execCommand(`echo ${this.pin} > /sys/class/gpio/unexport`);
    }
};


/*
delay fucntion
*/
function delay(time) {
  return new Promise(resolve => setTimeout(resolve, time));
}


async function getFileSize(filePath) {
  try {
    const stats = await fs.promises.stat(filePath);
    const fileSizeInBytes = stats.size;
    return fileSizeInBytes;
	
  } catch (error) {
    console.error('Error getting file size:', error);
    throw error;
  }
}



/*
Sending any message to the serial bus
In : what (string)- The name of the step 
	 waiting time (int) - waiting time 
	 buf (Buffer) -n bit buffer 
return : Promise
*/
async function send(what, waitingTime, buf) {
    let result = 0;
    let repetition = 0;

    while (result === 0 && repetition < MAX_REPETITION) {
        
        what != '' ? console.log(`${what} Sending to serial bus .. attempt ${repetition}`) : null;
        // Send data over serial port
        
        for (let i = 0; i< buf.length ; i++){
             await new Promise((resolve, reject) => {
                    port.write(buf.slice(i,i+1), err => {
                        if (err) {
                            console.log('err uart_send');
                            reject(err);
                        } else {
                            //console.log(' >> ', buf.slice(i,i+1));
                            resolve();
                        }
                    });
                });
            
            }
        

        //console.log(` Waiting for acknowledgment...`);
        
        // Wait for the specified time
        await new Promise(resolve => setTimeout(resolve, waitingTime));
		
		//ACK = 1;
		
        if (ACK) {
            result = 1;
			ACK = 0;
        } 
		else {
            result = 0;
            repetition++;
        }
		
    }

    return result;
	
}

/*
Send data packets in loop during sending of data. 
In : None
return: Promise 
*/
async function sendloop(){
    TRYAGAIN = 0;
    var writeAddress = ADDRESS;
    var size = 0;
    var temp = 0;
    var status = 0;
    var sentPcktCount = 0 ;
    var packetsToBeSent = Math.floor(app_size/PCKTSIZE)+1 ; 

    
    console.log(`App size : ${app_size} bytes\nPacket size: ${PCKTSIZE}\nNo Packets: ${Math.floor(app_size/PCKTSIZE)+1}`);
    progressBar.start(app_size, 0);
    
    //i is the number of byte transmitting
    var i = 0;
    for(var sentPcktCount= temp ; sentPcktCount < packetsToBeSent;){
        
        
        //write enable 
        status = await send(
                    '',
                    DATA_DELAY,
                    Buffer.from([0x31,0xCE]));
                    
        if(status == 0){
            console.log(colourize("ADDRESS NOT OK",colour.red));
            TRYAGAIN = 1;
            return Promise.reject("ADDRESS NOT OK");
            }
        status = 0;
    
        writeAddress  = ADDRESS + PCKTSIZE*sentPcktCount;
        var b1 = 0xff & (writeAddress >> 24);
		var b2 = 0xff & (writeAddress >> 16);
		var b3 = 0xff & (writeAddress >> 8); 
		var b4 = 0xff & writeAddress;
		var b5 = b1^b2^b3^b4;
        status = await send(
                    '',
                    CMD_DELAY,
                    Buffer.from([b1,b2,b3,b4,b5]));
                    
        status ? null :console.log(colourize("ADDRESS NOT OK",colour.red));
        status = 0;
        size = (app_size - i) >= PCKTSIZE ? PCKTSIZE : (app_size - i);
        
        //writing 1st byte : packet_size-1 
        const nbyte = PCKTSIZE -1;
        //writing from 2nd + size bytes
        var chunk = Buffer.alloc(PCKTSIZE);
        chunk = size == PCKTSIZE ?  APP_BIN.slice(i,i+size) : Buffer.concat([APP_BIN.slice(i,i+size),Buffer.alloc(PCKTSIZE-size, 0xff)]);
        //writing ckecksm byte
        var checksm = 0x00;
        const bufWithoutCS = Buffer.concat([Buffer.from([nbyte],'hex'),chunk]);
        for (let p =0 ; p < bufWithoutCS.length ; p++){
            checksm = checksm^bufWithoutCS[p];
            }
            
        const finalBuf = Buffer.concat([bufWithoutCS,Buffer.from([checksm])]);        
        status = await send(
                    '',
                    DATA_DELAY,
                    finalBuf
                    );
                    
        if(status == 0){
            console.log(colourize("PACKET SENDING ISSUE",colour.red));
            TRYAGAIN = 1;
            return Promise.reject("PACKET SENDING ISSUE");
            }
        status = 0;
        
        
        i += size;
        sentPcktCount += 1;
        progressBar.update(sentPcktCount*PCKTSIZE);
        
        
    }
    
    progressBar.stop();
	return Promise.resolve(colourize(" DATA SUCCESSFULLY TRANSFERED", colour.blue))
}

/*
If you are not colour blind
*/
function colourize(string,colour){
	switch(colour){
		case 0 : //red
			return `\x1b[31m` + string + `\x1b[00m`;break;
		case 1 : //green
			return `\x1b[32m` + string + `\x1b[00m`;break;
		case 2 : //yellow
			return `\x1b[33m` + string + `\x1b[00m`;break;
		case 3 : //blue
			return `\x1b[34m` + string + `\x1b[00m`;break;
		case 4 : //purple
			return `\x1b[35m` + string + `\x1b[00m`;break;
		default :
			return "colour not set : "+string;break;
	}
}

/*
Time delay fucntion
*/
function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
  

/*
Main fucntion
*/
async function main(){
    
	
	getFileSize(binLocation)
		.then(size => {
			/*Checking bin file*/ 
			console.log(` File size: ${size} bytes`);
			app_size = size;
			
			APP_BIN = Buffer.alloc(size);
			
			if (size > MAX_FW_SIZE){
				throw new Error(`File size is larger`);}
			else{
				
				return new Promise((resolve,reject) =>{
					fs.readFile(binLocation, (err, data) => {
						if (err) {
							console.error('Error reading file:', err);
							reject(err)
						}

						// The file contents are now stored in the 'data' buffer
						//console.log('File contents:', data);
						APP_BIN = Buffer.from(data);
						resolve(colourize(` BIN FILE (${size} bytes) READING SUCCESS`,colour.blue));
						execCommand(`echo 1 > ${ota_flag_path}`);
					
					});
					
				})
				
			}
		 })
		.then( async msg =>{
			/* Setting Boot Mode*/
            
            console.log(colourize("1 HIGH BOOT Pin, and Toggle RESET pin",colour.green));
            
            bootPin = new GPIO(BOOTPIN,'out',0);
            resetPin = new GPIO(RESETPIN,'out',0);
            await delay(500);
			
			bootPin.on();await delay(500);
			
			resetPin.off();await delay(500);
			resetPin.on();await delay(500);
			resetPin.off();await delay(500);
			
			
			return send(
				colourize("2 ENTER BOODMODE",colour.green),
				CMD_DELAY,
				Buffer.from([0x7F]))
			
			
		 })
		.then( msg =>{ 
			/*Erase Memory access request */

			
			return send(
				colourize("3 FLASH ERASE REQ",colour.green),
				CMD_DELAY,
				Buffer.from([0x43,0xBC]))
			
			 
		})
		.then (msg =>{
			/*Flash Erasing*/
			return send(
				colourize("4 GLOBAL ERASE CMD",colour.green),
				CMD_DELAY,
				Buffer.from([0xFF,0x00]))
			
		})
		.then((msg)=>{
			/*Sending OTA data */
			console.log(` ${msg}`);
			
			return sendloop();
			  
		})
        .then(async (msg)=>{
			/*RESET BOOTMODE*/
            console.log(colourize("5 LOW BOOT Pin, and Toggle RESET pin",colour.green));
			
			bootPin.off();await delay(500);
			
			resetPin.off();await delay(500);
			resetPin.on();await delay(500);
			resetPin.off();await delay(500);
			  
		})
		.then((msg) =>{
			/*All Steps Complete */
			execCommand(`echo 8 > ${ota_flag_path}`);
			console.log(colourize(` ALL STEPS COMPLETED .. Enjoy!!`,colour.purple));
			process.exit()
		})
		.catch(async error => {
			
			if(TRYAGAIN){
				TRYAGAIN = 0;
				await delay(3000);
				return main();
                }
			else{
				execCommand(`echo 4 > ${ota_flag_path}`);
				console.log(` Final Error : ${error}`);
			}
			process.exit()
		 })
		.finally(()=>{
			bootpin.unexport();
			resetpin.unexport();
		});
		
	
	
}

main();
