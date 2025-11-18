import { createContext, useState, useEffect } from "react";
import { authService } from "@/services/authService";
import { wsService } from "@/services/wsService";
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
        const token = authService.getToken();
        const cachedUser = authService.getUser();

        if (token && !authService.isTokenExpired(token)) {
          if (cachedUser) {
            setUser(cachedUser);
          }

          try {
            if (!wsService.isConnected()) {
              console.log('[Auth] Reconnecting WebSocket...');
              await wsService.connect();
              console.log('[Auth] WebSocket reconnected');
            }
            const currentUser = await authService.getCurrentUser();
            setUser(currentUser);
            authService.setUser(currentUser);
          } catch (err) {
            console.error("Failed to fetch current user:", err);
            if (!cachedUser) {
              authService.clearAuth();
              setUser(null);
            }
          }
        } else {
          authService.clearAuth();
        }
      } catch (err) {
        console.error("Auth initialization error:", err);
        authService.clearAuth();
      } finally {
        setLoading(false);
        setIsInitialized(true);
      }
    };

    initAuth();
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
