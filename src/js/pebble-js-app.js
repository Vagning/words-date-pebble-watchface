Pebble.addEventListener("showConfiguration",
                     function(e) {
                     	console.log("connect!" + e.ready);
                        Pebble.openURL("http://192.168.1.13:8000/server-side/");
                        //Pebble.openURL("http://www.google.com");
                        console.log(e.type);
					 });

Pebble.addEventListener("webviewclosed",
                   function(e) {
	                   //webview closed
	                   console.log("webview closed");
                       console.log(e.type);
                       console.log(e.response);
	                   var responseFromWebView = decodeURIComponent(e.response);
	                   var settings = JSON.parse(responseFromWebView);
	                   console.log(settings);
	                   Pebble.sendAppMessage(settings);
					});