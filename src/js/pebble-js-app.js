var Weather = {
  SUNNY: 0,
  CLOUDY: 1,
  RAIN: 2,
};

var clientId = '386489136328-0lmhbc842t5n8mk9cpcoumq5m9bk011q.apps.googleusercontent.com';
var state = {};

function sendPacket() {
  Pebble.sendAppMessage({
    "main": state.weather && state.weather.main || 0,
    "icon": parseInt(state.weather && state.weather.icon || 0),
    "temp": state.weather && state.weather.temp || 0,
    "rain": state.weather && state.weather.rain || 0,
    "code": state.api && state.api.code || '',
    "location": state.location || 'LOCATION MISSING'
    }, function(e) {
    console.log('Message sent.');
  }, function(e) {
    console.log('Failed to send a message.');
  });
}

function requestSimpleApi(url, callback, options) {
  options = options || {};
  console.log('Making a simple api call to: ' + url);
  var req = new XMLHttpRequest();
  req.open(options.method || 'GET', url, true);
  console.log(options.method);
  console.log(options.data);
  if (options.origin)
    req.setRequestHeader('Origin', options.origin);
  if (options.data) {
    req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    req.setRequestHeader('Content-Length', options.data.length);
  }
  req.onload = function(e) {
    console.log('Loaded:', req.readyState, req.status);
    console.log('1' + req.responseText + '2');
    if (req.readyState == 4) {
      console.log('Loaded with success.');
      if (req.status == 200)
        callback(JSON.parse(req.responseText));
      else
        callback({});
    }
  };
  req.onerror = function(e) {
    console.log('Error: ' + e);
    callback({});
  };
  console.log('Sending a request.');
  req.send(options.data || null);
}

function requestGoogleApiCode(callback) {
  var url = 'https://accounts.google.com/o/oauth2/device/code';
  var scope = 'https://www.googleapis.com/auth/calendar.readonly';
  requestSimpleApi(url, callback, {
    method: 'POST',
    data: 'client_id=' + clientId + '&scope=' + scope
  });
}

function updateWeather(callback) {
  navigator.geolocation.getCurrentPosition(function(position) {
    var coords = position.coords;
    console.log('Position fetched.');
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
              coords.latitude + '&lon=' + coords.longitude + '&units=metric';
    var foreacastUrl = 'http://api.openweathermap.org/data/2.5/forecast/daily?lat=' +
              coords.latitude + '&lon=' + coords.longitude + '&units=metric&cnt=1';
    console.log('Fetching weather: ' + url);
    requestSimpleApi(url, function(result) {
      if (!result) {
        state.weather = {};
        callback();
        return;
      }
      var main = result.weather[0].main;
      var icon = result.weather[0].icon;
      var temp = result.main.temp;
      state.weather = {
        main: main,
        icon: icon,
        temp: temp
      };
      requestSimpleApi(foreacastUrl, function(result) {
        var rain = result.list[0].rain || -1;
        if (!result)
          delete state.weather.rain;
        console.log('Weather fetched:', main, icon, temp, rain);
        callback();
      });
    });
  });
}

function updateCalendar(callback) {
  callback();
}

function updateLocation(callback) {
  navigator.geolocation.getCurrentPosition(function(position) {
    var coords = position.coords;
    console.log('Position fetched.');
    var key = 'AIzaSyD8Fon74O4r4iwGqDaW9dQvavcJ5m-UELQ';
    var url = 'https://maps.googleapis.com/maps/api/geocode/json?latlng=' + coords.latitude +
        ',' + coords.longitude + '&sensor=true&key=' + key + '&result_type=sublocality_level_1';
    var options = {origin: 'https://www.getpebble.com',
                   referer: 'https://www.getpebble.com'};
    console.log('Fetching location: ' + url);
    requestSimpleApi(url, function(result) {
      if (!result) {
        state.location = {};
        callback();
        return;
      }
      state.location = result.results[0].formatted_address.toUpperCase();
      callback();
    }, options);
  });
}
                  
function update() {
  var count = 3;
  var maybeFinish = function() {
    count--;
    if (!count) {
      sendPacket();
      localStorage.setItem("state", JSON.stringify(state));
    }
  };
  updateWeather(maybeFinish);
  updateLocation(maybeFinish);
  updateCalendar(maybeFinish);
}

Pebble.addEventListener("ready", function(e) {
  try {
    state = JSON.parse(localStorage.getItem("state")) || {};
  }
  catch (err) {
    console.log('Failed to parse.');
    state = {};
  }
  sendPacket();
  update();
});

Pebble.addEventListener("appmessage", function(e) {
  console.log('Updating requested...');
  update();
});

Pebble.addEventListener("showConfiguration", function(e) {
  var url = 'https://jovis.nazwa.pl/stormycloud/configure.html';
  Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
  var configuration = JSON.parse(decodeURIComponent(e.response));
  console.log("Configuration window returned: ", JSON.stringify(configuration));
});
