"use client";

import { Link, useLocation, useNavigate } from "react-router-dom";
import { useState } from "react";
import { useAuth } from "@/hooks/useAuth";

export default function NavBar() {
  const location = useLocation();
  const navigate = useNavigate();
  const [isOpen, setIsOpen] = useState(false);

  // ✅ Lấy thông tin auth từ context
  const { isAuthenticated, user, logout, loading } = useAuth();

  const isActive = (path) =>
    location.pathname === path
      ? "text-[#00d9ff]"
      : "text-gray-400 hover:text-[#00d9ff]";

  // ✅ Handler logout
  const handleLogout = async () => {
    try {
      await logout();
      navigate("/login");
    } catch (error) {
      console.error("Logout failed:", error);
    }
  };

  return (
    <nav className="bg-[#1a2332] border-b border-[#00d9ff] border-opacity-20">
      <div className="max-w-7xl mx-auto px-6 py-4">
        <div className="flex items-center justify-between">
          <Link to="/" className="flex items-center gap-2">
            <div className="w-8 h-8 bg-gradient-to-br from-[#00d9ff] to-[#9d4edd] rounded flex items-center justify-center">
              <span className="text-white font-bold">⚓</span>
            </div>
            <span className="text-xl font-bold text-[#00d9ff]">Battleship</span>
          </Link>

          {/* Desktop Menu */}
          <div className="hidden md:flex items-center gap-8">
            {isAuthenticated && (
              <>
                <Link
                  to="/dashboard"
                  className={`transition ${isActive("/dashboard")}`}
                >
                  Dashboard
                </Link>
                <Link
                  to="/leaderboard"
                  className={`transition ${isActive("/leaderboard")}`}
                >
                  Leaderboard
                </Link>
                <Link
                  to="/profile"
                  className={`transition ${isActive("/profile")}`}
                >
                  Profile
                </Link>

                {/* ✅ Hiển thị username */}
                <span className="text-[#00d9ff] text-sm">
                  👤 {user?.username || "User"}
                </span>
              </>
            )}

            {/* ✅ Auth Button - Login hoặc Logout */}
            {isAuthenticated ? (
              <button
                onClick={handleLogout}
                disabled={loading}
                className="px-4 py-2 bg-[#ff4757] text-white rounded font-semibold hover:bg-[#ff6b6b] transition disabled:opacity-50 disabled:cursor-not-allowed"
              >
                {loading ? "Signing out..." : "Sign Out"}
              </button>
            ) : (
              <Link
                to="/login"
                className="px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded font-semibold hover:bg-[#00ffd9] transition"
              >
                Sign In
              </Link>
            )}
          </div>

          {/* Mobile Menu Button */}
          <button
            onClick={() => setIsOpen(!isOpen)}
            className="md:hidden text-[#00d9ff]"
          >
            ☰
          </button>
        </div>

        {/* Mobile Menu */}
        {isOpen && (
          <div className="md:hidden mt-4 space-y-3 pb-4">
            {isAuthenticated ? (
              <>
                <Link
                  to="/dashboard"
                  className="block text-[#00d9ff]"
                  onClick={() => setIsOpen(false)}
                >
                  Dashboard
                </Link>
                <Link
                  to="/leaderboard"
                  className="block text-[#00d9ff]"
                  onClick={() => setIsOpen(false)}
                >
                  Leaderboard
                </Link>
                <Link
                  to="/profile"
                  className="block text-[#00d9ff]"
                  onClick={() => setIsOpen(false)}
                >
                  Profile
                </Link>

                {/* ✅ Username mobile */}
                <div className="text-[#00d9ff] text-sm py-2 border-t border-[#00d9ff] border-opacity-20">
                  👤 {user?.username || "User"}
                </div>

                {/* ✅ Logout button mobile */}
                <button
                  onClick={() => {
                    handleLogout();
                    setIsOpen(false);
                  }}
                  disabled={loading}
                  className="block w-full px-4 py-2 bg-[#ff4757] text-white rounded font-semibold text-center hover:bg-[#ff6b6b] transition disabled:opacity-50"
                >
                  {loading ? "Signing out..." : "Sign Out"}
                </button>
              </>
            ) : (
              <>
                <Link
                  to="/login"
                  className="block px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded font-semibold text-center"
                  onClick={() => setIsOpen(false)}
                >
                  Sign In
                </Link>
                <Link
                  to="/register"
                  className="block px-4 py-2 border-2 border-[#00d9ff] text-[#00d9ff] rounded font-semibold text-center hover:bg-[#00d9ff] hover:text-[#0f1419] transition"
                  onClick={() => setIsOpen(false)}
                >
                  Register
                </Link>
              </>
            )}
          </div>
        )}
      </div>
    </nav>
  );
}
