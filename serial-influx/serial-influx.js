console.log('starting serial->influx');
const SerialPort = require('serialport');
const Delimiter = require('@serialport/parser-delimiter');
const Influx = require('influxdb-nodejs');
const client = new Influx('http://127.0.0.1:8086/beer');

const port = new SerialPort('/dev/ttyUSB0', {
    baudRate: 38400
});

const parser = port.pipe(new Delimiter({ delimiter: '\n' }));

const fieldSchema = {
    count: 'i',
    temperature: 'f',
};
const tagSchema = {
};
client.schema('beer', fieldSchema, tagSchema, {
    stripUnknown: true,
});

console.log('setup done... waiting for message');

parser.on('data', (data) => {
    console.log('got some data');
    const parsed = /t:(\d+) count:(\d+) temp: (\d+.\d+)/.exec(data);
    if (parsed) {
        const t = parseInt(parsed[1]);
        const beerCount = parseInt(parsed[2], 10);
        const temperature = parseFloat(parsed[3]);
        console.log(t, beerCount, temperature);
        client.write('beer')
            .tag({})
            .field({
                count: beerCount,
                temperature: temperature,
            })
            .then(() => console.info('write point success'))
            .catch(console.error);
    }
});