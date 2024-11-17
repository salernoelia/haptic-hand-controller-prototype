const osc = require("osc");
const fs = require("fs");
const { log } = require("console");
let oscServerConnectionEstablished = false;

// OSC Server setup for receiving messages
const oscServer = new osc.UDPPort({
  localAddress: "0.0.0.0",
  localPort: 50002,
});

// OSC Client setup for sending messages
const oscClient = new osc.UDPPort({
  remoteAddress: "192.168.1.118",
  remotePort: 50001,
  localAddress: "0.0.0.0",
});

// Function to log incoming OSC messages to signals.json
const logMessageToFile = (message) => {
  const logEntry = {
    timestamp: new Date().toISOString(),
    address: message.address,
    args: message.args.map((arg) =>
      arg.value !== undefined ? arg.value : arg
    ), // Flatten args to simple values
  };

  // Convert the log entry to a JSON string with a newline for separation
  const logEntryString = JSON.stringify(logEntry) + "\n";

  if (logEntry.address !== "/gyro") {
    return;
  }

  // Append the new log entry to signals.json
  fs.appendFile("signals.json", logEntryString, (err) => {
    if (err) {
      console.error("Error writing to signals.json:", err);
    } else {
      console.log("OSC message logged to signals.json");
    }
  });
};

// Initialize OSC Server to receive messages
const startOSCServer = () => {
  oscServer.on("message", (oscMessage) => {
    try {
      console.log("OSC message received:", oscMessage);
      logMessageToFile(oscMessage);
    } catch (error) {
      console.error("Error processing OSC message:", error);
    }
  });
  oscServer.open();
};

// Initialize OSC Client to send a message
const sendOSCMessage = () => {
  oscClient.open();

  oscClient.on("ready", () => {
    console.log("OSC Client ready to send messages");
    oscServerConnectionEstablished = true;

    sendVibrateSignal(100);

    //wait 1s
    setTimeout(() => {
      sendVibrateSignal(200);
    }, 500);
  });
};

const sendVibrateSignal = (duration) => {
  if (!oscServerConnectionEstablished) {
    console.error("OSC Server connection not established");
    return;
  }

  if (!duration) {
    duration = 100;
  }

  oscClient.send({
    address: "/vibrate",
    args: [
      {
        type: "i", // integer type
        value: 1, // data to be sent (1 for ON)
      },
    ],
  });

  setTimeout(() => {
    oscClient.send({
      address: "/vibrate",
      args: [
        {
          type: "i", // integer type
          value: 0, // data to be sent (0 for OFF)
        },
      ],
    });
  }, duration);
};

// Start the server and send a message
startOSCServer();
sendOSCMessage();
