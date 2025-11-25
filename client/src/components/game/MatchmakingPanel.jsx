import { useState, useEffect } from "react";
import { wsService, MSG_TYPES } from "@/services/wsService";
import { useNavigate } from "react-router-dom";

export default function MatchmakingPanel() {
  const [searching, setSearching] = useState(false);
  const [gameType, setGameType] = useState("ranked");
  const [timeControl, setTimeControl] = useState("5+0");
  const navigate = useNavigate();

  useEffect(() => {
    const handleMatchFound = (payload) => {
      console.log("[Matchmaking] Match found! Opponent:", payload.opponent);
      setSearching(false);

      // ✅ Navigate to game page với gameId
      // Lưu ý: payload.gameId cần được gửi từ server
      if (payload.gameId) {
        navigate(`/game/${payload.gameId}`);
      } else {
        // Fallback: nếu server chưa gửi gameId, tạo temporary ID
        const tempGameId = `game_${Date.now()}`;
        navigate(`/game/${tempGameId}`);
      }
    };

    wsService.onMessage(MSG_TYPES.START_GAME, handleMatchFound);

    return () => {
      wsService.offMessage(MSG_TYPES.START_GAME, handleMatchFound);
    };
  }, [navigate]);

  const handleSearch = () => {
    if (searching) {
      wsService.leaveQueue();
      setSearching(false);
    } else {
      try {
        wsService.joinQueue();
        setSearching(true);
      } catch (error) {
        console.error("[Matchmaking] Failed to join queue:", error);
        alert("Failed to join queue. Please try again.");
      }
    }
  };

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-8">
      <h2 className="text-2xl font-bold text-[#00d9ff] mb-6">Find a Match</h2>

      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-[#00d9ff] mb-2">
            Game Type
          </label>
          <select
            value={gameType}
            onChange={(e) => setGameType(e.target.value)}
            disabled={searching}
            className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] focus:outline-none focus:ring-2 focus:ring-[#00d9ff] disabled:opacity-50"
          >
            <option value="ranked">Ranked Match</option>
            <option value="casual">Casual Match</option>
            <option value="tournament">Tournament</option>
          </select>
        </div>

        <div>
          <label className="block text-sm font-medium text-[#00d9ff] mb-2">
            Time Control
          </label>
          <select
            value={timeControl}
            onChange={(e) => setTimeControl(e.target.value)}
            disabled={searching}
            className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] focus:outline-none focus:ring-2 focus:ring-[#00d9ff] disabled:opacity-50"
          >
            <option value="3+0">3+0 (Blitz)</option>
            <option value="5+0">5+0 (Rapid)</option>
            <option value="10+0">10+0 (Classical)</option>
          </select>
        </div>

        <button
          onClick={handleSearch}
          className={`w-full py-3 font-bold rounded transition transform hover:scale-105 ${
            searching
              ? "bg-[#ff4757] text-white hover:bg-[#ff6b6b]"
              : "bg-gradient-to-r from-[#00d9ff] to-[#9d4edd] text-[#0f1419]"
          }`}
        >
          {searching ? "Cancel Search" : "Search for Opponent"}
        </button>

        {searching && (
          <div className="mt-4 p-4 bg-[#0f1419] rounded border border-[#00d9ff] border-opacity-30 text-center animate-fade-in">
            <div className="flex justify-center mb-3">
              <div className="animate-spin w-8 h-8 border-4 border-[#00d9ff] border-t-[#9d4edd] rounded-full"></div>
            </div>
            <p className="text-[#00d9ff] font-semibold mb-1">
              Searching for opponent...
            </p>
            <p className="text-gray-400 text-sm">ELO range: ±200</p>
          </div>
        )}
      </div>
    </div>
  );
}
