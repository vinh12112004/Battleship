import { wsService, MSG_TYPES } from "./wsService";

const TOKEN_KEY = "auth_token";
const USER_KEY = "auth_user";

export const authService = {
    // ==================== WebSocket Auth ====================
    async register(username, email, password) {
        if (!wsService.ws || wsService.ws.readyState !== WebSocket.OPEN) {
            await wsService.connect("ws://localhost:9090");
        }

        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
                reject(new Error("Registration timeout"));
            }, 10000);

            const successHandler = (payload) => {
                clearTimeout(timeout);
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);

                this.setToken(payload.token);
                this.setUser({ username: payload.username });
                resolve({
                    token: payload.token,
                    user: { username: payload.username },
                });
            };

            const failHandler = (payload) => {
                clearTimeout(timeout);
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
                reject(new Error(payload.reason || "Registration failed"));
            };

            wsService.onMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
            wsService.onMessage(MSG_TYPES.AUTH_FAILED, failHandler);

            wsService.sendMessage(MSG_TYPES.REGISTER, { username, password });
        });
    },

    async login(username, password) {
        if (!wsService.ws || wsService.ws.readyState !== WebSocket.OPEN) {
            await wsService.connect("ws://localhost:9090");
        }

        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
                reject(new Error("Login timeout"));
            }, 10000);

            const successHandler = (payload) => {
                clearTimeout(timeout);
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);

                this.setToken(payload.token);
                this.setUser({ username: payload.username });
                resolve({
                    token: payload.token,
                    user: { username: payload.username },
                });
            };

            const failHandler = (payload) => {
                clearTimeout(timeout);
                wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
                wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
                reject(new Error(payload.reason || "Login failed"));
            };

            wsService.onMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
            wsService.onMessage(MSG_TYPES.AUTH_FAILED, failHandler);

            wsService.sendMessage(MSG_TYPES.LOGIN, { username, password });
        });
    },

    async logout() {
        if (wsService.ws && wsService.ws.readyState === WebSocket.OPEN) {
            wsService.sendMessage(MSG_TYPES.LOGOUT, {});
        }

        this.clearAuth();
        wsService.logout();
    },

    // ==================== Auto-login ====================
    // ✅ Tự động reconnect khi có token
    async autoReconnect() {
        const token = this.getToken();
        const cachedUser = this.getUser();

        // Nếu không có token, return null
        if (!token) {
            return null;
        }

        // Kiểm tra token hết hạn
        if (this.isTokenExpired(token)) {
            console.log("[AuthService] Token expired, clearing auth...");
            this.clearAuth();
            return null;
        }

        // Token còn hạn, reconnect WebSocket
        try {
            if (!wsService.isConnected()) {
                console.log(
                    "[AuthService] Reconnecting WebSocket with valid token..."
                );
                await wsService.connect();
            }

            console.log("[AuthService] Auto-reconnect successful");
            return { token, user: cachedUser };
        } catch (error) {
            console.error("[AuthService] WebSocket reconnect failed:", error);
            // Vẫn return user data nếu có (offline mode)
            return cachedUser ? { token, user: cachedUser } : null;
        }
    },

    // ==================== HTTP fallback ====================
    async getCurrentUser() {
        return null;
    },

    // ==================== Local Storage Management ====================
    setToken(token) {
        if (token) {
            localStorage.setItem(TOKEN_KEY, token);
        }
    },

    getToken() {
        return localStorage.getItem(TOKEN_KEY);
    },

    removeToken() {
        localStorage.removeItem(TOKEN_KEY);
    },

    setUser(user) {
        if (user) {
            localStorage.setItem(USER_KEY, JSON.stringify(user));
        }
    },

    getUser() {
        const user = localStorage.getItem(USER_KEY);
        return user ? JSON.parse(user) : null;
    },

    removeUser() {
        localStorage.removeItem(USER_KEY);
    },

    clearAuth() {
        this.removeToken();
        this.removeUser();
    },

    isAuthenticated() {
        const token = this.getToken();
        if (!token) return false;
        return !this.isTokenExpired(token);
    },

    isTokenExpired(token) {
        if (!token) return true;

        try {
            const payload = JSON.parse(atob(token.split(".")[1]));
            const expiryTime = payload.exp * 1000;
            // ✅ Token hết hạn nếu còn < 5 phút
            return Date.now() >= expiryTime - 5 * 60 * 1000;
        } catch {
            return true;
        }
    },
};
