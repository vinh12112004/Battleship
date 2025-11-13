"use client";

import { Link, useLocation } from "react-router-dom";
import { useState } from "react";

export default function NavBar() {
  const location = useLocation();
  const [isOpen, setIsOpen] = useState(false);

  const isActive = (path) =>
    location.pathname === path
      ? "text-[#00d9ff]"
      : "text-gray-400 hover:text-[#00d9ff]";

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

          <div className="hidden md:flex items-center gap-8">
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
            <Link
              to="/login"
              className="px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded font-semibold hover:bg-[#00ffd9] transition"
            >
              Sign In
            </Link>
          </div>

          <button
            onClick={() => setIsOpen(!isOpen)}
            className="md:hidden text-[#00d9ff]"
          >
            ☰
          </button>
        </div>

        {isOpen && (
          <div className="md:hidden mt-4 space-y-3 pb-4">
            <Link to="/dashboard" className="block text-[#00d9ff]">
              Dashboard
            </Link>
            <Link to="/leaderboard" className="block text-[#00d9ff]">
              Leaderboard
            </Link>
            <Link to="/profile" className="block text-[#00d9ff]">
              Profile
            </Link>
            <Link
              to="/login"
              className="block px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded font-semibold text-center"
            >
              Sign In
            </Link>
          </div>
        )}
      </div>
    </nav>
  );
}
