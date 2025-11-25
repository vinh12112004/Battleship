"use client";

import { useState, useEffect } from "react";
import { useParams } from "react-router-dom";
import NavBar from "../components/common/Navbar";
import GameBoard from "../components/game/GameBoard";
import GameInfoPanel from "../components/game/GameInfoPanel";
import GameChat from "../components/game/GameChat";
import { useGame } from "@/hooks/useGame";

export default function GamePage() {
  const { id } = useParams();
  const { gameState, makeMove, sendMessage, isConnected } = useGame();
  const [localGameState, setLocalGameState] = useState({
    yourBoard: Array(100).fill("water"),
    opponentBoard: Array(100).fill("hidden"),
    ships: [],
    phase: "setup",
  });

  useEffect(() => {
    if (gameState) {
      setLocalGameState(gameState);
    }
  }, [gameState]);

  const handleMove = (row, col) => {
    if (isConnected) {
      makeMove(id, row, col);
    }
  };

  return (
    <div className="min-h-screen bg-[#0f1419]">
      <NavBar />
      {/* Connection status indicator */}
      <div className="px-6 py-2 flex justify-end items-center max-w-7xl mx-auto">
        <div
          className={`flex items-center gap-2 text-sm ${
            isConnected ? "text-cyan-400" : "text-red-400"
          }`}
        >
          <div
            className={`w-2 h-2 rounded-full ${
              isConnected ? "bg-cyan-400" : "bg-red-400"
            }`}
          ></div>
          {isConnected ? "Connected" : "Disconnected"}
        </div>
      </div>

      <div className="p-6 max-w-7xl mx-auto">
        <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
          {/* Game boards */}
          <div className="lg:col-span-3">
            <GameBoard gameState={localGameState} onMove={handleMove} />
          </div>

          {/* Right sidebar */}
          <div className="space-y-6">
            <GameInfoPanel gameId={id} />
            <GameChat
              gameId={id}
              messages={localGameState?.messages || []}
              onSendMessage={sendMessage}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
