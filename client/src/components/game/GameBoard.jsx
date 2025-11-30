import { useState } from "react";
import {
  CarrierShip,
  Battleship,
  Destroyer,
  Submarine,
  PatrolBoat,
} from "./Ships";

export default function GameBoard({ gameState, onMove, onPlaceShip }) {
  const { yourBoard, opponentBoard, phase, currentTurn, ships } = gameState;
  const GRID_SIZE = 10;
  const [hoveredCell, setHoveredCell] = useState(null);
  const [dragOverCell, setDragOverCell] = useState(null);

  // ✅ Map ship size to component
  const shipComponents = {
    5: CarrierShip,
    4: Battleship,
    3: Destroyer,
    2: Submarine,
    1: PatrolBoat,
  };

  // ✅ Hàm kiểm tra ô có phải đầu thuyền không
  const isShipStart = (index, isYourBoard) => {
    if (!isYourBoard || !ships) return null;

    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;

    return ships.find((ship) => ship.startRow === row && ship.startCol === col);
  };

  // ✅ Drag & Drop handlers
  const handleDragOver = (e, index) => {
    e.preventDefault();
    setDragOverCell(index);
  };

  const handleDragLeave = () => {
    setDragOverCell(null);
  };

  const handleCellDrop = (e, index) => {
    e.preventDefault();
    setDragOverCell(null);

    const shipData = e.dataTransfer.getData("ship");
    if (!shipData) return;

    const ship = JSON.parse(shipData);
    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;

    if (onPlaceShip) {
      onPlaceShip(ship, row, col);
    }
  };

  // ✅ Render cell với visual improvements
  const renderCell = (value, index, isYourBoard) => {
    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;
    const isHovered = hoveredCell === index;
    const isDragOver = dragOverCell === index;

    // ✅ Kiểm tra xem ô này có phải đầu thuyền không
    const shipAtCell = isShipStart(index, isYourBoard);

    // Nội dung hiển thị
    let cellContent = null;
    let cellColorClass = "";
    let cellBorderClass = "border-slate-700/50";

    if (value === "hit") {
      cellContent = (
        <div className="relative w-full h-full">
          <div className="absolute inset-0 bg-red-900/30 animate-pulse" />
          <span className="relative z-10 text-2xl animate-bounce">💥</span>
        </div>
      );
      cellColorClass = "bg-red-900/50";
      cellBorderClass = "border-red-500/50";
    } else if (value === "miss") {
      cellContent = (
        <div className="relative w-full h-full">
          <div className="absolute inset-0 bg-blue-900/20" />
          <span className="relative z-10 text-xl opacity-60">💦</span>
        </div>
      );
      cellColorClass = "bg-blue-900/30";
      cellBorderClass = "border-blue-500/30";
    } else if (isYourBoard && typeof value === "number" && value > 0) {
      // ✅ Hiển thị ship visual nếu là đầu thuyền
      if (shipAtCell) {
        const ShipComponent = shipComponents[shipAtCell.size];
        if (ShipComponent) {
          cellContent = (
            <div
              className="absolute inset-0 pointer-events-none"
              style={{
                width: `${shipAtCell.size * 100}%`,
                left: 0,
                top: 0,
                zIndex: 10,
              }}
            >
              <ShipComponent
                style={{
                  width: "100%",
                  height: "auto",
                  filter: "drop-shadow(0 0 8px rgba(0, 217, 255, 0.6))",
                }}
              />
            </div>
          );
        }
      } else {
        // Các ô còn lại của thuyền (không hiển thị gì, chỉ tô màu)
        cellContent = null;
      }
      cellColorClass = "bg-slate-800/50";
      cellBorderClass = "border-cyan-400/30";
    } else if (!isYourBoard && value === "hidden") {
      cellColorClass = "bg-slate-900/50";
    }

    // ✅ Hover effects
    if (
      isHovered &&
      !isYourBoard &&
      phase === "playing" &&
      currentTurn === "you"
    ) {
      cellColorClass += " ring-2 ring-red-500/50";
    }

    // ✅ Drag over effects
    if (isDragOver && isYourBoard && phase === "placing_ships") {
      cellColorClass += " ring-2 ring-cyan-400 bg-cyan-900/30";
    }

    const className = `
            relative w-full h-full border ${cellBorderClass} 
            flex items-center justify-center 
            transition-all duration-200
            ${cellColorClass}
            ${
              !isYourBoard && phase === "playing" && currentTurn === "you"
                ? "cursor-crosshair hover:bg-red-900/20"
                : ""
            }
            ${
              isYourBoard && phase === "placing_ships"
                ? "hover:bg-cyan-900/20"
                : ""
            }
        `;

    const dropProps =
      isYourBoard && phase === "placing_ships"
        ? {
            onDrop: (e) => handleCellDrop(e, index),
            onDragOver: (e) => handleDragOver(e, index),
            onDragLeave: handleDragLeave,
          }
        : {};

    return (
      <div
        key={index}
        className={className}
        onMouseEnter={() => setHoveredCell(index)}
        onMouseLeave={() => setHoveredCell(null)}
        onClick={() => {
          if (!isYourBoard && phase === "playing" && currentTurn === "you") {
            onMove(row, col);
          }
        }}
        {...dropProps}
      >
        {cellContent}
      </div>
    );
  };

  // ✅ Render coordinate labels
  const renderColumnLabels = () => (
    <div className="grid grid-cols-10 gap-1 mb-2">
      {[0, 1, 2, 3, 4, 5, 6, 7, 8, 9].map((col) => (
        <div
          key={col}
          className="text-center text-cyan-400/60 text-xs font-mono"
        >
          {col}
        </div>
      ))}
    </div>
  );

  const renderRowLabels = () => (
    <div className="flex flex-col gap-1 mr-2">
      {["A", "B", "C", "D", "E", "F", "G", "H", "I", "J"].map((row) => (
        <div
          key={row}
          className="flex items-center justify-center h-full text-cyan-400/60 text-xs font-mono"
        >
          {row}
        </div>
      ))}
    </div>
  );

  return (
    <div className="bg-slate-900/30 backdrop-blur-sm border border-cyan-400/30 rounded-lg p-6 shadow-[0_0_30px_rgba(0,217,255,0.1)]">
      <h2 className="text-2xl font-bold text-cyan-400 mb-6 text-center tracking-wider uppercase">
        {phase === "placing_ships" && "📍 Deploy Your Fleet"}
        {phase === "waiting_for_opponent" && "⏳ Waiting for Opponent..."}
        {phase === "playing" && "⚔️ Battle in Progress"}
      </h2>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        {/* Your board */}
        <div className="relative">
          <h3 className="text-lg font-semibold text-cyan-400 mb-3 flex items-center gap-2">
            <span className="w-3 h-3 bg-cyan-400 rounded-full animate-pulse" />
            Your Fleet
          </h3>

          <div className="flex">
            {renderRowLabels()}
            <div className="flex-1">
              {renderColumnLabels()}
              <div className="grid grid-cols-10 gap-1 aspect-square bg-slate-950/50 p-2 rounded border border-cyan-400/20">
                {yourBoard.map((cell, index) => renderCell(cell, index, true))}
              </div>
            </div>
          </div>

          {phase === "placing_ships" && (
            <p className="mt-3 text-sm text-cyan-400/70 text-center font-mono">
              Drag ships from dock to place them
            </p>
          )}
        </div>

        {/* Opponent board */}
        <div className="relative">
          <h3 className="text-lg font-semibold text-red-400 mb-3 flex items-center gap-2">
            <span className="w-3 h-3 bg-red-400 rounded-full animate-pulse" />
            Enemy Waters
          </h3>

          <div className="flex">
            {renderRowLabels()}
            <div className="flex-1">
              {renderColumnLabels()}
              <div className="grid grid-cols-10 gap-1 aspect-square bg-slate-950/50 p-2 rounded border border-red-400/20">
                {opponentBoard.map((cell, index) =>
                  renderCell(cell, index, false)
                )}
              </div>
            </div>
          </div>

          {phase === "playing" && currentTurn === "you" && (
            <p className="mt-3 text-sm text-red-400/70 text-center font-mono animate-pulse">
              Click to fire! 🎯
            </p>
          )}
        </div>
      </div>

      {/* Turn indicator */}
      <div className="mt-6 text-center">
        {phase === "playing" && (
          <div
            className={`
                        inline-flex items-center gap-3 px-6 py-3 rounded-lg
                        ${
                          currentTurn === "you"
                            ? "bg-green-900/30 border border-green-400/50 text-green-400"
                            : "bg-orange-900/30 border border-orange-400/50 text-orange-400"
                        }
                    `}
          >
            <span className="text-2xl">
              {currentTurn === "you" ? "🎯" : "⏳"}
            </span>
            <span className="text-lg font-bold tracking-wider uppercase">
              {currentTurn === "you" ? "Your Turn" : "Opponent's Turn"}
            </span>
          </div>
        )}
      </div>
    </div>
  );
}
