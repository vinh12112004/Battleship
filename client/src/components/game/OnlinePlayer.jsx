import { useState, useEffect } from "react";
import { wsService, MSG_TYPES } from "../../services/wsService";
import { toast } from "react-toastify";

export default function OnlinePlayers() {
  const [players, setPlayers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [challengesSent, setChallengesSent] = useState(new Set());

  useEffect(() => {
    // Handler nhận danh sách players từ server
    const handleOnlinePlayersList = (payload) => {
      console.log("[OnlinePlayers] Received players:", payload);

      if (payload.players && Array.isArray(payload.players)) {
        setPlayers(payload.players);
        setLoading(false);
        setError(null);
      }
    };

    const handleChallengeDeclined = (payload) => {
      console.log("[Challenge] Declined:", payload);
      toast.error("Challenge was declined", {
        autoClose: 3000,
      });

      // Remove from sent challenges
      setChallengesSent((prev) => {
        const newSet = new Set(prev);
        newSet.delete(payload.challenge_id);
        return newSet;
      });
    };

    const handleChallengeCancelled = (payload) => {
      console.log("[Challenge] Cancelled:", payload);
      toast.info("Challenge was cancelled", {
        autoClose: 3000,
      });
    };

    const handleChallengeExpired = (payload) => {
      console.log("[Challenge] Expired:", payload);
      toast.warning("⏰ Challenge expired", {
        autoClose: 3000,
      });

      // Remove from sent challenges
      setChallengesSent((prev) => {
        const newSet = new Set(prev);
        newSet.delete(payload.challenge_id);
        return newSet;
      });
    };

    wsService.onMessage(MSG_TYPES.CHALLENGE_DECLINED, handleChallengeDeclined);
    wsService.onMessage(
      MSG_TYPES.CHALLENGE_CANCELLED,
      handleChallengeCancelled
    );
    wsService.onMessage(MSG_TYPES.CHALLENGE_EXPIRED, handleChallengeExpired);
    // Đăng ký handler
    wsService.onMessage(MSG_TYPES.ONLINE_PLAYERS_LIST, handleOnlinePlayersList);

    // Request danh sách players khi component mount
    const fetchPlayers = () => {
      try {
        if (wsService.isConnected()) {
          // ✅ Dùng sendMessage() trực tiếp, không cần payload
          wsService.sendMessage(MSG_TYPES.GET_ONLINE_PLAYERS, {});
        } else {
          setError("WebSocket not connected");
          setLoading(false);
        }
      } catch (err) {
        console.error("[OnlinePlayers] Error:", err);
        setError(err.message);
        setLoading(false);
      }
    };

    fetchPlayers();

    // Auto-refresh mỗi 10 giây
    const refreshInterval = setInterval(() => {
      if (wsService.isConnected()) {
        wsService.sendMessage(MSG_TYPES.GET_ONLINE_PLAYERS, {});
      }
    }, 10000);

    // Cleanup
    return () => {
      wsService.offMessage(
        MSG_TYPES.ONLINE_PLAYERS_LIST,
        handleOnlinePlayersList
      );
      wsService.offMessage(
        MSG_TYPES.CHALLENGE_DECLINED,
        handleChallengeDeclined
      );
      wsService.offMessage(
        MSG_TYPES.CHALLENGE_CANCELLED,
        handleChallengeCancelled
      );
      wsService.offMessage(MSG_TYPES.CHALLENGE_EXPIRED, handleChallengeExpired);
      clearInterval(refreshInterval);
    };
  }, []);

  // Hàm challenge player (có thể implement sau)
  const handleChallenge = (player) => {
    console.log("[Challenge] Challenging:", player);

    try {
      wsService.sendMessage(MSG_TYPES.CHALLENGE_PLAYER, {
        target_id: player.username,
        game_mode: "casual",
        time_control: 10,
      });

      toast.info(`⚔️ Challenge sent to ${player.username}!`, {
        autoClose: 3000,
      });

      // Track sent challenge (will be updated when we receive challenge_id from server)
      // For now, just mark as challenged
    } catch (error) {
      console.error("[Challenge] Failed:", error);
      toast.error("Failed to send challenge", {
        autoClose: 2000,
      });
    }
  };

  // Hàm refresh manual
  const handleRefresh = () => {
    setLoading(true);
    wsService.sendMessage(MSG_TYPES.GET_ONLINE_PLAYERS, {});
  };

  if (loading) {
    return (
      <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
        <h2 className="text-xl font-bold text-[#00d9ff] mb-4">
          Online Players
        </h2>
        <div className="flex items-center justify-center py-8">
          <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-[#00d9ff]"></div>
          <span className="ml-3 text-gray-400">Loading players...</span>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
        <h2 className="text-xl font-bold text-[#00d9ff] mb-4">
          Online Players
        </h2>
        <div className="text-center py-8">
          <p className="text-red-400 mb-2">❌ {error}</p>
          <button
            onClick={() => {
              setLoading(true);
              setError(null);
              wsService.sendMessage(MSG_TYPES.GET_ONLINE_PLAYERS, {});
            }}
            className="px-4 py-2 bg-[#00d9ff] bg-opacity-20 text-[#00d9ff] rounded hover:bg-opacity-40 transition"
          >
            Retry
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
      <div className="flex items-center justify-between mb-4">
        <h2 className="text-xl font-bold text-[#00d9ff]">
          Online Players ({players.length})
        </h2>
        <button
          onClick={handleRefresh}
          className="text-sm text-gray-400 hover:text-[#00d9ff] transition"
          title="Refresh"
        >
          🔄
        </button>
      </div>

      {players.length === 0 ? (
        <div className="text-center py-8 text-gray-400">No players online</div>
      ) : (
        <div className="space-y-3 max-h-96 overflow-y-auto">
          {players.map((player, index) => (
            <div
              key={index}
              className="flex items-center justify-between p-3 bg-[#0f1419] rounded hover:bg-[#1a2a40] transition"
            >
              <div className="flex items-center gap-3">
                <div className="w-3 h-3 rounded-full bg-[#00d084]"></div>
                <div>
                  <p className="font-semibold text-[#e8f0ff]">
                    {player.username}
                  </p>
                  <p className="text-xs text-gray-500">
                    {player.eloRating} ELO • {player.rank}
                  </p>
                </div>
              </div>
              <button
                onClick={() => handleChallenge(player)}
                className="px-3 py-1 text-xs bg-[#00d9ff] bg-opacity-20 text-[#00d9ff] rounded hover:bg-opacity-40 transition disabled:opacity-50 disabled:cursor-not-allowed"
                disabled={challengesSent.has(player.user_id)}
              >
                {challengesSent.has(player.user_id)
                  ? "⏳ Sent"
                  : "⚔️ Challenge"}
              </button>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
