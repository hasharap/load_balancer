const { SerialPort } = require('/usr/lib/node_modules/serialport');
const { ReadlineParser } = require('/usr/lib/node_modules/@serialport/parser-readline');
const config = require('/chargerFiles/config.json');
const max_current = parseFloat(config.M1.mcuConfig.max_current);

const port = new SerialPort({
  path: '/dev/ttyS2',
  baudRate: 115200,
});

const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

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
		port.write('i' , (err) => {
			if(err) {
				return console.log('Error on write: ' ,err.message);
			}
			console.log('Sent: x');
		})
	},1000);
});




parser.on('data', (line) => {
	const data = line.trim(); //clean input
	
	if(data === 'U')
	{
		console.log('Undetected');
               // process.exit();
    	}
	else if(data === 'L')
	{
		console.log('Low Current');
	}
	else if(data === 'M')
	{
		console.log('Middle Current');
	}
	else
	{
		const read = parseFloat(line.trim());
		if(!isNaN(read))
		{
			const current = max_current-read;
			console.log(COLOR.cyan+`Current value : ${current}`+COLOR.reset);
		}
		else
		{
			console.log('Unknown Data: ${data}');  
		}
	}
});