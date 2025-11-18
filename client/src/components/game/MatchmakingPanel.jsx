"use client";

import { useState } from "react";

export default function MatchmakingPanel() {
  const [searching, setSearching] = useState(false);

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-8">
      <h2 className="text-2xl font-bold text-[#00d9ff] mb-6">Find a Match</h2>

      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-[#00d9ff] mb-2">
            Game Type
          </label>
          <select className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] focus:outline-none focus:ring-2 focus:ring-[#00d9ff]">
            <option>Ranked Match</option>
            <option>Casual Match</option>
            <option>Tournament</option>
          </select>
        </div>

        <div>
          <label className="block text-sm font-medium text-[#00d9ff] mb-2">
            Time Control
          </label>
          <select className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] focus:outline-none focus:ring-2 focus:ring-[#00d9ff]">
            <option>3+0 (Blitz)</option>
            <option>5+0 (Rapid)</option>
            <option>10+0 (Classical)</option>
          </select>
        </div>

        <button
          onClick={() => setSearching(!searching)}
          className={`w-full py-3 font-bold rounded transition transform hover:scale-105 ${
            searching
              ? "bg-[#ff4757] text-white"
              : "bg-gradient-to-r from-[#00d9ff] to-[#9d4edd] text-[#0f1419]"
          }`}
        >
          {searching ? "Cancel Search" : "Search for Opponent"}
        </button>

        {searching && (
          <div className="mt-4 p-4 bg-[#0f1419] rounded border border-[#00d9ff] border-opacity-30 text-center">
            <div className="animate-spin inline-block w-4 h-4 border-2 border-[#00d9ff] border-t-[#9d4edd] rounded-full mb-2"></div>
            <p className="text-[#00d9ff] text-sm">Searching for opponent...</p>
          </div>
        )}
      </div>
    </div>
  );
}
