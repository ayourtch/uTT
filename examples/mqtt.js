var mqtt = require('mqtt');

// connect to the broker
var client = mqtt.connect('mqtt://localhost');

client.on('connect', (err, granted) => {
  // subscribe to all temperature sensors
  client.subscribe('sensors/+/temperature', () => {
    // publish some temperature numbers
    client.publish('sensors/house/temperature', '21');
    client.publish('sensors/sauna/temperature', '107');
  });
});

client.on('message', (topic, message) => {
  // receive our numbers
  console.log(topic + ': ' + message.toString() + ' Celcius');
});
