class WebSocketService {
  constructor() {
    this.ws = null;
    this.messageHandlers = new Map();
  }

  connect(url = "ws://localhost:9090") {
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(url);
      this.ws.binaryType = "arraybuffer";

      this.ws.onopen = () => {
        console.log("WebSocket connected");
        resolve();
      };

      this.ws.onerror = (error) => {
        console.error("WebSocket error:", error);
        reject(error);
      };

      this.ws.onmessage = (event) => {
        this.handleMessage(event.data);
      };

      this.ws.onclose = () => {
        console.log("WebSocket disconnected");
      };
    });
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }

  // Serialize message theo format của C struct
  serializeMessage(type, payload) {
    // message_t struct size: 4 bytes (type) + payload
    const MSG_REGISTER = 1;
    const MSG_LOGIN = 2;
    const MAX_JWT_LEN = 512;

    let buffer;

    if (type === MSG_REGISTER || type === MSG_LOGIN) {
      // auth_payload: username[32] + password[32] = 64 bytes
      buffer = new ArrayBuffer(4 + 64);
      const view = new DataView(buffer);
      const uint8 = new Uint8Array(buffer);

      // Write message type (4 bytes, little-endian)
      view.setUint32(0, type, true);

      // Write username (32 bytes)
      const usernameBytes = new TextEncoder().encode(payload.username);
      uint8.set(usernameBytes.slice(0, 31), 4);

      // Write password (32 bytes)
      const passwordBytes = new TextEncoder().encode(payload.password);
      uint8.set(passwordBytes.slice(0, 31), 36);
    } else {
      // Default buffer for other message types
      buffer = new ArrayBuffer(4 + MAX_JWT_LEN);
      const view = new DataView(buffer);
      view.setUint32(0, type, true);
    }

    return buffer;
  }

  // Deserialize response từ server
  deserializeMessage(arrayBuffer) {
    const view = new DataView(arrayBuffer);
    const type = view.getUint32(0, true); // Little-endian

    const MSG_AUTH_SUCCESS = 3;
    const MSG_AUTH_FAILED = 4;
    const MAX_JWT_LEN = 512;

    let payload = {};

    if (type === MSG_AUTH_SUCCESS) {
      // auth_success_payload: token[512] + username[32]
      const uint8 = new Uint8Array(arrayBuffer);

      // Read token (512 bytes starting at offset 4)
      const tokenBytes = uint8.slice(4, 4 + MAX_JWT_LEN);
      const tokenEnd = tokenBytes.indexOf(0);
      payload.token = new TextDecoder().decode(
        tokenBytes.slice(0, tokenEnd > 0 ? tokenEnd : MAX_JWT_LEN)
      );

      // Read username (32 bytes starting at offset 516)
      const usernameBytes = uint8.slice(4 + MAX_JWT_LEN, 4 + MAX_JWT_LEN + 32);
      const usernameEnd = usernameBytes.indexOf(0);
      payload.username = new TextDecoder().decode(
        usernameBytes.slice(0, usernameEnd > 0 ? usernameEnd : 32)
      );
    } else if (type === MSG_AUTH_FAILED) {
      // auth_failed_payload: reason[64]
      const uint8 = new Uint8Array(arrayBuffer);
      const reasonBytes = uint8.slice(4, 4 + 64);
      const reasonEnd = reasonBytes.indexOf(0);
      payload.reason = new TextDecoder().decode(
        reasonBytes.slice(0, reasonEnd > 0 ? reasonEnd : 64)
      );
    }

    return { type, payload };
  }

  sendMessage(type, payload) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error("WebSocket not connected");
    }

    const buffer = this.serializeMessage(type, payload);
    this.ws.send(buffer);
  }

  handleMessage(data) {
    const message = this.deserializeMessage(data);
    const handlers = this.messageHandlers.get(message.type);
    if (handlers) {
      handlers.forEach((handler) => handler(message.payload));
    }
  }

  onMessage(type, handler) {
    if (!this.messageHandlers.has(type)) {
      this.messageHandlers.set(type, []);
    }
    this.messageHandlers.get(type).push(handler);
  }

  offMessage(type, handler) {
    const handlers = this.messageHandlers.get(type);
    if (handlers) {
      const index = handlers.indexOf(handler);
      if (index > -1) {
        handlers.splice(index, 1);
      }
    }
  }
}

export const wsService = new WebSocketService();

// Message type constants
export const MSG_TYPES = {
  REGISTER: 1,
  LOGIN: 2,
  AUTH_SUCCESS: 3,
  AUTH_FAILED: 4,
  JOIN_QUEUE: 5,
  LEAVE_QUEUE: 6,
  START_GAME: 7,
  PLAYER_MOVE: 8,
  MOVE_RESULT: 9,
  GAME_OVER: 10,
  CHAT: 11,
};
