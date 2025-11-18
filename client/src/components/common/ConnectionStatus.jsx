import { useState, useEffect } from "react";
import { wsService } from "@/services/wsService";

export default function ConnectionStatus() {
  const [status, setStatus] = useState("disconnected");
  const [showLogoutToast, setShowLogoutToast] = useState(false);

  useEffect(() => {
    const handleConnectionChange = (state) => {
      console.log("[ConnectionStatus] State changed:", state);
      setStatus(state);

      // ✅ Hiển thị toast khi logout thành công
      if (state === "logged_out") {
        setShowLogoutToast(true);
        setTimeout(() => setShowLogoutToast(false), 3000);
      }
    };

    wsService.onConnectionStateChange(handleConnectionChange);

    // Cleanup (optional, nếu cần remove listener)
    return () => {
      // wsService.removeConnectionStateCallback(handleConnectionChange);
    };
  }, []);

  // ✅ QUAN TRỌNG: Không hiển thị banner khi connected HOẶC logged_out
  if (status === "connected" || status === "logged_out") {
    // Chỉ hiển thị toast logout thành công
    if (showLogoutToast) {
      return (
        <div className="fixed top-4 right-4 bg-green-500 text-white py-3 px-6 rounded-lg shadow-lg z-50 animate-fade-in">
          <span className="mr-2">✓</span>
          Logged out successfully
        </div>
      );
    }
    return null;
  }

  const statusConfig = {
    connecting: {
      text: "Connecting to server...",
      color: "bg-yellow-500",
      icon: "🔄",
    },
    reconnecting: {
      text: "Reconnecting...",
      color: "bg-orange-500",
      icon: "🔄",
    },
    disconnected: {
      text: "Disconnected from server",
      color: "bg-red-500",
      icon: "❌",
    },
    error: {
      text: "Connection error",
      color: "bg-red-600",
      icon: "⚠️",
    },
    failed: {
      text: "Connection failed. Please refresh.",
      color: "bg-red-700",
      icon: "🚫",
    },
  };

  const config = statusConfig[status] || statusConfig.disconnected;

  return (
    <div
      className={`fixed top-0 left-0 right-0 ${config.color} text-white py-2 px-4 text-center z-50 animate-pulse`}
    >
      <span className="mr-2">{config.icon}</span>
      {config.text}
    </div>
  );
}
