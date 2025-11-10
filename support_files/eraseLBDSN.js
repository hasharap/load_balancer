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
		port.write('e' , (err) => {
			if(err) {
				return console.log(COLOR.red+'Error on write: ' ,err.message+COLOR.reset);
			}
			console.log('Sent: e');
		})
	},1000);
});
parser.on('data', (line) => {
	const data = line.trim(); //clean input
	
	if(data === 'E')
	{
		console.log(COLOR.green+'Erased old SN from charger side.. ');
		console.log('Write a new one to continue.. '+COLOR.reset);
		process.exit();
	}
	
});