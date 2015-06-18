Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('http://127.0.0.1/foursliders.html');
  //Pebble.openURL('https://dl.dropboxusercontent.com/u/13130748/pebble/foursliders.html');
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    //Get JSON dictionary
    console.log(e.response);
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
    if ( e.response.length === 0 ) {
      return;
    }
 
    //Send to Pebble, persist there
    Pebble.sendAppMessage(
      {"KEY_THEME": configuration.theme,
       "KEY_STATUS": configuration.status},
      function(e) {
        console.log("Sending settings data...");
      },
      function(e) {
        console.log("Settings feedback failed! " + e.error.message);
      }
    );
  }
);