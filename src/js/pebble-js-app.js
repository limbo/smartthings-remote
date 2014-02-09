var client = {
	'auth_uri' : 'http://st-json-auth.herokuapp.com/',
	'end_points': []
};

var pebbleSendQueue = {
	queue: [],
	queueFull: false,
	send: function(msg) {
		if (this.queueFull) {
			this.queue.push(msg);
			return;
		}
		this.queueFull = true;
		this._doSend(msg);
	},
	_sendDone: function(e) {
		if (this.queue.length == 0) {
			this.queueFull = false;
			return;
		}
		var msg = pebbleSendQueue.queue.splice(0,1)[0];
		this._doSend(msg);
	},
	_sendFailed: function(e) {
		var msg = pebbleSendQueue.queue.splice(0,1)[0];
		this._doSend(msg);
	},
	_doSend: function(msg) {
		this.inQueue = msg;
		Pebble.sendAppMessage(msg, function(e) { pebbleSendQueue._sendDone(e) }, function(e) { pebbleSendQueue._sendFailed(e) });
	}
};

function httpGet(url, params, cb) {
	var p = '';
	for (var param in params) {
		if (p != '') {
			p += "&";
		}
		p += param + "=" + params[param];
	}
	url = url + "?" + p;
	console.log("GET " + url);
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.setRequestHeader("Connection", "close");

	req.onload = function() {
		if(req.status == 200) {
			console.log(req.responseText);
			cb(req.responseText);
		} else {
			console.log ("HTTP GET Error: " + req.status);
			Pebble.showSimpleNotificationOnPebble("HTTP Error", "Status code: " + req.status);
		}
	}
	req.onerror = function(e) {
		console.log ("HTTP GET Error: " + e.error);
		Pebble.showSimpleNotificationOnPebble("HTTP Error", e.error);
	}
	req.send();
}

function parse_switch(s) {
	var d = {
		"0": 0, 
		"1": s.id, 
		"2": s.label || "Switch", 
		"3": s.state == "on" ? 1 : 0
		};
	pebbleSendQueue.send(d);
}

function gen_code()
{
    var text = "";
    var possible = "abcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < 5; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function fetch_devices() {
	var token = window.localStorage.getItem('access_token');
	httpGet(client.auth_uri, {'access_token': token}, function(response) {
		data = JSON.parse(response);
		client.end_points[0] = data.switch_endpoint;
		client.end_points[1] = data.lock_endpoint;
		for(var i in data.switches) {
			parse_switch(data.switches[i]);
		}
	});	
}
Pebble.addEventListener("ready",
    function(e) {
				console.log("user_code:" + window.localStorage.getItem('user_code'));
				console.log("access_token:" + window.localStorage.getItem('access_token'));
				var token = window.localStorage.getItem('access_token');
				if (!token) {
					// need to get a token first.
					var user_code = window.localStorage.getItem('user_code');
					if (!user_code) {
						// need to generate a code.
						var user_code = gen_code();
						window.localStorage.setItem('user_code', user_code);
						Pebble.showSimpleNotificationOnPebble("Authentication", "This is your code: " + user_code);
					} else {
						// try to retrieve access_token
						httpGet(client.auth_uri, {'user_code': user_code}, function(response) {
							data = JSON.parse(response);
							console.log("access_token response: " + response);
							if (data.access_token) {
								window.localStorage.setItem('access_token', data.access_token);
								fetch_devices();
							} else {
								Pebble.showSimpleNotificationOnPebble("Authentication", "Not done yet. Your code: " + user_code);
							}
				  	});						
					}
				} else {
					fetch_devices();
				}
});

Pebble.addEventListener("appmessage", function(e) {
	var msg = e.payload;
	console.log("incoming msg: " + JSON.stringify(msg));
	if ('toggle' in msg) {
		id = msg.toggle;
		uri = client.end_points[0] + '/' + id + '/toggle';
		var token = window.localStorage.getItem('access_token');
		httpGet(uri, {'access_token': token}, function(response) {
			uri = client.end_points[0] + '/' + id;
			httpGet(uri, {'access_token': token}, function(response) {
				console.log("rcvd:" + response);
				data = JSON.parse(response);
				parse_switch(data);
		  });
	  });
	}
});

