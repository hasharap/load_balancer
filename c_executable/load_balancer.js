const { SerialPort } = require('/usr/lib/node_modules/serialport');
const { ReadlineParser } = require('/usr/lib/node_modules/@serialport/parser-readline');
const fs = require('fs');

const config = require('/chargerFiles/L2/config.json');
const max_current = parseFloat(config.M1.mcuConfig.max_current);

const I_undetected = 15;
const I_low = 7;
const I_moderate = 15;




// initialize a file to write controll current
fs.writeFile('/tmp/load_balancer/control_current', 'a\n', (err) => {
  if (err) console.error('Init write error (dlbd_write):', err.message);
});

// initialize a file to read charging state (write from mcu.js)
fs.writeFile('/tmp/load_balancer/charger_state', 'a\n', (err) => {
  if (err) console.error('Init write error (dlbd_read):', err.message);
});


const port = new SerialPort({
  path: '/dev/ttyS2',
  baudRate: 115200,
});

const parser = port.pipe(new ReadlineParser({delimiter: '\n'}));

// Periodic check of file "dlbd_read" and sending command over serial
port.on('open', () => {
  setInterval(() => {
    fs.readFile('/tmp/dlbd_read', 'utf8', (err, chState) => {
      if (err) {
        console.error('Read error:', err.message);
        return;
      }

      chState = chState.trim();
      let cmdToRak = '';

      if (chState === 'STATE_C2') {
        cmdToRak = 'c';
      } else if (chState === 'STATE_A1') {
        cmdToRak = 'i';
      } else {
        console.log('Invalid state in charger state read:', chState);
        return;
      }

      port.write(cmdToRak, (err) => {
        if (err) {
          console.log('Error on write:', err.message);
        } else {
          console.log(`Sent to RAK: ${cmdToRak}`);
        }
      });
    });
  }, 1000); // Every 1 second
});

// Process incoming serial data
let ready = true;
const delay = 200; // milliseconds between reads

parser.on('data', (line) => {
  if (!ready) return;
  ready = false;

  const data = line.trim();

  let content;
  
  switch (data) {
    case 'U':
      content = I_undetected.toString();
	 // console.log(content);
      break;
    case 'L':
      content = I_low.toString();
      break;
    case 'M':
      content = I_moderate.toString();
      break;
    default:
      const read = parseFloat(data);
      if (!isNaN(read)) {
        const control_current = max_current - read;
        content = control_current.toString();
		// console.log(content);
      } else {
        console.log(`Unknown Data: ${data}`);
        ready = true;
      }
      break;
  }

  fs.writeFile('/tmp/load_balancer/control_current', content, (err) => {
    if (err) console.error('Write error:', err.message);
  });

  setTimeout(() => {
    ready = true;
  }, delay);
});