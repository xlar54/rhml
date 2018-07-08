var net = require('net');
var fs = require('fs');
 
var sockets = [];
var inqueue = "";
/*
 * Method executed when data is received from a socket
 */
function receiveData(socket, data) {
	var d = data;//.toString(); //.toString();
	console.log(d.charCodeAt(0));
	
	if(d.charCodeAt(0) == 13 || d.charCodeAt(0) == 10)
	{
		console.log("CMD:" + inqueue);
		if(inqueue.substring(0,3)=='GET')		{
			
			var page = inqueue.substring(3);
			requestPage(socket, page);	
		}
		if(inqueue.substring(0,3)=='BYE')		{
			
			closeSocket(socket);	
		}
		inqueue = "";
	}
	else
	{
		inqueue += d.trim().toUpperCase();
	}
}

function requestPage(socket, page) {

		console.log("Requested " + page);
		
		// Read the file and print its contents.
		fs.readFile(page, 'utf8', function(err, data) {
			if(err !== null)
			{
				fs.readFile("404.rhml", 'utf8', function(err, data) {
						if(err !== null)
						{
							socket.write("*T,404 FILE NOT FOUND..AND SERVER IS MISSING 404 PAGE\r");
							socket.write("*E,\r");
						}
						else
						{
							socket.write(data);
						}
				});
			}
			else
			{
				socket.write(data);
			}
		});
}
 
/*
 * Method executed when a socket ends
 */
function closeSocket(socket) {
	var i = sockets.indexOf(socket);
	if (i != -1) {
		sockets.splice(i, 1);
	}
}
 
/*
 * Callback method executed when a new TCP socket is opened.
 */
function newSocket(socket) {
	socket.setEncoding('utf8');
	console.log('connected...');
	sockets.push(socket);
	
	socket.write("*C\r");
	socket.write("*C\r");
	requestPage(socket,"index.rhml");
	
	socket.on('data', function(data) {
		receiveData(socket, data);
	})
	socket.on('end', function() {
		closeSocket(socket);
	})
}
 
// Create a new server and provide a callback for when a connection occurs
var server = net.createServer(newSocket);
 
// Listen on port 23
console.log("Listening on port 23");
server.listen(23);