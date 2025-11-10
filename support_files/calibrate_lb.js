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

port.on('open', () => {
  console.log('Serial port opened');
  setInterval(() => {
    port.write('u', (err) => {
      if (err) {
        return console.log(COLOR.red + 'Error on write: ' + err.message + COLOR.reset);
      }
      console.log(COLOR.yellow + 'Sent: calibration request' + COLOR.reset);
	  console.log(COLOR.yellow + 'Waiting for response Packet..' + COLOR.reset);
    });
  }, 5000);
});

parser.on('data', (line) => {
  const data = line.trim();

  if (data.startsWith('CBR:')) {
    const rnrValues = data.slice(4).split(',').map(Number);

    if (rnrValues.length === 11) {
      const [r0, r1, r2, type, p1, p2, p3, d1, d2, d3, r10] = rnrValues;

      if (r0 === 5 && r1 === 5 && r2 === 5) {
        console.log(COLOR.green + '✔ Valid response received' + COLOR.reset);

        // Load Balancer Type
        switch (type) {
          case 1: console.log(COLOR.red+'→ Single Phase Load Balancer: Recognized Phase 1'+COLOR.reset); break;
          case 2: console.log(COLOR.red+'→ Single Phase Load Balancer: Recognized Phase 2'+COLOR.reset); break;
          case 3: console.log(COLOR.red+'→ Single Phase Load Balancer: Recognized Phase 3'+COLOR.reset); break;
          case 4: console.log(COLOR.red+'→ Three Phase Load Balancer'); break;
          default: console.log('→ Load Balancer Type Not Recognized'); break;
        }

        // Phase Match
        const phaseType = (val, label) => {
          switch (val) {
            case 1: return `${COLOR.cyan}${label} - Match ⚡️${COLOR.reset}`;
            case 2: return `${COLOR.cyan}${label} - Lag ⚠️${COLOR.reset}`;
            case 3: return `${COLOR.cyan}${label} - Lead ⚠️${COLOR.reset}`;
            default: return `${label} - None`;
          }
        };

        console.log(phaseType(p1, 'Phase 1'));
        console.log(phaseType(p2, 'Phase 2'));
        console.log(phaseType(p3, 'Phase 3'));

        // Power Direction
        const powerDir = (val, label) => {
          switch (val) {
            case 1: return `${COLOR.purple}${label} - Default Power Direction Positive +${COLOR.reset}`;
            case 2: return `${COLOR.purple}${label} - Default Power Direction Negative -${COLOR.reset}`;
            default: return `${label} - None`;
          }
        };

        console.log(powerDir(d1, 'Phase 1'));
        console.log(powerDir(d2, 'Phase 2'));
        console.log(powerDir(d3, 'Phase 3'));
        
        process.exit();

      } else {
        console.log(COLOR.red + '✘ Invalid CBR prefix: ' + r0 + ', ' + r1 + ', ' + r2 + COLOR.reset);
      }

    } else {
      console.log(COLOR.red + '✘ Invalid CBR length: ' + rnrValues.length + COLOR.reset);
    }
  }
});
