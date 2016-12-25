var Service, Characteristic;
var request = require('sync-request');

var temperatureService;
var humidityService;
var lightService;
var url 
var humidity = 0;
var temperature = 0;
var light = 0;

module.exports = function (homebridge) {
    Service = homebridge.hap.Service;
    Characteristic = homebridge.hap.Characteristic;
    homebridge.registerAccessory("homebridge-httptemperaturehumidity", "HttpTemphum", HttpTemphum);
}


function HttpTemphum(log, config) {
    this.log = log;

    // url info
    this.url = config["url"];
    this.http_method = config["http_method"] || "GET";
    this.sendimmediately = config["sendimmediately"] || "";
    this.name = config["name"];
    this.manufacturer = config["manufacturer"] || "Luca Manufacturer";
    this.model = config["model"] || "Luca Model";
    this.serial = config["serial"] || "Luca Serial";
}

HttpTemphum.prototype = {

    httpRequest: function (url, body, method, username, password, sendimmediately, callback) {
        request({
                    url: url,
                    body: body,
                    method: method,
                    rejectUnauthorized: false
                },
                function (error, response, body) {
                    callback(error, response, body)
                })
    },

    getStateHumidity: function(callback){    
	callback(null, this.humidity);
    },

    getStateLight: function(callback){    
    callback(null, this.light);
    },

    getState: function (callback) {
        var body;
    try {
	  var res = request(this.http_method, this.url, {});
    } catch (error) {
        this.log(error.message);
        callback(error);
    } finally { 
      if (res==null) {
        this.log('Endpoint not avaible');
        // callback('error: endpoint not avaible');
      } else if(res.statusCode > 400){
	    this.log('HTTP power function failed');
	    callback(error);
	  } else {
	    this.log('HTTP power function succeeded!');
          var info = JSON.parse(res.body);

          temperatureService.setCharacteristic(Characteristic.CurrentTemperature, info.temperature);
          humidityService.setCharacteristic(Characteristic.CurrentRelativeHumidity, info.humidity);
          lightService.setCharacteristic(Characteristic.CurrentAmbientLightLevel, info.Lux);

          this.log(res.body);
          this.log(info);

          this.temperature = info.temperature;
          this.humidity = info.humidity;
          if (info.Lux>0) { 
            this.light = info.Lux;
          } else {
            this.light = 1;
          }

	    callback(null, this.temperature);
	    }
      }
    },

    identify: function (callback) {
        this.log("Identify requested!");
        callback(); // success
    },

    getServices: function () {
        var informationService = new Service.AccessoryInformation();
        informationService
                .setCharacteristic(Characteristic.Manufacturer, this.manufacturer)
                .setCharacteristic(Characteristic.Model, this.model)
                .setCharacteristic(Characteristic.SerialNumber, this.serial);

        temperatureService = new Service.TemperatureSensor(this.name);
        temperatureService
                .getCharacteristic(Characteristic.CurrentTemperature)
                .on('get', this.getState.bind(this));

        humidityService = new Service.HumiditySensor(this.name);
        humidityService
                .getCharacteristic(Characteristic.CurrentRelativeHumidity)
                .on('get', this.getStateHumidity.bind(this));

        lightService = new Service.LightSensor(this.name);
        lightService
                .getCharacteristic(Characteristic.CurrentAmbientLightLevel)
                .on('get', this.getStateLight.bind(this));            
        

        return [informationService, temperatureService, humidityService, lightService];
    }
};
