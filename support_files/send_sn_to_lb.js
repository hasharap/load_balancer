const { SerialPort } = require('/usr/lib/node_modules/serialport');
const { ReadlineParser } = require('/usr/lib/node_modules/@serialport/parser-readline');

const port = new SerialPort({
  path: '/dev/ttyS2',
  baudRate: 115200,
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

// ANSI color codes
const COLOR = {
  reset: '\x1b[0m',
  green: '\x1b[32m',
  red: '\x1b[31m',
  purple: '\x1b[35m',
  yellow: '\x1b[33m',
  cyan: '\x1b[36m',
  bold: '\x1b[1m'
};


port.on('open' , () => {
	console.log('serial port opend');
	setInterval(() => {
		port.write('s' , (err) => {
			if(err) {
				return console.log(COLOR.red+'Error on write: ' ,err.message+COLOR.reset);
			}
			console.log(COLOR.yellow+'Sent SN write command to DLBD');
			console.log('Waiting for response Packet.... '+COLOR.reset);
		})
	},1000);
});
parser.on('data', (line) => {
	const data = line.trim(); //clean input
	
	if(data === 'S')
	{
		console.log(COLOR.green+'SN wrote Successfully to the LB !!!... '+COLOR.reset);
		process.exit();
	}
	
});