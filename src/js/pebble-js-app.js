Pebble.addEventListener("showConfiguration",
                     function(e) {
                        Pebble.openURL("http://dhertz.github.io/words-date-pebble-watchface/");
					 });

Pebble.addEventListener("webviewclosed",
                   function(e) {
	                   //webview closed
	                   console.log("webview closed");
	                   console.log(e.response);
	                   var responseFromWebView = decodeURIComponent(e.response);
	                   var settings = JSON.parse(responseFromWebView);
	                   console.log(JSON.stringify(settings));
	                   Pebble.sendAppMessage(settings);
					});