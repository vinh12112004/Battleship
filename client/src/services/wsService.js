// Message type constants
const MSG_TYPES = {
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
  LOGOUT: 12,
  PING: 13,
  PONG: 14,
  PLACE_SHIP: 15,
  PLAYER_READY: 16,
  GET_ONLINE_PLAYERS: 17,
  ONLINE_PLAYERS_LIST: 18,
  CHALLENGE_PLAYER: 19,
  CHALLENGE_RECEIVED: 20,
  CHALLENGE_ACCEPT: 21,
  CHALLENGE_DECLINE: 22,
  CHALLENGE_DECLINED: 23,
  CHALLENGE_EXPIRED: 24,
  CHALLENGE_CANCEL: 25,
  CHALLENGE_CANCELLED: 26,
  AUTH_TOKEN: 27,
};

class WebSocketService {
  constructor() {
    if (WebSocketService.instance) {
      return WebSocketService.instance;
    }
    this.ws = null;
    this.messageHandlers = new Map();
    this.url = null;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 2000; // 2 seconds
    this.isManualDisconnect = false;
    this.connectionStateCallbacks = [];
    this.autoReconnectEnabled = true;
    this.isLoggedOut = false;
    this.logoutTimeout = null;
    // Định nghĩa cấu trúc từ C struct
    this.MAX_JWT_LEN = 512;
    this.USERNAME_LEN = 32;
    this.PASSWORD_LEN = 32;
    this.REASON_LEN = 64;
    this.CHAT_LEN = 128;
    this.START_GAME_PAYLOAD_LEN = 32;

    const PLACE_SHIP_SIZE = 16; // ship_type(4) + row(4) + col(4) + is_horizontal(1) + padding(3)
    const MOVE_SIZE = 73; // ✅ CORRECT: game_id(65) + row(4) + col(4) = 73 bytes (packed)
    const MOVE_RESULT_SIZE = 16; // ✅ row(4) + col(4) + is_hit(1) + is_sunk(1) + sunk_ship_type(4) + game_over(1) + padding(1)
    const START_GAME_SIZE = 128; // opponent[32] + game_id[64] + current_turn[32]
    const READY_SIZE = 165; // game_id[65] + board_state[100]
    const AUTH_SUCCESS_SIZE = this.MAX_JWT_LEN + this.USERNAME_LEN; // 512 + 32 = 544

    // Thành phần lớn nhất là online_players_payload:
    // count(4) + players(50*64) + elo(50*4) + ranks(50*32) = 4 + 3200 + 200 + 1600 = 5004
    this.MAX_PAYLOAD_SIZE = 5004;

    // Kích thước cố định của toàn bộ message_t
    this.MESSAGE_T_SIZE = 4 + this.MAX_JWT_LEN + this.MAX_PAYLOAD_SIZE;

    // Vị trí (Offsets) của các trường trong message_t
    this.OFFSET_TYPE = 0;
    this.OFFSET_TOKEN = 4;
    this.OFFSET_PAYLOAD = 4 + this.MAX_JWT_LEN; // 516

    WebSocketService.instance = this;
  }

  connect(url = "ws://localhost:9090") {
    this.url = url;

    // Nếu đã connected, trả về Promise resolved
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.log("[WS] Already connected");
      return Promise.resolve();
    }

    // Nếu đang connecting, đợi connection hiện tại
    if (this.ws && this.ws.readyState === WebSocket.CONNECTING) {
      console.log("[WS] Already connecting, waiting...");
      return this.waitForConnection();
    }

    this.isManualDisconnect = false;
    this.isLoggedOut = false;

    if (this.logoutTimeout) {
      clearTimeout(this.logoutTimeout);
      this.logoutTimeout = null;
    }

    return new Promise((resolve, reject) => {
      // Đóng kết nối cũ chỉ khi nó đang CLOSING (không đóng OPEN hoặc CONNECTING)
      if (this.ws && this.ws.readyState === WebSocket.CLOSING) {
        console.log("[WS] Closing old connection...");
        this.ws.close();
      }

      console.log(`[WS] Creating new connection to ${url}...`);
      this.ws = new WebSocket(url);
      this.ws.binaryType = "arraybuffer";

      this.connectResolve = resolve;
      this.connectReject = reject;

      this.ws.onopen = () => {
        this.reconnectAttempts = 0;
        console.log(`[WS] ✅ Connected to ${url}`);
        this.notifyConnectionState("connected");
        this.startPing(); // gửi ping mỗi 30s
        const token = localStorage.getItem("auth_token");
        if (token) {
          console.log("[WS] Auto-authenticating with existing token...");

          // Send token to server for re-authentication
          this.sendMessage(MSG_TYPES.AUTH_TOKEN, { token });
        }
        resolve();
      };

      this.ws.onerror = (error) => {
        console.error("[WS] ❌ Connection error:", error);
        this.notifyConnectionState("error");
        reject(error);
      };

      this.ws.onmessage = (event) => {
        this.handleMessage(event.data);
      };

      this.ws.onclose = (event) => {
        console.log(
          `[WS] 🔌 Disconnected (code: ${event.code}, reason: ${event.reason})`
        );
        this.stopPing(); // dừng khi disconnect
        if (!this.isLoggedOut) {
          this.notifyConnectionState("disconnected");
        }

        // Auto-reconnect nếu không phải manual disconnect
        if (
          !this.isManualDisconnect &&
          this.autoReconnectEnabled &&
          this.reconnectAttempts < this.maxReconnectAttempts
        ) {
          this.reconnectAttempts++;
          console.log(
            `[WS] 🔄 Reconnecting... (Attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`
          );
          this.notifyConnectionState("reconnecting");

          setTimeout(() => {
            this.connect(this.url).catch((err) => {
              console.error("[WS] Reconnect failed:", err);
            });
          }, this.reconnectDelay);
        } else if (this.reconnectAttempts >= this.maxReconnectAttempts) {
          console.error("[WS] ❌ Max reconnect attempts reached. Giving up.");
          this.notifyConnectionState("failed");
        }
      };
    });
  }

  /**
   * Đăng ký callback để theo dõi trạng thái kết nối
   * States: 'connecting', 'connected', 'disconnected', 'reconnecting', 'error', 'failed'
   */
  onConnectionStateChange(callback) {
    this.connectionStateCallbacks.push(callback);
  }

  notifyConnectionState(state) {
    this.connectionStateCallbacks.forEach((cb) => cb(state));
  }

  enableAutoReconnect(enabled = true) {
    this.autoReconnectEnabled = enabled;
  }

  startPing() {
    this.stopPing(); // Clear existing interval

    this.pingInterval = setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        console.log("[WS] Sending ping...");

        // Gửi MSG_PING để keep-alive
        const buffer = new ArrayBuffer(this.MESSAGE_T_SIZE);
        const view = new DataView(buffer);
        view.setUint32(0, MSG_TYPES.PING, true); // ✅ Dùng constant

        this.ws.send(buffer);
      }
    }, 30000); // 30 seconds
  }

  stopPing() {
    if (this.pingInterval) {
      clearInterval(this.pingInterval);
      this.pingInterval = null;
    }
  }

  disconnect() {
    this.stopPing();
    this.isManualDisconnect = true;
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    if (!this.isLoggedOut) {
      this.notifyConnectionState("disconnected");
    }
    console.log("[WS] Manual disconnect");
  }

  logout() {
    this.stopPing();
    console.log("[WS] logout() called");

    // ✅ 1. Set flag NGAY LẬP TỨC
    this.isLoggedOut = true;
    this.isManualDisconnect = true;

    // ✅ 2. Notify logged_out state TRƯỚC khi đóng socket
    this.notifyConnectionState("logged_out");

    // ✅ 3. Delay một chút để state được xử lý
    this.logoutTimeout = setTimeout(() => {
      console.log("[WS] Closing socket after logout delay");

      if (this.ws) {
        this.ws.close(1000, "User logout"); // Normal closure
        this.ws = null;
      }

      this.logoutTimeout = null;
    }, 100); // 100ms delay

    console.log("[WS] logout() finished, socket will close in 100ms");
  }

  /**
   * Kiểm tra trạng thái kết nối
   */
  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }

  /**
   * Đợi kết nối sẵn sàng (dùng trong trường hợp reconnecting)
   */
  waitForConnection(timeout = 5000) {
    return new Promise((resolve, reject) => {
      if (this.isConnected()) {
        resolve();
        return;
      }

      const startTime = Date.now();
      const checkInterval = setInterval(() => {
        if (this.isConnected()) {
          clearInterval(checkInterval);
          resolve();
        } else if (Date.now() - startTime > timeout) {
          clearInterval(checkInterval);
          reject(new Error("Connection timeout"));
        }
      }, 100);
    });
  }

  /**
   * Đóng gói (Serialize) tin nhắn ĐÚNG theo C struct message_t (kích thước cố định 1060 bytes)
   */
  serializeMessage(type, payload, token = "") {
    const buffer = new ArrayBuffer(this.MESSAGE_T_SIZE);
    const view = new DataView(buffer);
    const uint8 = new Uint8Array(buffer);

    // 1. Ghi Type (4 bytes)
    // Giả định server C chạy trên x86 (Little-Endian) vì server C không chuyển đổi 'type'
    view.setUint32(this.OFFSET_TYPE, type, true); // true = Little-Endian

    // 2. Ghi Token cấp cao nhất (512 bytes)
    // (Chỉ cần thiết cho các tin nhắn yêu cầu xác thực)
    if (token) {
      const tokenBytes = new TextEncoder().encode(token);
      uint8.set(tokenBytes.slice(0, this.MAX_JWT_LEN - 1), this.OFFSET_TOKEN);
    }

    if (type === MSG_TYPES.AUTH_TOKEN) {
      console.log("[WS] Serialized MSG_AUTH_TOKEN with token");
      return buffer;
    }

    // 3. Ghi vào Payload (tại offset 516)
    if (type === MSG_TYPES.REGISTER || type === MSG_TYPES.LOGIN) {
      // Ghi username vào payload (offset 516)
      const usernameBytes = new TextEncoder().encode(payload.username);
      uint8.set(
        usernameBytes.slice(0, this.USERNAME_LEN - 1),
        this.OFFSET_PAYLOAD
      );

      // Ghi password vào payload + 32 (offset 516 + 32 = 548)
      const passwordBytes = new TextEncoder().encode(payload.password);
      uint8.set(
        passwordBytes.slice(0, this.PASSWORD_LEN - 1),
        this.OFFSET_PAYLOAD + this.USERNAME_LEN
      );
    } else if (type === MSG_TYPES.PLACE_SHIP) {
      // ✅ STRUCT: ship_type(4) + row(4) + col(4) + is_horizontal(1) + padding(3) = 16 bytes

      // ✅ DEBUG: Log giá trị trước khi ghi
      console.log("[WS] PLACE_SHIP payload:", {
        ship_type: payload.ship_type,
        row: payload.row,
        col: payload.col,
        is_horizontal: payload.is_horizontal,
        offset_payload: this.OFFSET_PAYLOAD,
      });

      // Ghi Little-Endian (server là x86)
      view.setInt32(this.OFFSET_PAYLOAD, payload.ship_type, true); // offset 0-3
      view.setInt32(this.OFFSET_PAYLOAD + 4, payload.row, true); // offset 4-7
      view.setInt32(this.OFFSET_PAYLOAD + 8, payload.col, true); // offset 8-11
      view.setUint8(this.OFFSET_PAYLOAD + 12, payload.is_horizontal ? 1 : 0); // offset 12

      // ✅ Ghi padding = 0
      view.setUint8(this.OFFSET_PAYLOAD + 13, 0);
      view.setUint8(this.OFFSET_PAYLOAD + 14, 0);
      view.setUint8(this.OFFSET_PAYLOAD + 15, 0);

      // ✅ DEBUG: In ra hex dump
      const hexDump = Array.from(
        new Uint8Array(buffer, this.OFFSET_PAYLOAD, 16)
      )
        .map((b) => b.toString(16).padStart(2, "0"))
        .join(" ");
      console.log("[WS] PLACE_SHIP hex dump (first 16 bytes):", hexDump);
    } else if (type === MSG_TYPES.PLAYER_MOVE) {
      // Serialize: game_id (65) + row (4) + col (4) = 73 bytes
      const gameIdBytes = new TextEncoder().encode(payload.game_id);
      uint8.set(gameIdBytes.slice(0, 64), this.OFFSET_PAYLOAD);
      uint8[this.OFFSET_PAYLOAD + 64] = 0; // Null terminator

      view.setInt32(this.OFFSET_PAYLOAD + 65, payload.row, true);
      view.setInt32(this.OFFSET_PAYLOAD + 69, payload.col, true);

      console.log(
        `[WS] Serialized PLAYER_MOVE: game_id=${payload.game_id}, row=${payload.row}, col=${payload.col}`
      );
    } else if (type === MSG_TYPES.CHAT) {
      const chatBytes = new TextEncoder().encode(payload.message);
      uint8.set(chatBytes.slice(0, this.CHAT_LEN - 1), this.OFFSET_PAYLOAD);
    } else if (type === MSG_TYPES.PLAYER_READY) {
      // ✅ Đảm bảo gửi đúng thứ tự: game_id (65 bytes) + board_state (100 bytes)

      // 1. Write game_id (65 bytes)
      const gameIdBytes = new TextEncoder().encode(payload.game_id);
      uint8.set(gameIdBytes.slice(0, 64), this.OFFSET_PAYLOAD);
      uint8[this.OFFSET_PAYLOAD + 64] = 0; // Null terminator

      // 2. Write board_state (100 bytes)
      if (payload.board_state && Array.isArray(payload.board_state)) {
        const boardBytes = new Uint8Array(payload.board_state);
        uint8.set(boardBytes.slice(0, 100), this.OFFSET_PAYLOAD + 65);
      }
    } else if (type === MSG_TYPES.CHALLENGE_PLAYER) {
      // challenge_payload struct (packed):
      //   char challenger_id[64];    // offset 0-63   (server fills from token)
      //   char target_id[64];        // offset 64-127
      //   char challenge_id[65];     // offset 128-192 (server generates)
      //   char game_mode[32];        // offset 193-224
      //   int time_control;          // offset 225-228
      // TOTAL: 229 bytes
      const targetIdBytes = new TextEncoder().encode(payload.target_id);
      uint8.set(targetIdBytes.slice(0, 63), this.OFFSET_PAYLOAD + 64);
      uint8[this.OFFSET_PAYLOAD + 64 + 63] = 0; // Null terminator

      // Skip challenge_id (offset 128-192) - server will generate

      // Write game_mode at offset 193 (from OFFSET_PAYLOAD)
      const gameModeBytes = new TextEncoder().encode(
        payload.game_mode || "casual"
      );
      uint8.set(gameModeBytes.slice(0, 31), this.OFFSET_PAYLOAD + 193);
      uint8[this.OFFSET_PAYLOAD + 193 + 31] = 0; // Null terminator

      //Write time_control at offset 225 (from OFFSET_PAYLOAD)
      view.setInt32(
        this.OFFSET_PAYLOAD + 225,
        payload.time_control || 10,
        true
      );

      console.log("[WS] CHALLENGE_PLAYER serialized:", {
        target_id: payload.target_id,
        game_mode: payload.game_mode,
        time_control: payload.time_control,
        payload_start_offset: this.OFFSET_PAYLOAD,
      });
    } else if (
      type === MSG_TYPES.CHALLENGE_ACCEPT ||
      type === MSG_TYPES.CHALLENGE_DECLINE ||
      type === MSG_TYPES.CHALLENGE_CANCEL
    ) {
      // challenge_id (65)
      const challengeIdBytes = new TextEncoder().encode(payload.challenge_id);
      uint8.set(challengeIdBytes.slice(0, 64), this.OFFSET_PAYLOAD);
    }
    // Thêm các loại tin nhắn khác (JOIN_QUEUE, ...) ở đây

    return buffer;
  }

  /**
   * Giải nén (Deserialize) tin nhắn từ server
   */
  deserializeMessage(arrayBuffer) {
    if (arrayBuffer.byteLength !== this.MESSAGE_T_SIZE) {
      console.error(
        `Received invalid message size. Got ${arrayBuffer.byteLength}, expected ${this.MESSAGE_T_SIZE}`
      );
      return null;
    }

    const view = new DataView(arrayBuffer);
    const uint8 = new Uint8Array(arrayBuffer);
    const decoder = new TextDecoder();

    // Đọc Type (offset 0)
    const type = view.getUint32(this.OFFSET_TYPE, true); // Little-Endian
    let payload = {};

    // Hàm tiện ích để đọc chuỗi C (kết thúc bằng \0)
    const decodeCString = (offset, length) => {
      const bytes = uint8.slice(offset, offset + length);
      const nullTerminator = bytes.indexOf(0);
      return decoder.decode(
        bytes.slice(0, nullTerminator > -1 ? nullTerminator : length)
      );
    };

    if (type === MSG_TYPES.AUTH_SUCCESS) {
      // Server gửi: resp.payload.auth_suc.token và resp.payload.auth_suc.username

      // Đọc token TỪ BÊN TRONG PAYLOAD (offset 516)
      payload.token = decodeCString(this.OFFSET_PAYLOAD, this.MAX_JWT_LEN);

      // Đọc username TỪ BÊN TRONG PAYLOAD (offset 516 + 512 = 1028)
      payload.username = decodeCString(
        this.OFFSET_PAYLOAD + this.MAX_JWT_LEN,
        this.USERNAME_LEN
      );
    } else if (type === MSG_TYPES.AUTH_FAILED) {
      // Server gửi: resp.payload.auth_fail.reason
      // Đọc reason TỪ BÊN TRONG PAYLOAD (offset 516)
      payload.reason = decodeCString(this.OFFSET_PAYLOAD, this.REASON_LEN);
    } else if (type === MSG_TYPES.MOVE_RESULT) {
      // Deserialize: row(4) + col(4) + is_hit(1) + is_sunk(1) + sunk_ship_type(4) + game_over(1) + is_your_shot(1)
      payload.row = view.getInt32(this.OFFSET_PAYLOAD, true);
      payload.col = view.getInt32(this.OFFSET_PAYLOAD + 4, true);
      payload.is_hit = view.getUint8(this.OFFSET_PAYLOAD + 8) === 1;
      payload.is_sunk = view.getUint8(this.OFFSET_PAYLOAD + 9) === 1;
      payload.sunk_ship_type = view.getInt32(this.OFFSET_PAYLOAD + 10, true);
      payload.game_over = view.getUint8(this.OFFSET_PAYLOAD + 14) === 1;
      payload.is_your_shot = view.getUint8(this.OFFSET_PAYLOAD + 15) === 1;

      console.log("[WS] MOVE_RESULT deserialized:", payload);
    } else if (type === MSG_TYPES.START_GAME) {
      // ✅ DESERIALIZE START_GAME: opponent (32 bytes) + game_id (64 bytes) + current_turn[32] = 128 bytes
      payload.opponent = decodeCString(this.OFFSET_PAYLOAD, 32);
      payload.game_id = decodeCString(this.OFFSET_PAYLOAD + 32, 64);
      payload.current_turn = decodeCString(this.OFFSET_PAYLOAD + 96, 32);
      // ✅ DEBUG LOG
      console.log("[WS] <<<< START_GAME received:", {
        type,
        opponent: payload.opponent,
        game_id: payload.game_id,
        opponent_length: payload.opponent.length,
        game_id_length: payload.game_id.length,
      });
    } else if (type === MSG_TYPES.ONLINE_PLAYERS_LIST) {
      // ✅ Deserialize danh sách players
      // Struct: count(4) + players[50][64] + elo_ratings[50*4] + ranks[50][32]

      payload.count = view.getInt32(this.OFFSET_PAYLOAD, true);
      payload.players = [];

      let offset = this.OFFSET_PAYLOAD + 4; // Bắt đầu sau count

      // Đọc 50 usernames (mỗi username 64 bytes)
      const usernames = [];
      for (let i = 0; i < 50; i++) {
        const username = decodeCString(offset, 64);
        usernames.push(username);
        offset += 64;
      }
      // Đọc 50 elo_ratings (mỗi int 4 bytes)
      const eloRatings = [];
      for (let i = 0; i < 50; i++) {
        const elo = view.getInt32(offset, true);
        eloRatings.push(elo);
        offset += 4;
      }
      // Đọc 50 ranks (mỗi rank 32 bytes)

      const ranks = [];
      for (let i = 0; i < 50; i++) {
        const rank = decodeCString(offset, 32);
        ranks.push(rank);
        offset += 32;
      }

      // Chỉ lấy số lượng players thực tế
      for (let i = 0; i < payload.count; i++) {
        payload.players.push({
          username: usernames[i],
          eloRating: eloRatings[i],
          rank: ranks[i],
        });
      }

      console.log(
        `[WS] Received ${payload.count} online players:`,
        payload.players
      );
    } else if (type === MSG_TYPES.CHALLENGE_RECEIVED) {
      const challenger_username = this.readString(
        uint8,
        this.OFFSET_PAYLOAD,
        64
      );
      const challenger_id = this.readString(
        uint8,
        this.OFFSET_PAYLOAD + 64,
        64
      );
      const challenge_id = this.readString(
        uint8,
        this.OFFSET_PAYLOAD + 128,
        65
      );
      const game_mode = this.readString(uint8, this.OFFSET_PAYLOAD + 193, 32);
      const time_control = view.getInt32(this.OFFSET_PAYLOAD + 225, true);
      const expires_at = Number(
        view.getBigInt64(this.OFFSET_PAYLOAD + 229, true)
      );

      return {
        type,
        payload: {
          challenger_username,
          challenger_id,
          challenge_id,
          game_mode,
          time_control,
          expires_at,
        },
      };
    } else if (
      type === MSG_TYPES.CHALLENGE_DECLINED ||
      type === MSG_TYPES.CHALLENGE_EXPIRED ||
      type === MSG_TYPES.CHALLENGE_CANCELLED
    ) {
      const challenge_id = this.readString(uint8, this.OFFSET_PAYLOAD, 65);

      return {
        type,
        payload: {
          challenge_id,
        },
      };
    }

    // Thêm các loại tin nhắn khác ở đây

    return { type, payload };
  }

  sendMessage(type, payload) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error("WebSocket not connected");
    }

    if (type === MSG_TYPES.AUTH_TOKEN) {
      const token = payload.token || payload; // Accept both {token: "..."} or "..."
      const buffer = this.serializeMessage(type, {}, token); // Empty payload, token in token field
      this.ws.send(buffer);
      console.log("[WS] Sent MSG_AUTH_TOKEN");
      return;
    }

    // Lấy token từ authService (hoặc localStorage) nếu tin nhắn cần xác thực
    let token = "";
    if (type !== MSG_TYPES.REGISTER && type !== MSG_TYPES.LOGIN) {
      // Bạn cần triển khai hàm getToken() này trong authService
      token = localStorage.getItem("auth_token") || "";
    }

    // Gửi tin nhắn đã được đóng gói đúng
    const buffer = this.serializeMessage(type, payload, token);
    this.ws.send(buffer);
  }

  handleMessage(data) {
    const message = this.deserializeMessage(data);
    if (!message) return; // Bỏ qua tin nhắn không hợp lệ

    const handlers = this.messageHandlers.get(message.type);
    if (handlers) {
      handlers.forEach((handler) => handler(message.payload));
    }
  }

  // (onMessage và offMessage giữ nguyên)
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

  joinQueue() {
    const token = localStorage.getItem("auth_token");
    if (!token) {
      throw new Error("Not authenticated");
    }

    const buffer = new ArrayBuffer(this.MESSAGE_T_SIZE);
    const view = new DataView(buffer);

    // Set message type
    view.setUint32(0, MSG_TYPES.JOIN_QUEUE, true);

    // Set token
    const tokenBytes = new TextEncoder().encode(token);
    const uint8 = new Uint8Array(buffer);
    uint8.set(tokenBytes.slice(0, this.MAX_JWT_LEN - 1), 4);

    this.ws.send(buffer);
    console.log("[WS] Joined matchmaking queue");
  }

  leaveQueue() {
    const token = localStorage.getItem("auth_token");
    if (!token) return;

    const buffer = new ArrayBuffer(this.MESSAGE_T_SIZE);
    const view = new DataView(buffer);

    view.setUint32(0, MSG_TYPES.LEAVE_QUEUE, true);

    const tokenBytes = new TextEncoder().encode(token);
    const uint8 = new Uint8Array(buffer);
    uint8.set(tokenBytes.slice(0, this.MAX_JWT_LEN - 1), 4);

    this.ws.send(buffer);
    console.log("[WS] Left matchmaking queue");
  }

  readString(uint8Array, offset, maxLength) {
    const bytes = uint8Array.slice(offset, offset + maxLength);
    const nullIndex = bytes.indexOf(0);
    const actualBytes = nullIndex >= 0 ? bytes.slice(0, nullIndex) : bytes;
    return new TextDecoder().decode(actualBytes);
  }
}

// Export instance thay vì class
export const wsService = new WebSocketService();

export { MSG_TYPES };
