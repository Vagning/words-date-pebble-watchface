Pebble.addEventListener("showConfiguration",
                     function(e) {
                     	console.log("connect!" + e.ready);
                        Pebble.openURL("http://dhertz.github.io/words-date-pebble-watchface/");
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