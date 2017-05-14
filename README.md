## µTT (motivation post so far, prototype exists)
µTT ("microTT") is a free and open source MQTT broker designed to [raise the bar](https://github.com/alexhultman/MQTT-broker-benchmarks) when it comes to simple pub/sub needs. In contrast to many other brokers it focuses on one thing and one thing only - the open and lightweight MQTT standard. This allows for an instant adoption where the current MQTT client support can be leveraged to connect existing IoT devices with no change of code.

#### The "we need a new standard" meme
The popular [xkcd meme](https://xkcd.com/927/) cannot be any more relevant here. We already have a billion different brokers out there, each with its own custom protocol all aiming to achieve the same general goal. Open standards invite a healthy dose of evolution while custom protocols only aim to lock users to a specific vendor and complicate benchmarking. Why add a new protocol only to ruin it with bloated and inefficient JSON/text fundamentals with O(n) parsing when we have lightweight and binary O(1) protocols already standardized?

#### Vendor-neutral, minimal & efficient pub/sub
MQTT has clients in all major languages and they provide easy to use pub/sub just like any other solution would. Below is a simple Node.js example using MQTT.js:
```javascript
var mqtt = require('mqtt')
var client  = mqtt.connect('mqtt://test.mosquitto.org')

client.on('connect', function () {
  client.subscribe('presence')
  client.publish('presence', 'Hello mqtt')
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(message.toString())
  client.end()
})
```
