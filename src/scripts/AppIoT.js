// call script using: 'node AppIoT.js <serial device>'
// to check that the device is receiving data you can try: 'cat /dev/rfcomm0'

var httpServiceUri = "https://eappiotsens.servicebus.windows.net";
var httpServicePath = "datacollectoroutbox/publishers/ca929aa5-1676-47b3-8f4f-0e28746eb7dd";
var gatewayID = 'ca929aa5-1676-47b3-8f4f-0e28746eb7dd'; // '<from registration ticket of gateway, tag DataCollectorId>';
var httpSAS = 'SharedAccessSignature sr=https%3a%2f%2feappiotsens.servicebus.windows.net%2fdatacollectoroutbox%2fpublishers%2fca929aa5-1676-47b3-8f4f-0e28746eb7dd%2fmessages&sig=9fbocwM4ViyV%2f30wd4XjbrR%2fb80o%2bkd0xlZKrmy66tA%3d&se=4647142342&skn=SendAccessPolicy';

var blockedSensorID           = 'def922e8-4277-4479-8707-c6142bb58c80';  // border
var distanceSensorID          = 'e46048a2-20cf-4949-8e0a-6cc992e01a5b';  // centimeter
var lightSensorID             = '0a4fce0f-84d3-4bcf-a437-1b50c72f34c4';  // lux
var movementDetectedSensorID  = 'ba8f52e5-18d1-41ac-893a-981dcc02a690';  // boolean
var isMovingSensorID          = 'e97b90be-356b-4337-852d-e9a86b83d27a';  // boolean
var obstacleDetectedSensorID  = 'f5a9b05e-d4d9-43d4-ad96-cc74d4c45d28';  // boolean
var statusSensorID            = '1717c333-a62e-4004-b57c-cd64a4cf8532';  // Text
var tempSensorID              = '6a1353b0-45a5-49e7-af92-b543c6858b4a';  // Celsius
var isWaitingSensorID         = 'ab00b8cc-fc85-4b4e-ae04-b1ef70a4d6cb';  // boolean
var isWatchingSensorID        = 'd38f7adc-bdbe-491a-a3e8-9e431aae76ce';  // boolean

var https = require('https');
var url = require('url');
var moment = require('moment');

var SerialPort = require('serialport');
portName = process.argv[2];

var myPort = new SerialPort(portName, {
  baudRate: 115200,
  parser: SerialPort.parsers.readline('<eom>')
});

myPort.on('open', showPortOpen);
myPort.on('data', sendSerialData);
myPort.on('close', showPortClose);
myPort.on('error', showError);

function showPortOpen() {
  console.log('port open. data rate: ' + myPort.options.baudRate);
}

function sendSerialData(data) {
  console.log('data received.');
  parseData(data);
}

function showPortClose() {
  console.log('port closed.');
}

function showError(error) {
  console.log('Serial port error: ' + error);
}

function parseData(data) {
  var jsonString = data;
  jsonString = jsonString.replace(/\\u00../g, "");
  jsonString = jsonString.replace(/[^a-zA-Z 0-9\{\}:,\.;\"]/g, "");
  console.log('-------------------------------------------------------');
  try {
    obj = JSON.parse(jsonString);
    console.log('temperature: ' + obj['temperature']);
    SendPayload(tempSensorID, obj['temperature']);
    console.log('lightSensor: ' + obj['lightSensor']);
    SendPayload(lightSensorID, obj['lightSensor']);
    console.log('distanceCm: ' + obj['distanceCm']);
    SendPayload(distanceSensorID, obj['distanceCm']);
    console.log('obstacleDetected: ' + obj['obstacleDetected']);
    SendPayload(obstacleDetectedSensorID, obj['obstacleDetected']);
    SendPayload(obstacleDetectedSensorID, obj['obstacleDetected'] ? 1 : 0);
    console.log('blocked: ' + obj['blocked']);
    SendPayload(blockedSensorID, obj['blocked'] ? 1 : 0);
    console.log('watch: ' + obj['watch']);
    SendPayload(isWatchingSensorID, obj['watch'] ? 1 : 0);
    console.log('wait: ' + obj['wait']);
    SendPayload(isWaitingSensorID, obj['wait'] ? 1 : 0);
  } catch (e) {
    console.error(e);
  }
}

function SendPayload(id, value) {
  var timestamp = moment().valueOf();

  var post_req = null,
  post_data = '[{"id":"' + id + '","v":[{"m":[' + value + '],"t":' + timestamp + '}]}]';

  var postURL = httpServiceUri + "/" + httpServicePath + "/messages";
  var reqUrl = url.parse(postURL);

  var post_options = {
    hostname: reqUrl.hostname,
    path: reqUrl.pathname,
    port: 443,
    protocol: 'https:',
    method  : 'POST',
    headers : {
      'Authorization': httpSAS,
      'DataCollectorId': gatewayID,
      'PayloadType': 'Measurements',
      'Timestamp': moment().valueOf(), // This timestamp should be "now"
      'Content-Type': 'application/atom+xml;type=entry;charset=utf-8',
      'Cache-Control': 'no-cache',
      'Content-Length': post_data.length
    }
  };

  post_req = https.request(post_options, function (res) {
    console.log('STATUS: ' + res.statusCode);
    console.log('HEADERS: ' + JSON.stringify(res.headers));
    res.setEncoding('utf8');
    res.on('data', function (chunk) {
      console.log('Response: ', chunk);
    });
  });

  post_req.on('error', function (e) {
    console.log('problem with request: ' + e.message);
  });

  post_req.write(post_data);
  post_req.end();
}
