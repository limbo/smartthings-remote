smartthings-remote
==================

A pebble app to remotely control smartthings devices.

### Build instructions:
- Install the pebble SDK from https://developer.getpebble.com/2/getting-started/
- Enable developer mode on your mobile pebble app. This requires Pebble 2.0 on your phone. See this page for more info: https://developer.getpebble.com/2/getting-started/hello-world/
- Build and install the app using the Pebble tool:
`> pebble build`
`> pebble install --phone IP_ADDRESS_OF_PHONE`
The app should now be running on your phone. 

### Setting up the app
- You should be seeing a five digit code on your pebble screen.
- Open http://st-json-auth.herokuapp.com/ in your browser. Type in the code and click submit.
- You should be prompted to log into your smartthings account and then select which switches you want to control.
- Hit authorize and you should see a message telling you to go back to your pebble.
- Quit and restart the app on your pebble and you should be able to see your devices listed.
