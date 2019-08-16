const SerialPort = require('serialport');
const Delimiter = require('@serialport/parser-delimiter');

const port = new SerialPort('/dev/ttyUSB0', {
    baudRate: 38400
});

const parser = port.pipe(new Delimiter({ delimiter: '\n' }));

parser.on('data', (data) => {
    console.log(data.toString());
    const parsed = /t:(\d+) count:(\d+) temp: (\d+.\d+)/.exec(data);
    if (parsed) {
        const t = parseInt(parsed[1]);
        const beerCount = parseInt(parsed[2], 10);
        const temperature = parseFloat(parsed[3]);
        console.log(t, beerCount, temperature);
    }
});