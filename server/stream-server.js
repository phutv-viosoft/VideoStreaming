var STREAM_MAGIC_BYTES = 'jsmp'; // Must be 4 bytes
var width = 320, height = 240;
var clientStreams = [];
var clientServices = [];

/*****************Websocket Server******************/
// Websocket Server 1 using stream video flows
var socketServer1 = new (require('ws').Server)({port: 9001});
socketServer1.on('connection', function(socket) {
  var streamHeader = new Buffer(8);
  streamHeader.write(STREAM_MAGIC_BYTES);
  streamHeader.writeUInt16BE(width, 4);
  streamHeader.writeUInt16BE(height, 6);
  socket.send(streamHeader, {binary:true});
  
  socket.onmessage = function(evt) {
    if(evt.data != "CANCEL") {
      clientStreams.forEach(function (item, index) {
	for(var i in item.websocket) {
	  if(item.websocket[i] == socket) {
	    item.websocket.splice(i,1);
	  }
	}
      });
      clientStreams.forEach(function (item, index) {
      if(item.name == evt.data) {
	item.websocket.push(socket);
      }
    });
	clientStreams[0].response.write("DATA@"+"RPCAMERA_00001");
    }
    if(evt.data == "CANCEL"){
      clientStreams.forEach(function (item, index) {
	for(var i in item.websocket) {
	  if(item.websocket[i] == socket) {
	    item.websocket.splice(i,1);
	  }
	}
      });
//       item.websocket.splice(websocket,1);
     // clientStreams[0].response.write("PAUSE@"+"RPCAMERA_00001");
    }
  }
  socket.on('close', function(code, message){
    clientStreams.forEach(function (item, index) {
      for(var i in item.websocket) {
	if(item.websocket[i] == socket) {
	  item.websocket.splice(i,1);
	}
      }
    });
  });
});
socketServer1.broadcast = function(data, opts, websocket) {
  for( var i in websocket ) {
    if (websocket[i].readyState == 1) {
      websocket[i].send(data, opts);
    }
    else {
      console.log( 'Error: Client ('+i+') not connected.' );
    }
  }
};

/******HTTP Server to accept incomming MPEG Stream******/
var streamServer = require('http').createServer( function(request, response) {
  var params = request.url.substr(1).split('/');
//   console.log('[PhuTran] New client connected');
  //Set timeout for restart server schedule
  //Here, timeout is 2147483647 milliseconds (approximately 25 days) 
  request.setTimeout(2147483647 * 1000); 
  /*Receive from client*/
  request.on('data', function(data) {
    //socketServer1.broadcast(data, {binary:true});
    var tmp = data.toString().split("@",2);    
    if(tmp[0] == "REQUEST") {
      var is_exist = 0;
      clientStreams.forEach(function (item, index) {
	if(item.name == tmp[1] && item.response == response)
	  is_exist = 1;
      });
      if(is_exist == 0) {
	var ws = [];
	var client = {"name": tmp[1], "response": response, "websocket": ws};
	clientStreams.push(client);	
      }
      response.write("REQUEST"+"@"+"200 OK");
    } else /*if(tmp[0] == "DATA")*/ {
      socketServer1.broadcast(data, {binary:true}, clientStreams[0].websocket);
    }
  });
  request.on('close', function() {
    var client;
    clientStreams.forEach(function (item, index) {
	if(item.response == response)
	  client = index;
      });
    clientStreams.splice(client,1);
  });
  request.on('end', function() {
    console.log('[PhuTran] Problem with request');
  });
}).listen(8080);
