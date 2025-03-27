const char home[] = "<!DOCTYPE html><html>\
<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
<link rel=\"icon\" href=\"data:,\">\
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\
.button { background-color: #555555; border: none; color: white; padding: 16px 40px;\
text-decoration: none; font-size: 10px; margin: 2px; cursor: pointer;}\
.buttonSelected { background-color:rgb(38, 173, 47); border: none; color: white; padding: 16px 40px;\
text-decoration: none; font-size: 10px; margin: 2px; cursor: pointer;}\
</style></head>\
<body>\
<script>function myFunction(ssid) {\
let passwd = prompt(\"Enter password for \" + ssid);\
if (passwd != null) {\
var xhr = new XMLHttpRequest();\
xhr.open(\"POST\", '/connect', true);\
xhr.setRequestHeader('Content-Type', 'application/json');\
xhr.send(JSON.stringify({\
    ssid : ssid,\
    passwd : passwd\
}));\
}\
}\
</script>\
<h1> Strava Dashboard</h1>\
<h2> Wifi Configuration</h2>\n";

const char test[] = "<!DOCTYPE html>\
<html>\
<body>\
\
<h1>The Window Object</h1>\
<h2>The prompt() Method</h2>\
\
<p>Click the button to demonstrate the prompt box.</p>\
\
</body>\
</html>";

const char refreshPage[] = "<!DOCTYPE html>\
<html>\
<head>\
<meta http-equiv =\"refresh\" content=\"2\" >\
</head>\
<body>\
<h1>Scan in progress...</h1>\
</body>\
</html>";

const char connectingPage[] = "<!DOCTYPE html>\
<html>\
<head>\
<meta http-equiv =\"refresh\" content=\"5\" >\
</head>\
<body>\
<h1>Connecting to wifi...</h1>\
</body>\
</html>";
const char connectedPage[] = "<!DOCTYPE html>\
<html>\
<head>\
</head>\
<body>\
<h1>Connected ! </h1>\
</body>\
</html>";

const char redirectToHome[] = "<!DOCTYPE html>\
<html>\
<head>\
<meta http-equiv =\"refresh\" content=\"5; url=/\" >\
</head>\
<body>\
<h1> Connection failed </h1>\
<h2> Redirecting to home page </h2>\
</body>\
</html>";

const char portalPage[] = "<!DOCTYPE html>\
<html>\
<body>\
\
<p><button onclick=\"window.open('/wifi')\" class=\"button\"> Click here to setup wifi</button></a></p>\
\
</body>\
</html>";

const char configOK[] = "<!DOCTYPE html>\
<html>\
<body>\
\
<h1>Configuration OK</h1>\
<h2>You can close this window</h2>\
\
</body>\
</html>";

const char configKO[] = "<!DOCTYPE html>\
<html>\
<body>\
\
<h1>Configuration failed</h1>\
<h2>You can close this window</h2>\
\
</body>\
</html>";

const char homeAP[] = "<!DOCTYPE html>\
<html>\
<head>\
    <style>\
        html {font-family: Helvetica;display: inline-block;margin: 0px auto;text-align: center;}button {display: block;margin: 0 auto;}\
    </style>\
</head>\
<body>\
    <h1>Dashboard Configuration</h1>\
    <p><button type=\"button\" onclick=\"location.href='/wifi'\">Add WiFi connexion</button>\
        </p>\
</body>\
</html>";

const char homeSTAStart[] = "<!DOCTYPE html>\
<html>\
<head>\
    <style>\
        html {font-family: Helvetica;display: inline-block;margin: 0px auto;text-align: center;}button {display: block;margin: 0 auto;}\
    </style>\
</head>\
<body>\
    <h1>Dashboard Configuration</h1>\
    <p><button type=\"button\" onclick=\"location.href='/wifi'\">Add WiFi connexion</button>\
    <input type=\"image\" onclick=\"location.href='";
const char homeSTAEnd[] = "'\" src=\"data:image/jpeg;base64,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\">\
</p>\
</body> </html > ";