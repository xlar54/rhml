var net = require('net');
var fs = require('fs');

var imges = [];
var sockets = [];
var inqueue = "";

function sleep(milliseconds) {
  var start = new Date().getTime();
  for (var i = 0; i < 1e7; i++) {
    if ((new Date().getTime() - start) > milliseconds){
      break;
    }
  }
}

var rle = (function () {
    var decode = function (input) {
        var repeat = /^\d+/.exec(input), character;
        if (repeat === null) {
            return "";
        }

        repeat = repeat[0];
        character = input[repeat.length];
        return new Array(+repeat + 1).join(character) + decode(input.substr((repeat + character).length));
    };

    return {
        encode: function (input) {
            var i = 0, j = input.length, output = "", lastCharacter, currentCharCount;
                for (; i < j; ++i) {
                    if (typeof lastCharacter === "undefined") {
                        lastCharacter = input[i];
                        currentCharCount = 1;
                        continue;
                    }
                    
                    if (input[i] !== lastCharacter) {
                        output += currentCharCount + lastCharacter;
                        lastCharacter = input[i];                        
                        currentCharCount = 1;
                        continue;
                    }
                    
                    currentCharCount++; 
                }

                return output + (currentCharCount + lastCharacter);
        },
        decode: decode   
    };    
}());


/*
 * Method executed when data is received from a socket
 */
function receiveData(socket, data) {
	var d = data;
	
	if(d.charCodeAt(0) == 13 || d.charCodeAt(0) == 10)
	{
		// ignore modem commands
		if(inqueue.substring(0,2)=='+++')		{
			inqueue = "";
			return;
		}
		
		if(inqueue.substring(0,5)=='POST:')		{
			
			// FUTURE...
			inqueue = "";
			return;
				
		}
		if(inqueue.substring(0,3)=='BYE')		{
			
			closeSocket(socket);
			inqueue = "";
			return;
		}
		requestPage(socket, inqueue);
		inqueue = "";
	}
	else
	{
		inqueue += d.trim().toUpperCase();
	}
}

function requestPage(socket, page) {

		sleep(300);
		page = page.toLowerCase();
		
		console.log(socket.remoteAddress + " requested " + page);
		
		fs.stat(page, function(err, stat) {
			
			if(err == null) {
				
				var lineReader = require('readline').createInterface({
				  input: require('fs').createReadStream(page)
				});

				lineReader.on('line', function (line) {
				  
					if(line.substring(0,3) == "*I,")
					{
						imges.push(line.substring(3));
					}
					else
					{
						if(line.substring(0,3) == "*X,")
						{
							var newlin = "";
							line = line.substring(3);
							for(var t=0;t<line.length;t++)
							{
								if(line.charAt(t) != ' ')
									newlin += "X";
								else
									newlin += " ";
							}

							line = "*X," + rle.encode(newlin)+"\r";
						}
								
						for(var v=0;v<line.length;v++)
						{
							// slow down output just a bit (may be machine dependent)
							// without this, some data loss occurs because data send is too fast
							sleep(20);
							socket.write(line.charAt(v));
						}
						sleep(20);
						socket.write("\r");
					}
				  
				}).on('close', function() {
					
					sleep(60);
					if(imges.length == 0)
					{
						socket.write("*E\r");
					}
					else
					{
						page = imges.pop();
						requestPage(socket,page);
					}
					
				});
			} else if(err.code == 'ENOENT') {
				
				fs.readFile("404.rhml", 'utf8', function(err, data) {
				if(err !== null)
				{
					socket.write("*T,404 FILE NOT FOUND..AND SERVER IS MISSING 404 PAGE\r");
					socket.write("*E\r");
				}
				else
				{
					data += "\r*E\r";
					socket.write(data);
				}
				
			});
			} else {
				console.log('Some other error: ', err.code);
			}
		});


		
		// Read the file and print its contents.
		/*fs.readFile(page, 'utf8', function(err, data) {
			if(err !== null)
			{
				fs.readFile("404.rhml", 'utf8', function(err, data) {
						if(err !== null)
						{
							socket.write("*T,404 FILE NOT FOUND..AND SERVER IS MISSING 404 PAGE\r");
							socket.write("*E\r");
						}
						else
						{
							data += "\r*E\r";
							socket.write(data);
						}
				});
			}
			else
			{
				data += "\r*E\r";
				for(var v=0;v<data.length;v++)
				{
					// slow down output just a bit (may be machine dependent)
					// without this, some data loss occurs because data send is too fast
					sleep(10);
					socket.write(data.charAt(v));
				}
				//socket.write(data);
			}
		});*/
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
	console.log(socket.remoteAddress + ' connected');
	sockets.push(socket);
	
	sleep(2000);
	socket.write("\n");
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
console.log("\n\nRHML server listening on port 23");
server.listen(23);