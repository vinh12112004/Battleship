class WebSocketService {
    constructor() {
        this.ws = null;
        this.messageHandlers = new Map();

        // Định nghĩa cấu trúc từ C struct
        this.MAX_JWT_LEN = 512;
        this.USERNAME_LEN = 32;
        this.PASSWORD_LEN = 32;
        this.REASON_LEN = 64;
        this.CHAT_LEN = 128;
        this.START_GAME_PAYLOAD_LEN = 32;

        // Kích thước của union payload (lấy thành phần lớn nhất)
        // auth_success_payload là lớn nhất: token[512] + username[32] = 544
        this.MAX_PAYLOAD_SIZE = this.MAX_JWT_LEN + this.USERNAME_LEN; // 544

        // Kích thước cố định của toàn bộ message_t
        this.MESSAGE_T_SIZE = 4 + this.MAX_JWT_LEN + this.MAX_PAYLOAD_SIZE; // 4 + 512 + 544 = 1060 bytes

        // Vị trí (Offsets) của các trường trong message_t
        this.OFFSET_TYPE = 0;
        this.OFFSET_TOKEN = 4;
        this.OFFSET_PAYLOAD = 4 + this.MAX_JWT_LEN; // 516
    }

    connect(url = "ws://localhost:9090") {
        return new Promise((resolve, reject) => {
            this.ws = new WebSocket(url);
            this.ws.binaryType = "arraybuffer"; // Rất quan trọng!

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

        // 3. Ghi vào Payload (tại offset 516)
        if (type === MSG_TYPES.REGISTER || type === MSG_TYPES.LOGIN) {
            // Ghi username vào payload (offset 516)
            const usernameBytes = new TextEncoder().encode(payload.username);
            uint8.set(usernameBytes.slice(0, this.USERNAME_LEN - 1), this.OFFSET_PAYLOAD);

            // Ghi password vào payload + 32 (offset 516 + 32 = 548)
            const passwordBytes = new TextEncoder().encode(payload.password);
            uint8.set(passwordBytes.slice(0, this.PASSWORD_LEN - 1), this.OFFSET_PAYLOAD + this.USERNAME_LEN);
        
        } else if (type === MSG_TYPES.PLAYER_MOVE) {
            // Chú ý: Server C dùng ntohl/htonl -> Big Endian cho các trường này
            // Ghi x (offset 516)
            view.setUint32(this.OFFSET_PAYLOAD, payload.x, false); // false = Big-Endian
            // Ghi y (offset 520)
            view.setUint32(this.OFFSET_PAYLOAD + 4, payload.y, false); // false = Big-Endian
        
        } else if (type === MSG_TYPES.CHAT) {
            const chatBytes = new TextEncoder().encode(payload.message);
            uint8.set(chatBytes.slice(0, this.CHAT_LEN - 1), this.OFFSET_PAYLOAD);
        }
        // Thêm các loại tin nhắn khác (JOIN_QUEUE, ...) ở đây

        return buffer;
    }

    /**
     * Giải nén (Deserialize) tin nhắn từ server
     */
    deserializeMessage(arrayBuffer) {
        if (arrayBuffer.byteLength !== this.MESSAGE_T_SIZE) {
            console.error(`Received invalid message size. Got ${arrayBuffer.byteLength}, expected ${this.MESSAGE_T_SIZE}`);
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
            return decoder.decode(bytes.slice(0, nullTerminator > -1 ? nullTerminator : length));
        };

        if (type === MSG_TYPES.AUTH_SUCCESS) {
            // Server gửi: resp.payload.auth_suc.token và resp.payload.auth_suc.username
            
            // Đọc token TỪ BÊN TRONG PAYLOAD (offset 516)
            payload.token = decodeCString(this.OFFSET_PAYLOAD, this.MAX_JWT_LEN);
            
            // Đọc username TỪ BÊN TRONG PAYLOAD (offset 516 + 512 = 1028)
            payload.username = decodeCString(this.OFFSET_PAYLOAD + this.MAX_JWT_LEN, this.USERNAME_LEN);
        
        } else if (type === MSG_TYPES.AUTH_FAILED) {
            // Server gửi: resp.payload.auth_fail.reason
            // Đọc reason TỪ BÊN TRONG PAYLOAD (offset 516)
            payload.reason = decodeCString(this.OFFSET_PAYLOAD, this.REASON_LEN);
        
        } else if (type === MSG_TYPES.MOVE_RESULT) {
            // Đọc từ PAYLOAD (offset 516)
            // Nhớ dùng Big-Endian (false)
            payload.x = view.getUint32(this.OFFSET_PAYLOAD, false);
            payload.y = view.getUint32(this.OFFSET_PAYLOAD + 4, false);
            payload.hit = view.getUint32(this.OFFSET_PAYLOAD + 8, false);
            payload.sunk = decodeCString(this.OFFSET_PAYLOAD + 12, 32); // (Giả sử 32B, cần kiểm tra lại C struct)
        
        } else if (type === MSG_TYPES.START_GAME) {
            payload.opponent = decodeCString(this.OFFSET_PAYLOAD, this.START_GAME_PAYLOAD_LEN);
        }
        // Thêm các loại tin nhắn khác ở đây

        return { type, payload };
    }

    sendMessage(type, payload) {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            throw new Error("WebSocket not connected");
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
    LOGOUT: 12 // Thêm type 12 cho LOGOUT
};