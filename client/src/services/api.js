import { authService } from "./authService";

const API_BASE_URL = import.meta.env.VITE_API_URL || "http://localhost:9090";

class ApiError extends Error {
  constructor(message, status, data) {
    super(message);
    this.name = "ApiError";
    this.status = status;
    this.data = data;
  }
}

let isRefreshing = false;
let refreshSubscribers = [];

function subscribeTokenRefresh(callback) {
  refreshSubscribers.push(callback);
}

function onTokenRefreshed(token) {
  refreshSubscribers.forEach((callback) => callback(token));
  refreshSubscribers = [];
}

async function request(endpoint, options = {}) {
  const url = `${API_BASE_URL}${endpoint}`;
  let token = authService.getToken();

  if (
    token &&
    authService.shouldRefreshToken(token) &&
    !endpoint.includes("/auth/login") &&
    !endpoint.includes("/auth/register")
  ) {
    if (!isRefreshing) {
      isRefreshing = true;
      try {
        const response = await authService.refreshToken();
        authService.setToken(response.token);
        token = response.token;
        onTokenRefreshed(token);
      } catch {
        authService.clearAuth();
        window.location.href = "/login";
        throw new ApiError("Session expired", 401, {});
      } finally {
        isRefreshing = false;
      }
    } else {
      // Wait for token refresh
      token = await new Promise((resolve) => {
        subscribeTokenRefresh(resolve);
      });
    }
  }

  const config = {
    ...options,
    headers: {
      "Content-Type": "application/json",
      ...(token && { Authorization: `Bearer ${token}` }),
      ...options.headers,
    },
  };

  try {
    const response = await fetch(url, config);

    // Handle 401 Unauthorized
    if (response.status === 401 && !endpoint.includes("/auth/")) {
      authService.clearAuth();
      window.location.href = "/login";
      throw new ApiError("Unauthorized", 401, {});
    }

    const data = await response.json();

    if (!response.ok) {
      throw new ApiError(
        data.message || "Request failed",
        response.status,
        data
      );
    }

    return data;
  } catch (error) {
    if (error instanceof ApiError) throw error;
    throw new ApiError("Network error", 0, { message: error.message });
  }
}

export const api = {
  get: (endpoint, options) => request(endpoint, { ...options, method: "GET" }),
  post: (endpoint, data, options) =>
    request(endpoint, {
      ...options,
      method: "POST",
      body: JSON.stringify(data),
    }),
  put: (endpoint, data, options) =>
    request(endpoint, {
      ...options,
      method: "PUT",
      body: JSON.stringify(data),
    }),
  patch: (endpoint, data, options) =>
    request(endpoint, {
      ...options,
      method: "PATCH",
      body: JSON.stringify(data),
    }),
  delete: (endpoint, options) =>
    request(endpoint, { ...options, method: "DELETE" }),
};

export { ApiError };
