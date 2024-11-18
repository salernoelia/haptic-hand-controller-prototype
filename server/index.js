// server.js
const osc = require("osc");
const WebSocket = require("ws");
const { log } = require("console");

// OSC Server setup for receiving messages
const oscServer = new osc.UDPPort({
  localAddress: "0.0.0.0",
  localPort: 50002,
});

// OSC Client setup for sending messages
const oscClient = new osc.UDPPort({
  remoteAddress: "192.168.1.118", // Update to ESP32's IP if necessary
  remotePort: 50001,
  localAddress: "0.0.0.0",
});

// Start OSC Server
oscServer.on("ready", () => {
  console.log("OSC Server is listening on port 50002");
});

// Initialize WebSocket Server
const wss = new WebSocket.Server({ port: 8080 });
console.log("WebSocket Server is listening on port 8080");

// Broadcast function to send data to all connected clients
wss.broadcast = function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};

// Handle incoming OSC messages
oscServer.on("message", (oscMessage) => {
  try {
    console.log("OSC message received:", oscMessage);

    // Handle orientation data
    if (oscMessage.address === "/orientation") {
      const [theta, phi] = oscMessage.args;
      console.log(`Orientation - Theta: ${theta.toFixed(2)}, Phi: ${phi.toFixed(2)}`);

      // Create a JSON object to send to WebSocket clients
      const orientationData = JSON.stringify({ theta, phi });
      wss.broadcast(orientationData);
    }

    // Handle other OSC messages as needed
    if (oscMessage.address === "/vibrate") {
      const vibrateState = oscMessage.args[0].value;
      console.log(`Vibrate: ${vibrateState}`);
      // Implement vibration logic or relay it to connected devices if needed
    }
  } catch (error) {
    console.error("Error processing OSC message:", error);
  }
});

// Open OSC Server
oscServer.open();

// Open OSC Client
oscClient.open();

// Function to send vibrate signal via OSC
const sendVibrateSignal = (duration) => {
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

// Example: Listen for WebSocket client messages to trigger vibration
wss.on("connection", (ws) => {
  console.log("WebSocket client connected");

  ws.on("message", (message) => {
    console.log("Received from client:", message);
    // Example: Trigger vibration based on client command
    if (message === "vibrate") {
      sendVibrateSignal(100); // Vibrate for 100 ms
    }
  });

  ws.on("close", () => {
    console.log("WebSocket client disconnected");
  });
});
