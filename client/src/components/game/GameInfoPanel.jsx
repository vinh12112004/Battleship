"use client";

import { useState } from "react";

export default function GameInfoPanel() {
  const [gameInfo] = useState({
    yourShips: 5,
    enemyShips: 5,
    yourHits: 8,
    enemyHits: 3,
    timeRemaining: 234,
  });

  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, "0")}`;
  };

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6 space-y-4">
      <h2 className="text-lg font-bold text-[#00d9ff] mb-4">Game Info</h2>

      <div className="space-y-3">
        <div className="flex justify-between items-center p-3 bg-[#0f1419] rounded">
          <span className="text-gray-400">Your Ships:</span>
          <span className="font-bold text-[#00d9ff]">{gameInfo.yourShips}</span>
        </div>

        <div className="flex justify-between items-center p-3 bg-[#0f1419] rounded">
          <span className="text-gray-400">Enemy Ships:</span>
          <span className="font-bold text-[#9d4edd]">
            {gameInfo.enemyShips}
          </span>
        </div>

        <div className="flex justify-between items-center p-3 bg-[#0f1419] rounded">
          <span className="text-gray-400">Your Hits:</span>
          <span className="font-bold text-[#00d084]">{gameInfo.yourHits}</span>
        </div>

        <div className="flex justify-between items-center p-3 bg-[#0f1419] rounded">
          <span className="text-gray-400">Enemy Hits:</span>
          <span className="font-bold text-[#ff4757]">{gameInfo.enemyHits}</span>
        </div>

        <div className="flex justify-between items-center p-3 bg-gradient-to-r from-[#00d9ff] from-opacity-20 to-[#9d4edd] to-opacity-20 rounded border border-[#00d9ff] border-opacity-30">
          <span className="text-gray-400">Time Remaining:</span>
          <span className="font-bold text-[#00d9ff]">
            {formatTime(gameInfo.timeRemaining)}
          </span>
        </div>
      </div>

      <button className="w-full py-2 bg-[#ff4757] text-white rounded font-semibold hover:bg-[#ff3344] transition">
        Resign
      </button>
    </div>
  );
}
