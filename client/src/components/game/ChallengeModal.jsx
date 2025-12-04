import { useState, useEffect, useCallback } from "react";
import { wsService, MSG_TYPES } from "@/services/wsService";
import { toast } from "react-toastify";

export default function ChallengeModal() {
  const [challenge, setChallenge] = useState(null);
  const [countdown, setCountdown] = useState(60);

  useEffect(() => {
    // ✅ Handle incoming challenges
    const handleChallengeReceived = (payload) => {
      console.log("[ChallengeModal] Received:", payload);

      setChallenge({
        challengeId: payload.challenge_id,
        challenger: payload.challenger_username,
        challengerId: payload.challenger_id,
        gameMode: payload.game_mode,
        timeControl: payload.time_control,
        expiresAt: payload.expires_at,
      });

      playNotificationSound();

      toast.info(`⚔️ ${payload.challenger_username} challenges you!`, {
        autoClose: 5000,
      });
    };

    // ✅ Handle challenge expired
    const handleChallengeExpired = (payload) => {
      console.log("[ChallengeModal] Expired:", payload);

      if (challenge && challenge.challengeId === payload.challenge_id) {
        toast.warning("⏰ Challenge expired", {
          autoClose: 3000,
        });
        setChallenge(null);
      }
    };

    // ✅ Handle challenge cancelled by opponent
    const handleChallengeCancelled = (payload) => {
      console.log("[ChallengeModal] Cancelled:", payload);

      if (challenge && challenge.challengeId === payload.challenge_id) {
        toast.info("Challenge was cancelled", {
          autoClose: 3000,
        });
        setChallenge(null);
      }
    };

    wsService.onMessage(MSG_TYPES.CHALLENGE_RECEIVED, handleChallengeReceived);
    wsService.onMessage(MSG_TYPES.CHALLENGE_EXPIRED, handleChallengeExpired);
    wsService.onMessage(
      MSG_TYPES.CHALLENGE_CANCELLED,
      handleChallengeCancelled
    );

    return () => {
      wsService.offMessage(
        MSG_TYPES.CHALLENGE_RECEIVED,
        handleChallengeReceived
      );
      wsService.offMessage(MSG_TYPES.CHALLENGE_EXPIRED, handleChallengeExpired);
      wsService.offMessage(
        MSG_TYPES.CHALLENGE_CANCELLED,
        handleChallengeCancelled
      );
    };
  }, [challenge]);

  const handleExpire = useCallback(() => {
    toast.warning("Challenge expired", {
      autoClose: 2000,
    });
    setChallenge(null);
  }, []);

  // ✅ Countdown timer
  useEffect(() => {
    if (!challenge) return;

    const timer = setInterval(() => {
      const now = Math.floor(Date.now() / 1000);
      const remaining = challenge.expiresAt - now;

      if (remaining <= 0) {
        handleExpire();
      } else {
        setCountdown(remaining);
      }
    }, 1000);

    return () => clearInterval(timer);
  }, [challenge, handleExpire]);

  const handleAccept = () => {
    if (!challenge) return;

    try {
      wsService.sendMessage(MSG_TYPES.CHALLENGE_ACCEPT, {
        challenge_id: challenge.challengeId,
      });

      toast.success(`✅ Challenge accepted! Starting game...`, {
        autoClose: 2000,
      });

      setChallenge(null);
    } catch (error) {
      console.error("[ChallengeModal] Accept failed:", error);
      toast.error("Failed to accept challenge", {
        autoClose: 2000,
      });
    }
  };

  const handleDecline = () => {
    if (!challenge) return;

    try {
      wsService.sendMessage(MSG_TYPES.CHALLENGE_DECLINE, {
        challenge_id: challenge.challengeId,
      });

      toast.info("Challenge declined", {
        autoClose: 2000,
      });

      setChallenge(null);
    } catch (error) {
      console.error("[ChallengeModal] Decline failed:", error);
    }
  };

  const playNotificationSound = () => {
    // TODO: Add audio notification
    try {
      const audio = new Audio("/sounds/challenge.mp3");
      audio.play();
    } catch (e) {
      console.warn("Could not play sound:", e);
    }
  };

  if (!challenge) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm animate-fadeIn">
      <div className="bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900 border-2 border-cyan-400/50 rounded-xl p-8 max-w-md w-full mx-4 shadow-2xl shadow-cyan-400/20 animate-scaleIn">
        {/* Header */}
        <div className="text-center mb-6">
          <div className="text-6xl mb-3 animate-bounce">⚔️</div>
          <h2 className="text-3xl font-bold text-cyan-400 mb-2">CHALLENGE!</h2>
          <p className="text-xl text-gray-300">
            <span className="text-cyan-400 font-semibold">
              {challenge.challenger}
            </span>{" "}
            challenges you to a battle!
          </p>
        </div>

        {/* Details */}
        <div className="bg-slate-800/50 rounded-lg p-4 mb-6 space-y-2">
          <div className="flex justify-between text-sm">
            <span className="text-gray-400">Game Mode:</span>
            <span className="text-cyan-400 font-semibold uppercase">
              {challenge.gameMode}
            </span>
          </div>
          <div className="flex justify-between text-sm">
            <span className="text-gray-400">Time Control:</span>
            <span className="text-cyan-400 font-semibold">
              {challenge.timeControl} minutes
            </span>
          </div>
          <div className="flex justify-between text-sm">
            <span className="text-gray-400">Expires in:</span>
            <span
              className={`font-mono font-bold ${
                countdown <= 10
                  ? "text-red-400 animate-pulse"
                  : "text-yellow-400"
              }`}
            >
              {countdown}s
            </span>
          </div>
        </div>

        {/* Progress bar */}
        <div className="w-full bg-slate-700 rounded-full h-2 mb-6 overflow-hidden">
          <div
            className="bg-gradient-to-r from-cyan-400 to-blue-500 h-full transition-all duration-1000"
            style={{ width: `${(countdown / 60) * 100}%` }}
          ></div>
        </div>

        {/* Buttons */}
        <div className="flex gap-4">
          <button
            onClick={handleAccept}
            className="flex-1 py-3 bg-gradient-to-r from-green-500 to-green-600 text-white font-bold rounded-lg hover:from-green-600 hover:to-green-700 transition transform hover:scale-105 shadow-lg shadow-green-500/30"
          >
            ✅ ACCEPT
          </button>
          <button
            onClick={handleDecline}
            className="flex-1 py-3 bg-gradient-to-r from-red-500 to-red-600 text-white font-bold rounded-lg hover:from-red-600 hover:to-red-700 transition transform hover:scale-105 shadow-lg shadow-red-500/30"
          >
            ❌ DECLINE
          </button>
        </div>
      </div>
    </div>
  );
}
