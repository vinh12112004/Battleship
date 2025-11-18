import { wsService, MSG_TYPES } from "./wsService";

const TOKEN_KEY = "auth_token";
const USER_KEY = "auth_user";

export const authService = {
  // ==================== WebSocket Auth ====================
  async register(username, email, password) {
    // Connect to WebSocket if not connected
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
        resolve({ token: payload.token, user: { username: payload.username } });
      };

      const failHandler = (payload) => {
        clearTimeout(timeout);
        wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
        wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
        reject(new Error(payload.reason || "Registration failed"));
      };

      wsService.onMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
      wsService.onMessage(MSG_TYPES.AUTH_FAILED, failHandler);

      // Send register message (wsService.sendMessage đã được cập nhật)
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
        resolve({ token: payload.token, user: { username: payload.username } });
      };

      const failHandler = (payload) => {
        clearTimeout(timeout);
        wsService.offMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
        wsService.offMessage(MSG_TYPES.AUTH_FAILED, failHandler);
        reject(new Error(payload.reason || "Login failed"));
      };

      wsService.onMessage(MSG_TYPES.AUTH_SUCCESS, successHandler);
      wsService.onMessage(MSG_TYPES.AUTH_FAILED, failHandler);

      // Send login message (wsService.sendMessage đã được cập nhật)
      wsService.sendMessage(MSG_TYPES.LOGIN, { username, password });
    });
  },

  async logout() {
    // Gửi tin nhắn logout đến server
    if (wsService.ws && wsService.ws.readyState === WebSocket.OPEN) {
      // Gửi tin nhắn với token hiện tại
      wsService.sendMessage(MSG_TYPES.LOGOUT, {});
    }

    this.clearAuth();
    wsService.logout();
  },

  // ==================== HTTP fallback (keep existing methods) ====================
  async getCurrentUser() {
    // Keep HTTP version for profile fetching
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
      return Date.now() >= expiryTime;
    } catch {
      return true;
    }
  },
};
