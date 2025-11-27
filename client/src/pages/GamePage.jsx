"use client";

import { useState, useEffect } from "react";
import { useParams } from "react-router-dom";
import NavBar from "../components/common/Navbar";
import GameBoard from "../components/game/GameBoard";
import GameInfoPanel from "../components/game/GameInfoPanel";
import GameChat from "../components/game/GameChat";
import { useGame } from "@/hooks/useGame";
import ShipDisplay from "../components/game/ShipDisplay.jsx";
import BattleshipSeaBackground from "../components/game/BattleshipSeaBackground.jsx";

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
        <div className="min-h-screen relative overflow-hidden">
            {/* Animated Sea Background Component */}
            <BattleshipSeaBackground />

            {/* Main Content */}
            <div className="relative z-10">
                <NavBar />

                {/* Header Section with Connection Status */}
                <div className="px-6 py-4 bg-gradient-to-b from-slate-900/90 via-slate-900/60 to-transparent backdrop-blur-sm border-b border-cyan-400/20">
                    <div className="max-w-7xl mx-auto flex flex-col md:flex-row justify-between items-center gap-4">
                        <div>
                            <h1 className="text-3xl md:text-4xl font-bold text-cyan-400 tracking-widest uppercase drop-shadow-[0_0_20px_rgba(56,189,248,0.8)]">
                                Battleship Command
                            </h1>
                            <p className="text-cyan-300 text-xs tracking-[0.2em] font-light opacity-80 mt-1">
                                TACTICAL WARFARE SYSTEM
                            </p>
                        </div>

                        {/* Connection Status */}
                        <div
                            className={`flex items-center gap-2 px-4 py-2 rounded-lg backdrop-blur-sm border ${
                                isConnected
                                    ? "bg-cyan-900/30 border-cyan-400/30 text-cyan-400"
                                    : "bg-red-900/30 border-red-400/30 text-red-400"
                            }`}
                        >
                            <div
                                className={`w-2.5 h-2.5 rounded-full ${
                                    isConnected
                                        ? "bg-cyan-400 animate-ping"
                                        : "bg-red-400"
                                } shadow-[0_0_10px_currentColor]`}
                            ></div>
                            <span className="text-sm font-mono tracking-wider uppercase">
                                {isConnected ? "ONLINE" : "OFFLINE"}
                            </span>
                        </div>
                    </div>
                </div>

                {/* Game Content */}
                <div className="p-6 max-w-7xl mx-auto">
                    <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
                        {/* Game boards */}
                        <div className="lg:col-span-3">
                            <GameBoard
                                gameState={localGameState}
                                onMove={handleMove}
                            />
                            <ShipDisplay />
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

                {/* Footer Section */}
                <div className="px-6 py-3 bg-gradient-to-t from-slate-900/90 via-slate-900/60 to-transparent backdrop-blur-sm border-t border-cyan-400/20 mt-6">
                    <div className="max-w-7xl mx-auto">
                        <div className="flex flex-col md:flex-row items-center justify-between text-sm gap-2">
                            <div className="text-cyan-300/70 font-mono tracking-wider">
                                GAME ID:{" "}
                                <span className="text-cyan-400">{id}</span>
                            </div>
                            <div className="text-cyan-300/70 font-mono tracking-wider">
                                PHASE:{" "}
                                <span className="text-cyan-400 uppercase">
                                    {localGameState?.phase || "LOADING"}
                                </span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}
