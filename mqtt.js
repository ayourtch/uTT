// This is a client bottlenecked "benchmark" example not capable of stressing µTT

// Mosca: 863ms
// Mosquitto: 174ms
// µTT: 138ms

// NOTE: Using the *native port* of this "benchmark", which is not *client bottlenecked* gives:

// Mosca: 936ms
// Mosquitto: 140ms
// µTT: 4ms

// As you can see, benchmarking with a proper client is *critically important* as MQTT.js cannot even stress µTT above 0% CPU

var mqtt = require('mqtt')
var clients = [];
var remainingPubs = 500 * 100;

function newConnection() {
	var client  = mqtt.connect('mqtt://localhost')
	client.on('message', function (topic, message) {
		if (--remainingPubs === 0) {
			console.timeEnd('publishing time');
		}
	})
	client.on('connect', function () {
		client.subscribe('some topic', function() {
			clients.push(client);
			if (clients.length == 500) {
				console.time('publishing time');
				for (var i = 0; i < 100; i++) {
					clients[i].publish('some topic');
				}
			} else {
				newConnection();
			}
		})
	})
}

newConnection();
