import {
  createContext,
  useState,
  useEffect,
  useCallback,
} from "react";
import { wsService, MSG_TYPES } from "@/services/wsService";
import { useAuth } from "@/hooks/useAuth";

// eslint-disable-next-line react-refresh/only-export-components
export const GameContext = createContext(null);

export function GameProvider({ children }) {
  const { user } = useAuth();
  const [gameState, setGameState] = useState(null);
  const [isConnected, setIsConnected] = useState(false);
  const [currentGameId, setCurrentGameId] = useState(null);

  useEffect(() => {
    // Monitor WebSocket connection status
    const handleConnectionChange = (state) => {
      setIsConnected(state === "connected");
    };

    wsService.onConnectionStateChange(handleConnectionChange);
    setIsConnected(wsService.isConnected());

    return () => {
      // Cleanup if needed
    };
  }, []);

  // ==================== Listen for game messages ====================
  useEffect(() => {
    const handleStartGame = (payload) => {
      console.log("[Game] Game started:", payload);
      setCurrentGameId(payload.gameId);
      setGameState({
        gameId: payload.gameId,
        opponent: payload.opponent,
        yourBoard: Array(100).fill("water"),
        opponentBoard: Array(100).fill("hidden"),
        phase: "placing_ships",
        currentTurn: payload.currentTurn || user?.username,
        ships: [],
        messages: [],
      });
    };

    const handleMoveResult = (payload) => {
      console.log("[Game] Move result:", payload);
      setGameState((prev) => {
        if (!prev) return null;

        const updatedOpponentBoard = [...prev.opponentBoard];
        const index = payload.y * 10 + payload.x;

        if (payload.hit) {
          updatedOpponentBoard[index] = "hit";
        } else {
          updatedOpponentBoard[index] = "miss";
        }

        return {
          ...prev,
          opponentBoard: updatedOpponentBoard,
          currentTurn: payload.nextTurn,
        };
      });
    };

    const handleGameOver = (payload) => {
      console.log("[Game] Game over:", payload);
      setGameState((prev) => {
        if (!prev) return null;
        return {
          ...prev,
          phase: "finished",
          winner: payload.winner,
        };
      });
    };

    wsService.onMessage(MSG_TYPES.START_GAME, handleStartGame);
    wsService.onMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);
    wsService.onMessage(MSG_TYPES.GAME_OVER, handleGameOver);

    return () => {
      wsService.offMessage(MSG_TYPES.START_GAME, handleStartGame);
      wsService.offMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);
      wsService.offMessage(MSG_TYPES.GAME_OVER, handleGameOver);
    };
  }, [user]);

  // ==================== Game actions ====================
  const makeMove = useCallback(
    (gameId, row, col) => {
      if (!isConnected) {
        console.error("[Game] Cannot make move: not connected");
        return;
      }

      if (!gameState || gameState.currentTurn !== user?.username) {
        console.warn("[Game] Not your turn");
        return;
      }

      try {
        wsService.sendMessage(MSG_TYPES.PLAYER_MOVE, {
          x: col,
          y: row,
        });
        console.log(`[Game] Move sent: (${row}, ${col})`);
      } catch (error) {
        console.error("[Game] Failed to send move:", error);
      }
    },
    [isConnected, gameState, user]
  );

  const placeShip = useCallback((ship) => {
    setGameState((prev) => {
      if (!prev) return null;

      const updatedShips = [...prev.ships, ship];
      const updatedBoard = [...prev.yourBoard];

      ship.coordinates.forEach(({ row, col }) => {
        const index = row * 10 + col;
        updatedBoard[index] = "ship";
      });

      return {
        ...prev,
        ships: updatedShips,
        yourBoard: updatedBoard,
      };
    });
  }, []);

  const startGame = useCallback(() => {
    if (!isConnected) {
      console.error("[Game] Cannot start game: not connected");
      return;
    }

    setGameState((prev) => {
      if (!prev) return null;
      return {
        ...prev,
        phase: "playing",
      };
    });
  }, [isConnected]);

  const sendMessage = useCallback(
    (gameId, message) => {
      if (!isConnected) {
        console.error("[Game] Cannot send message: not connected");
        return;
      }

      try {
        wsService.sendMessage(MSG_TYPES.CHAT, {
          message,
        });

        setGameState((prev) => {
          if (!prev) return null;
          return {
            ...prev,
            messages: [
              ...prev.messages,
              {
                sender: user?.username,
                text: message,
                timestamp: Date.now(),
              },
            ],
          };
        });
      } catch (error) {
        console.error("[Game] Failed to send chat message:", error);
      }
    },
    [isConnected, user]
  );

  const value = {
    gameState,
    isConnected,
    currentGameId,
    makeMove,
    placeShip,
    startGame,
    sendMessage,
  };

  return <GameContext.Provider value={value}>{children}</GameContext.Provider>;
}
