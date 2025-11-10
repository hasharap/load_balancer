/* Talk with STM32 mcu over serial defalut bootloader. Host code
 05.06.2024
 Ref:
 https://www.st.com/resource/en/programming_manual/cd00283419-stm32f10xxx-flash-memory-microcontrollers-stmicroelectronics.pdf
 This is based of AN3155 Application note and can be used for all STM32F*
 
 * */
const {SerialPort} = require('/usr/lib/node_modules/serialport');
const fs = require('fs');
const {ByteLengthParser} = require('/usr/lib/node_modules/@serialport/parser-byte-length');
const { exec } = require("child_process");
const readline = require('readline');
const port = new SerialPort({ path: '/dev/ttyS2', baudRate: 115200});
const BOOTPIN = 6;
const RESETPIN =47;
const CMD_DELAY = 2000 // in ms
const MAX_REPETITION = 1;
let bootpin;
let resetpin;
let receiveBuf;

let ACK = 0;

/*Receive*/
port.on("open", function() {
	console.log("Serial connection open..");
	port.on("data", function(data) {
        
		//console.log("<< ",data);
		receiveBuf = data;
        
        if (data.slice(0,1).toString('hex') === '79'){
			ACK = 1;
			//console.log(colourize("ACK RECEIVED!", colour.blue));
		}
		else if(data.slice(0,1).toString('hex') === '1F'){
			console.log(colourize("NAK RECEIVED",colour.yellow));
		}
	});
});

async function quicksend(buf){
	// Send data over serial port
	await new Promise((resolve, reject) => {
		port.write(buf, err => {
			if (err) {
				console.log('err uart_send');
				reject(err);
			} else {
				//console.log(">> ",buf)
				resolve();
			}
		});
	});
}

const colour = {
	red    : 0,
	green  : 1,
	yellow : 2,
	blue   : 3,
	purple : 4,
	bg :{
		yellow : 5,
		purple : 6,
	}
}
/*If you are not colour blind
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
		case 5 : //yellog bg
			return `\x1b[43m\x1b[30m` + string + `\x1b[00m`;break;
		case 6 : //purple bg
			return `\x1b[45m\x1b[30m` + string + `\x1b[00m`;break;
		default :
			return "colour not set : "+string;break;
	}
}


//read serial number
let serialNumber = '';
try {
	const path = '/chargerFiles/L2/load_balancer/lb_sn.json';
	const jsonData = fs.readFileSync(path, 'utf-8');
	const jsonObj = JSON.parse(jsonData);
	serialNumber = jsonObj.SerialNumber;

	if (!serialNumber || serialNumber.length !== 10) {
		console.log(colourize(`Invalid or missing SerialNumber`, colour.red));
		process.exit(1);
	}
	console.log(colourize(`SerialNumber to write: ${serialNumber}`, colour.blue));
} catch (err) {
	console.log(colourize(`Failed to read or parse LBDSN.json`, colour.red));
	console.error(err.message);
	process.exit(1);
}



/*GPIO Handle*/
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
bootpin = new GPIO(BOOTPIN,'out',0);
resetpin = new GPIO(RESETPIN,'out',0);

/*Delay*/
function delay(time) {
  return new Promise(resolve => setTimeout(resolve, time)); //time in milliseocnds 
}


// Function to convert text to hex
function textToHex(text) {
  // Convert the text to a Buffer object
  const buffer = Buffer.from(text, 'utf-8');
  // Convert the buffer to a hexadecimal string
  const hex = buffer.toString('hex');
  return hex;
}

    
/*Talk business with MCU*/
/*Get Version*/
async function command(cmd) {
	switch (cmd) {
		case '5': // Write Serial Number to memory
		     /*
		      //Erase it first
			  
			console.log(colourize("+ ERASE PAGE 126 BEFORE WRITING", colour.yellow));
		    // === ERASE MEMORY (Page 126) ===
			quicksend(Buffer.from([0x43]));
			await delay(1000);
			quicksend(Buffer.from([0xBC]));
			await delay(1000);

			console.log(`   Waiting for acknowledgment...`);
			await new Promise(resolve => setTimeout(resolve, 1000));
		
		    if (ACK) {
                ACK = 0;
                console.log(colourize(`   Erase Granted!`, colour.green));

				const pageNumber = 126;
				const Nbyte = 0x00;  //N+1 pages will be erased
				//const pageNobyte = pageNumber;
				//const cksmbyte = Nbyte^pageNobyte;
				const checksum = Nbyte ^ pageNumber;
				
				quicksend(Buffer.from([0x44, 0xBB])); 
				await delay(1000);
				quicksend(Buffer.from([Nbyte, pageNumber, checksum]));
				await delay(1000);

				console.log(`   Waiting for erase acknowledgment...`);
				await new Promise(resolve => setTimeout(resolve, 1000));

				if (ACK) {
					ACK = 0;
					console.log(colourize(`   Page 126 erased successfully!`, colour.blue));
				} else {
					console.log(colourize(`   Failed to erase page 126!`, colour.red));
					break;
				}
				} else {
					console.log(colourize(`   Erase command not acknowledged!`, colour.red));
					break;
				}
		    
		 */
			console.log(colourize("+ WRITE MEMORY", colour.blue));

			// Step 1: Send Write Memory Command
			quicksend(Buffer.from([0x31]));
			await delay(1000);
			quicksend(Buffer.from([0xCE]));
			await delay(1000);

			console.log("   Waiting for acknowledgment...");
			await new Promise(resolve => setTimeout(resolve, 1000));
			
			if(ACK){
				ACK=0;
				console.log(colourize(`   Granted!!`,colour.green));
				const address = parseInt('0x0803F000', 16)
			    const bytes = [];
				const byte1 = (address >> 24) & 0xFF;
				const byte2 = (address >> 16) & 0xFF;
				const byte3 = (address >> 8) & 0xFF;
				const byte4 = address & 0xFF;
				const byte5 = byte1^byte2^byte3^byte4;//CHECKSM
				bytes.push(byte1,byte2,byte3,byte4,byte5);
			
				for (let i = 0; i < bytes.length; i++)
					{
						quicksend(Buffer.from([bytes[i]]));
						await delay(1000);
					}
				
			}

			else {
				console.log(colourize("   ACK not received. Can't enable write memory!", colour.red));
				break;
			}
			

			// Step 2: Send target memory address 0x0803F000
			//const address = 0x0803F000;
			
			console.log("   Waiting for acknowledgment...");
			await new Promise(resolve => setTimeout(resolve, 1000));

			if (!ACK) {
				console.log(colourize("   ACK not received. Address not available!", colour.red));
				break;
			}
			ACK = 0;
			console.log(colourize("   Address Granted!", colour.blue));

			// Step 3: Prepare data bytes
			const serialBytes = Buffer.from(serialNumber, 'utf8');
			const N = serialBytes.length - 1; // N = number of bytes - 1
			let checksum2 = N;
			for (let i = 0; i < serialBytes.length; i++) {
				checksum2 ^= serialBytes[i];
			}

			const payload = Buffer.concat([
				Buffer.from([N]),
				serialBytes,
				Buffer.from([checksum2])
			]);

			// Step 4: Send the serial data
			for (let i = 0; i < payload.length; i++) {
				quicksend(Buffer.from([payload[i]]));
				await delay(100);
			}

			console.log("   Waiting for acknowledgment...");
			await delay(1000);

			if (ACK) {
				console.log(colourize("   Write Successful", colour.green));
			} else {
				console.log(colourize("   Write Unsuccessful", colour.red));
			}
			ACK = 0;

			break;
			
			
	}
}

async function enteringBootMode(){
	
	console.log(colourize("GPIO TOGGLE",colour.yellow));
	await delay(1000); // This wait is crucial untill the gpio pin is created
	bootpin.on();await delay(1000);
	
	resetpin.off();await delay(500);
	resetpin.on();await delay(500);
	resetpin.off();await delay(500);
	console.log(colourize("ENTERING BOOTMODE",colour.blue));
	
	quicksend(Buffer.from([0x7F]));
	await new Promise(resolve => setTimeout(resolve, 2000));
	
	return ACK ? Promise.resolve(colourize(" SUCCESS!",colour.green)) : Promise.reject(colourize(" UNSUCCESSFUL",colour.red));
}

async function exitingBootMode(){
	console.log(colourize("EXIT FROM BOOTMODE",colour.blue));
	bootpin.off();await delay(500);
	
	resetpin.off();await delay(500);
	resetpin.on();await delay(500);
	resetpin.off();await delay(1000);
	
	return Promise.resolve(colourize("EXIT FROM BOOTMODE",colour.green))
}



async function main(){	
	enteringBootMode()
	.then(async function sendcmd(msg){
		console.log(`${msg}`);
		const commandNumber = '5';
		return command(commandNumber);
	})
	.catch(err =>{
		console.log("Error found:",err);
	})
	.finally(async ()=>{
		const reply = 'y';
		
		if(reply == 'n'){
			main();
		}else{
			await exitingBootMode();
			bootpin.unexport();
			resetpin.unexport();
			console.log("All cleared .. ");
			process.exit();
		}
		
		
		
	})
    
    }
    

main();    
