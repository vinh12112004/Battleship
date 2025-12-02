"use client";

import { useState, useEffect, useCallback } from "react";
import { useParams, useNavigate } from "react-router-dom";
import NavBar from "../components/common/Navbar";
import GameBoard from "../components/game/GameBoard";
import GameInfoPanel from "../components/game/GameInfoPanel";
import GameChat from "../components/game/GameChat";
import { useGame } from "@/hooks/useGame";
import BattleshipSeaBackground from "../components/game/BattleshipSeaBackground.jsx";
import ShipDock from "../components/game/ShipDock.jsx";
import { wsService, MSG_TYPES } from "@/services/wsService";
import { toast } from "react-toastify";

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
    const navigate = useNavigate();
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

            // Parse JSON từ localStorage
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
                console.error(
                    "[GamePage] ❌ Failed to parse auth_user:",
                    error
                );
                return;
            }

            console.log("[GamePage] 🔍 My username:", myUsername);

            // Compare với current_turn từ server
            const currentTurnUsername = payload.current_turn;

            // So sánh
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
                // Nếu mình đang chờ (waiting) hoặc đang chơi (playing)
                // mà gameState bên ngoài lại bảo là "placing_ships" (do nó chưa update kịp)
                // thì BỎ QUA, không cho phép ghi đè lùi.
                if (
                    (prev.phase === "waiting_for_opponent" ||
                        prev.phase === "playing") &&
                    gameState.phase === "placing_ships"
                ) {
                    console.warn(
                        "[GamePage] Prevented state regression from hook"
                    );
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
                            prev.yourBoard.length > 0
                                ? prev.yourBoard
                                : gameState.yourBoard,
                        ships:
                            prev.ships.length > 0
                                ? prev.ships
                                : gameState.ships,
                        phase: "playing", // Ép cứng lại để chắc chắn
                    };
                }

                return { ...prev, ...gameState };
            });
        }
    }, [gameState]);

    // Listen for MOVE_RESULT
    const handleMoveResult = useCallback((payload) => {
        console.log("[GamePage] 🎯 MOVE_RESULT received:", payload);

        const handleGameOverNavigation = () => {
            navigate("/dashboard");
        };

        // XỬ LÝ TOAST (SIDE EFFECT) RA NGOÀI STATE UPDATE
        if (payload.is_your_shot) {
            if (payload.is_hit) {
                if (payload.is_sunk) {
                    const shipNames = {
                        1: "Patrol Boat",
                        2: "Submarine",
                        3: "Destroyer",
                        4: "Battleship",
                        5: "Carrier",
                    };
                    const shipName =
                        shipNames[payload.sunk_ship_type] || "Unknown Ship";

                    toast.success(`🚢 ${shipName.toUpperCase()} DESTROYED!`, {
                        icon: "💥",
                        style: {
                            background: "#1e293b",
                            color: "#22d3ee",
                            border: "2px solid #22d3ee",
                            fontSize: "16px",
                            fontWeight: "bold",
                        },
                    });
                } else {
                    toast.info("🎯 Direct Hit!", {
                        autoClose: 1000,
                        style: { background: "#1e293b", color: "#fbbf24" },
                    });
                }
            } else {
                toast.warning("💦 Missed!", {
                    autoClose: 1000,
                    style: { background: "#1e293b", color: "#94a3b8" },
                });
            }

            if (payload.game_over) {
                toast.success("🏆 VICTORY! All enemy ships destroyed!", {
                    autoClose: false,
                    style: {
                        background: "#065f46",
                        color: "#d1fae5",
                        fontSize: "18px",
                    },
                });
                handleGameOverNavigation();
            }
        } else {
            // Opponent shot logic
            if (payload.is_hit) {
                if (payload.is_sunk) {
                    const shipNames = {
                        1: "Patrol Boat",
                        2: "Submarine",
                        3: "Destroyer",
                        4: "Battleship",
                        5: "Carrier",
                    };
                    const shipName =
                        shipNames[payload.sunk_ship_type] || "Unknown Ship";

                    toast.error(
                        `💀 YOUR ${shipName.toUpperCase()} WAS DESTROYED!`,
                        {
                            icon: "🔥",
                            style: {
                                background: "#7f1d1d",
                                color: "#fecaca",
                                border: "2px solid #dc2626",
                                fontSize: "16px",
                                fontWeight: "bold",
                            },
                        }
                    );
                } else {
                    toast.error("💥 Your ship was hit!", {
                        autoClose: 1500,
                        style: { background: "#7f1d1d", color: "#fecaca" },
                    });
                }
            } else {
                toast.info("💦 Opponent missed!", {
                    autoClose: 1000,
                    style: { background: "#1e293b", color: "#94a3b8" },
                });
            }

            if (payload.game_over) {
                toast.error("💀 DEFEAT! All your ships were destroyed!", {
                    autoClose: false,
                    style: {
                        background: "#7f1d1d",
                        color: "#fecaca",
                        fontSize: "18px",
                    },
                });
                handleGameOverNavigation();
            }
        }

        // CẬP NHẬT STATE (PURE FUNCTION)
        setLocalGameState((prev) => {
            const index = payload.row * GRID_SIZE + payload.col;

            if (payload.is_your_shot) {
                const newOpponentBoard = [...prev.opponentBoard];

                // Update logic
                if (payload.is_hit) {
                    newOpponentBoard[index] = "hit";
                } else {
                    newOpponentBoard[index] = "miss";
                }

                if (payload.game_over) {
                    return {
                        ...prev,
                        opponentBoard: newOpponentBoard,
                        phase: "finished",
                        currentTurn: null,
                    };
                }

                return {
                    ...prev,
                    opponentBoard: newOpponentBoard,
                    currentTurn: "opponent",
                };
            } else {
                // Opponent shot logic
                const newYourBoard = [...prev.yourBoard];

                if (payload.is_hit) {
                    newYourBoard[index] = "hit";
                } else {
                    newYourBoard[index] = "miss";
                }

                if (payload.game_over) {
                    return {
                        ...prev,
                        yourBoard: newYourBoard,
                        phase: "finished",
                        currentTurn: null,
                    };
                }

                return {
                    ...prev,
                    yourBoard: newYourBoard,
                    currentTurn: "you",
                };
            }
        });
    }, []);

    useEffect(() => {
        wsService.onMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);

        return () => {
            wsService.offMessage(MSG_TYPES.MOVE_RESULT, handleMoveResult);
        };
    }, [handleMoveResult]); // Depend on memoized handler

    // Hàm kiểm tra xem vị trí có hợp lệ để đặt thuyền không
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
            const shipIndex = SHIP_DEFINITIONS.findIndex(
                (s) => s.id === ship.id
            );
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
                    finalBoard[s.startRow * GRID_SIZE + s.startCol + i] =
                        s.size;
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
                        finalBoard[s.startRow * GRID_SIZE + s.startCol + i] =
                            s.size;
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
                                onPlaceShip={handlePlaceShip}
                                GRID_SIZE={GRID_SIZE}
                                draggedShip={draggingShip}
                            />

                            {/* Hiển thị ShipDock chỉ trong giai đoạn đặt thuyền */}
                            <div style={{ minHeight: "150px" }}>
                                {localGameState.phase === "placing_ships" && (
                                    <ShipDock
                                        placedShips={placedShipIds}
                                        onShipDragStart={handleShipDragStart}
                                    />
                                )}
                            </div>

                            {/* 4. Logic hiển thị nút READY và trạng thái đặt thuyền */}
                            <div className="mt-4 text-center">
                                {localGameState.phase === "placing_ships" &&
                                    isReadyToStart && (
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
                                            {isConnected
                                                ? "BATTLE READY"
                                                : "CONNECTING..."}
                                        </button>
                                    )}
                                {/* Hiển thị tiến trình đặt thuyền */}
                                {localGameState.phase === "placing_ships" &&
                                    !isReadyToStart && (
                                        <p className="text-lg text-cyan-400/80 font-mono mt-4">
                                            Đang đặt thuyền:{" "}
                                            {placedShipIds.length}/
                                            {REQUIRED_SHIP_COUNT} tàu đã được
                                            đặt.
                                        </p>
                                    )}
                                {/* Thông báo chờ đối thủ sau khi nhấn Ready */}
                                {localGameState.phase ===
                                    "waiting_for_opponent" && (
                                    <p className="text-lg text-yellow-400 font-mono mt-4 animate-pulse">
                                        Đã sẵn sàng. Đang chờ đối thủ...
                                    </p>
                                )}
                            </div>
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
