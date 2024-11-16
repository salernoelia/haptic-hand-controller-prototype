const osc = require("osc");

// OSC setup
const oscServer = new osc.UDPPort({
  localAddress: "0.0.0.0",
  localPort: 50002,
});

// Initialization
const startOSCServer = () => {
  oscServer.on("message", (oscMessage) => {
    try {
      console.log("OSC message received:", oscMessage);
    } catch (error) {
      console.error("Error processing OSC message:", error);
    }
  });
  oscServer.open();
};

startOSCServer();
