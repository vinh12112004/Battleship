"use client"

import { useState } from "react"

export default function OnlinePlayers() {
  const [players] = useState([
    { id: 1, username: "ShadowFleet", eloRating: 2450, status: "playing" },
    { id: 2, username: "TidalWave", eloRating: 2380, status: "online" },
    { id: 3, username: "NauticalKing", eloRating: 2310, status: "online" },
    { id: 4, username: "CyberSiren", eloRating: 2250, status: "playing" },
    { id: 5, username: "OceanMaster", eloRating: 2180, status: "online" },
  ])

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
      <h2 className="text-xl font-bold text-[#00d9ff] mb-4">Online Players</h2>
      <div className="space-y-3">
        {players.map((player) => (
          <div
            key={player.id}
            className="flex items-center justify-between p-3 bg-[#0f1419] rounded hover:bg-[#1a2a40] transition"
          >
            <div className="flex items-center gap-3">
              <div
                className={`w-3 h-3 rounded-full ${player.status === "playing" ? "bg-[#ff4757]" : "bg-[#00d084]"}`}
              ></div>
              <div>
                <p className="font-semibold text-[#e8f0ff]">{player.username}</p>
                <p className="text-xs text-gray-500">{player.eloRating} ELO</p>
              </div>
            </div>
            <button className="px-3 py-1 text-xs bg-[#00d9ff] bg-opacity-20 text-[#00d9ff] rounded hover:bg-opacity-40 transition">
              Challenge
            </button>
          </div>
        ))}
      </div>
    </div>
  )
}
