"use client";

import { useState, useEffect, useCallback } from "react";
import { useParams } from "react-router-dom";
import NavBar from "../components/common/Navbar";
import GameBoard from "../components/game/GameBoard";
import GameInfoPanel from "../components/game/GameInfoPanel";
import GameChat from "../components/game/GameChat";
import { useGame } from "@/hooks/useGame";
import BattleshipSeaBackground from "../components/game/BattleshipSeaBackground.jsx";
import ShipDock from "../components/game/ShipDock.jsx";
import { wsService, MSG_TYPES } from "@/services/wsService";
const SHIP_DEFINITIONS = [
  { id: "carrier", size: 5 },
  { id: "battleship", size: 4 },
  { id: "destroyer", size: 3 },
  { id: "submarine", size: 2 },
  { id: "patrol", size: 1 },
];
const GRID_SIZE = 10;
// Định nghĩa số lượng thuyền cần đặt
const REQUIRED_SHIP_COUNT = SHIP_DEFINITIONS.length;

export default function GamePage() {
  const { id } = useParams();
  // Giả định `useGame` có các hàm cần thiết (makeMove, sendMessage)
  const { gameState, makeMove, sendMessage, isConnected } = useGame();

  const [localGameState, setLocalGameState] = useState({
    yourBoard: Array(GRID_SIZE * GRID_SIZE).fill(0),
    opponentBoard: Array(GRID_SIZE * GRID_SIZE).fill("hidden"),
    ships: [],
    phase: "placing_ships",
    currentTurn: null, // Thêm state này để quản lý lượt
    opponent: null,
  });
  // Trạng thái cục bộ cho việc kéo thả (đã giữ nguyên)
  const [draggingShip, setDraggingShip] = useState(null);

  useEffect(() => {
    const handleStartGame = (payload) => {
      console.log("[GamePage] 🎮 START_GAME received:", payload);

      // ✅ 1. Parse JSON từ localStorage
      const authUserStr = localStorage.getItem("auth_user");
      if (!authUserStr) {
        console.error("[GamePage] ❌ No auth_user in localStorage!");
        return;
      }

      let myUsername;
      try {
        const authUser = JSON.parse(authUserStr);
        myUsername = authUser.username;
      } catch (error) {
        console.error("[GamePage] ❌ Failed to parse auth_user:", error);
        return;
      }

      console.log("[GamePage] 🔍 My username:", myUsername);

      // ✅ 2. Compare với current_turn từ server
      const currentTurnUsername = payload.current_turn;

      // ✅ 3. So sánh
      const isMyTurn = currentTurnUsername === myUsername;

      console.log("[GamePage] Turn Check:", {
        myUsername,
        currentTurnUsername,
        isMyTurn,
        displayText: isMyTurn ? "YOUR TURN" : "OPPONENT'S TURN",
      });

      setLocalGameState((prev) => ({
        ...prev,
        phase: "playing",
        opponent: payload.opponent,
        game_id: payload.game_id,
        currentTurn: isMyTurn ? "you" : "opponent",
      }));
    };

    wsService.onMessage(MSG_TYPES.START_GAME, handleStartGame);

    return () => {
      wsService.offMessage(MSG_TYPES.START_GAME, handleStartGame);
    };
  }, []);

  useEffect(() => {
    if (gameState) {
      setLocalGameState((prev) => {
        // 🛑 BẢO VỆ TRẠNG THÁI:
        // Nếu mình đang chờ (waiting) hoặc đang chơi (playing)
        // mà gameState bên ngoài lại bảo là "placing_ships" (do nó chưa update kịp)
        // thì BỎ QUA, không cho phép ghi đè lùi.
        if (
          (prev.phase === "waiting_for_opponent" || prev.phase === "playing") &&
          gameState.phase === "placing_ships"
        ) {
          console.warn("[GamePage] Prevented state regression from hook");
          return prev;
        }

        // Nếu phase khớp hoặc tiến lên thì mới merge
        // Chỉ merge những field cần thiết, ưu tiên giữ lại yourBoard và ships của local
        // nếu đang trong phase playing (vì local chứa trạng thái realtime)
        if (prev.phase === "playing") {
          return {
            ...prev,
            ...gameState,
            // Giữ lại board và ships của local để tránh flicker
            yourBoard:
              prev.yourBoard.length > 0 ? prev.yourBoard : gameState.yourBoard,
            ships: prev.ships.length > 0 ? prev.ships : gameState.ships,
            phase: "playing", // Ép cứng lại để chắc chắn
          };
        }

        return { ...prev, ...gameState };
      });
    }
  }, [gameState]);

  // ✅ 3. Listen for MOVE_RESULT
  useEffect(() => {
    const handleMoveResult = (payload) => {
      console.log("[GamePage] 🎯 MOVE_RESULT received:", payload);

      const index = payload.row * GRID_SIZE + payload.col;

      setLocalGameState((prev) => {
        const newOpponentBoard = [...prev.opponentBoard];

        // ✅ Update opponent board với kết quả
        if (payload.is_hit) {
          newOpponentBoard[index] = "hit";
          console.log(`[GamePage] ✅ HIT at (${payload.row}, ${payload.col})`);

          if (payload.is_sunk) {
            console.log(
              `[GamePage] 🚢 SHIP SUNK! Type: ${payload.sunk_ship_type}`
            );
          }
        } else {
          newOpponentBoard[index] = "miss";
          console.log(`[GamePage] ❌ MISS at (${payload.row}, ${payload.col})`);
        }

        // ✅ Check game over
        if (payload.game_over) {
          console.log("[GamePage] 🏆 GAME OVER!");
          alert("🎉 Victory! You sunk all enemy ships!");
          return {
            ...prev,
            opponentBoard: newOpponentBoard,
            phase: "finished",
          };
        }

        // ✅ Switch turn
        const newTurn = prev.currentTurn === "you" ? "opponent" : "you";

        return {
          ...prev,
          opponentBoard: newOpponentBoard,
          currentTurn: newTurn,
        };
      });
    };

    wsService.onMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);

    return () => {
      wsService.offMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);
    };
  }, []);

  // Hàm kiểm tra xem vị trí có hợp lệ để đặt thuyền không (đã giữ nguyên)
  const isValidPlacement = (board, row, col, size, orientation) => {
    if (orientation !== "horizontal") return false;
    if (row < 0 || row >= GRID_SIZE || col < 0 || col + size > GRID_SIZE) {
      return false;
    }
    for (let i = 0; i < size; i++) {
      // Kiểm tra trên board đang được reset về 0 (đơn giản hóa)
      if (board[row * GRID_SIZE + col + i] !== 0) {
        return false;
      }
    }
    return true;
  };

  // Hàm xử lý đặt thuyền (đã giữ nguyên)
  const handlePlaceShip = useCallback((ship, row, col) => {
    setLocalGameState((prev) => {
      const shipIndex = SHIP_DEFINITIONS.findIndex((s) => s.id === ship.id);
      const size = SHIP_DEFINITIONS[shipIndex].size;

      // Loại bỏ thuyền cũ và reset board
      const newShips = prev.ships.filter((s) => s.id !== ship.id);
      let finalBoard = Array(GRID_SIZE * GRID_SIZE).fill(0);

      const newPlacement = {
        id: ship.id,
        size: size,
        startRow: row,
        startCol: col,
        orientation: "horizontal",
      };

      // Tạm thời tạo board chứa các thuyền CŨ để kiểm tra vị trí mới
      newShips.forEach((s) => {
        for (let i = 0; i < s.size; i++) {
          finalBoard[s.startRow * GRID_SIZE + s.startCol + i] = s.size;
        }
      });

      // Kiểm tra vị trí thuyền MỚI trên board đã có thuyền cũ
      if (isValidPlacement(finalBoard, row, col, size, "horizontal")) {
        // Đặt thuyền mới vào danh sách
        newShips.push(newPlacement);

        // Tái tạo lại finalBoard với TẤT CẢ các thuyền đã đặt
        finalBoard.fill(0);
        newShips.forEach((s) => {
          for (let i = 0; i < s.size; i++) {
            finalBoard[s.startRow * GRID_SIZE + s.startCol + i] = s.size;
          }
        });

        return {
          ...prev,
          yourBoard: finalBoard,
          ships: newShips,
        };
      }

      // Nếu không hợp lệ, trả lại board cũ (chưa reset)
      return prev;
    });

    setDraggingShip(null);
  }, []);

  // Hàm được truyền vào ShipDock để lưu thuyền đang kéo
  const handleShipDragStart = (ship) => {
    setDraggingShip(ship);
  };

  // Lấy ID của các thuyền đã đặt để truyền vào ShipDock
  const placedShipIds = localGameState.ships.map((s) => s.id);

  // 2. Logic kiểm tra sẵn sàng chơi
  const isReadyToStart =
    localGameState.phase === "placing_ships" &&
    placedShipIds.length === REQUIRED_SHIP_COUNT;

  // 3. Hàm xử lý khi nhấn nút Ready
  const handleReadyClick = () => {
    if (!isConnected || !isReadyToStart) return;

    wsService.sendMessage(MSG_TYPES.PLAYER_READY, {
      game_id: id,
      board_state: localGameState.yourBoard,
    });
    console.log(
      "Player is ready with board:",
      localGameState.yourBoard,
      "id:",
      id
    );
    setLocalGameState((prev) => ({
      ...prev,
      phase: "waiting_for_opponent",
    }));
  };

  const handleMove = (row, col) => {
    if (!isConnected) {
      console.error("[GamePage] Cannot shoot: WebSocket not connected");
      return;
    }

    if (localGameState.currentTurn !== "you") {
      console.warn("[GamePage] Cannot shoot: Not your turn");
      return;
    }

    console.log(`[GamePage] 🎯 Shooting at (${row}, ${col})`);

    wsService.sendMessage(MSG_TYPES.PLAYER_MOVE, {
      game_id: id,
      row: row,
      col: col,
    });
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
                  isConnected ? "bg-cyan-400 animate-ping" : "bg-red-400"
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
                onPlaceShip={handlePlaceShip}
                GRID_SIZE={GRID_SIZE}
              />

              {/* Hiển thị ShipDock chỉ trong giai đoạn đặt thuyền */}
              {localGameState.phase === "placing_ships" && (
                <ShipDock
                  placedShips={placedShipIds}
                  onShipDragStart={handleShipDragStart}
                />
              )}

              {/* 4. Logic hiển thị nút READY và trạng thái đặt thuyền */}
              <div className="mt-4 text-center">
                {localGameState.phase === "placing_ships" && isReadyToStart && (
                  <button
                    onClick={handleReadyClick}
                    disabled={!isConnected}
                    className="
                                            px-8 py-3 text-xl font-bold rounded-lg
                                            text-white bg-green-600
                                            hover:bg-green-500 transition duration-200
                                            shadow-lg shadow-green-900/50
                                            disabled:bg-gray-600 disabled:cursor-not-allowed
                                            tracking-widest uppercase
                                        "
                  >
                    {isConnected ? "BATTLE READY" : "CONNECTING..."}
                  </button>
                )}
                {/* Hiển thị tiến trình đặt thuyền */}
                {localGameState.phase === "placing_ships" &&
                  !isReadyToStart && (
                    <p className="text-lg text-cyan-400/80 font-mono mt-4">
                      Đang đặt thuyền: {placedShipIds.length}/
                      {REQUIRED_SHIP_COUNT} tàu đã được đặt.
                    </p>
                  )}
                {/* Thông báo chờ đối thủ sau khi nhấn Ready */}
                {localGameState.phase === "waiting_for_opponent" && (
                  <p className="text-lg text-yellow-400 font-mono mt-4 animate-pulse">
                    Đã sẵn sàng. Đang chờ đối thủ...
                  </p>
                )}
              </div>
            </div>

            {/* Right sidebar */}
            <div className="space-y-6">
              <GameInfoPanel gameId={id} />
              {/* <GameChat
                                gameId={id}
                                messages={localGameState?.messages || []}
                                onSendMessage={sendMessage}
                            /> */}
            </div>
          </div>
        </div>

        {/* Footer Section */}
        <div className="px-6 py-3 bg-gradient-to-t from-slate-900/90 via-slate-900/60 to-transparent backdrop-blur-sm border-t border-cyan-400/20 mt-6">
          <div className="max-w-7xl mx-auto">
            <div className="flex flex-col md:flex-row items-center justify-between text-sm gap-2">
              <div className="text-cyan-300/70 font-mono tracking-wider">
                GAME ID: <span className="text-cyan-400">{id}</span>
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
