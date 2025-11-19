import { createContext, useState, useEffect } from "react";
import { authService } from "@/services/authService";

// eslint-disable-next-line react-refresh/only-export-components
export const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [isInitialized, setIsInitialized] = useState(false);

  useEffect(() => {
    const initAuth = async () => {
      try {
        console.log("[Auth] Initializing authentication...");

        // ✅ Tự động reconnect nếu có token
        const result = await authService.autoReconnect();

        if (result) {
          console.log("[Auth] Auto-reconnect successful:", result.user);
          setUser(result.user);
        } else {
          console.log("[Auth] No valid session found");
        }
      } catch (err) {
        console.error("[Auth] Initialization error:", err);
        authService.clearAuth();
      } finally {
        setLoading(false);
        setIsInitialized(true);
      }
    };

    initAuth();

    // ✅ KHÔNG cleanup WebSocket khi unmount
    return () => {
      console.log("[Auth] Component unmounting, keeping WebSocket alive");
    };
  }, []);

  const value = {
    user,
    setUser,
    loading,
    setLoading,
    error,
    setError,
    isInitialized,
    isAuthenticated: !!user,
  };

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}
