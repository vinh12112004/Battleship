import { useContext, useCallback } from "react";
import { AuthContext } from "@/context/AuthContext";
import { authService } from "@/services/authService";

export const useAuth = () => {
  const context = useContext(AuthContext);

  if (!context) {
    throw new Error("useAuth must be used within AuthProvider");
  }

  const {
    user,
    setUser,
    loading,
    setLoading,
    error,
    setError,
    isInitialized,
    isAuthenticated,
  } = context;

  // ==================== Register ====================
  const register = useCallback(
    async (username, email, password) => {
      try {
        setError(null);
        setLoading(true);

        const response = await authService.register(username, email, password);

        if (response.token && response.user) {
          authService.setToken(response.token);
          authService.setUser(response.user);
          setUser(response.user);
        }

        return response;
      } catch (err) {
        const errorMessage =
          err.data?.message || err.message || "Registration failed";
        setError(errorMessage);
        throw err;
      } finally {
        setLoading(false);
      }
    },
    [setUser, setLoading, setError]
  );

  // ==================== Login ====================
  const login = useCallback(
    async (username, password) => {
      try {
        setError(null);
        setLoading(true);

        const response = await authService.login(username, password);

        if (response.token && response.user) {
          authService.setToken(response.token);
          authService.setUser(response.user);
          setUser(response.user);
        }

        return response;
      } catch (err) {
        const errorMessage = err.data?.message || err.message || "Login failed";
        setError(errorMessage);
        throw err;
      } finally {
        setLoading(false);
      }
    },
    [setUser, setLoading, setError]
  );

  // ==================== Logout ====================
  const logout = useCallback(async () => {
    try {
      await authService.logout();
    } catch (err) {
      console.error("Logout error:", err);
    } finally {
      authService.clearAuth();
      setUser(null);
      setError(null);
    }
  }, [setUser, setError]);

  // ==================== Update Profile ====================
  const updateProfile = useCallback(
    async (userData) => {
      try {
        setError(null);
        const updatedUser = await authService.updateProfile(userData);
        setUser(updatedUser);
        authService.setUser(updatedUser);
        return updatedUser;
      } catch (err) {
        const errorMessage =
          err.data?.message || err.message || "Update failed";
        setError(errorMessage);
        throw err;
      }
    },
    [setUser, setError]
  );

  // ==================== Change Password ====================
  const changePassword = useCallback(
    async (oldPassword, newPassword) => {
      try {
        setError(null);
        await authService.changePassword(oldPassword, newPassword);
      } catch (err) {
        const errorMessage =
          err.data?.message || err.message || "Password change failed";
        setError(errorMessage);
        throw err;
      }
    },
    [setError]
  );

  // ==================== Refresh User ====================
  const refreshUser = useCallback(async () => {
    try {
      const currentUser = await authService.getCurrentUser();
      setUser(currentUser);
      authService.setUser(currentUser);
      return currentUser;
    } catch (err) {
      console.error("Failed to refresh user:", err);
      throw err;
    }
  }, [setUser]);

  // ==================== Clear Error ====================
  const clearError = useCallback(() => {
    setError(null);
  }, [setError]);

  return {
    user,
    loading,
    error,
    isAuthenticated,
    isInitialized,
    login,
    register,
    logout,
    updateProfile,
    changePassword,
    refreshUser,
    clearError,
  };
};
