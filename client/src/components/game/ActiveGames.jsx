"use client"

import { useState } from "react"
import { Link } from "react-router-dom"

export default function ActiveGames() {
  const [games] = useState([
    { id: 1, opponent: "ShadowFleet", progress: "45%", yourShips: 3, theirShips: 2 },
    { id: 2, opponent: "TidalWave", progress: "20%", yourShips: 4, theirShips: 4 },
  ])

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
      <h2 className="text-xl font-bold text-[#00d9ff] mb-4">Your Active Games</h2>

      {games.length === 0 ? (
        <p className="text-gray-400 text-center py-8">No active games. Start playing!</p>
      ) : (
        <div className="space-y-4">
          {games.map((game) => (
            <Link
              key={game.id}
              to={`/game/${game.id}`}
              className="block p-4 bg-[#0f1419] rounded hover:border-[#00d9ff] border border-[#1a2a40] transition"
            >
              <div className="flex items-center justify-between mb-3">
                <span className="font-semibold text-[#00d9ff]">{game.opponent}</span>
                <span className="text-xs bg-[#00d9ff] bg-opacity-20 text-[#00d9ff] px-2 py-1 rounded">
                  {game.progress}
                </span>
              </div>
              <div className="flex items-center justify-between text-sm text-gray-400">
                <span>Your Ships: {game.yourShips}</span>
                <span>Their Ships: {game.theirShips}</span>
              </div>
            </Link>
          ))}
        </div>
      )}
    </div>
  )
}
