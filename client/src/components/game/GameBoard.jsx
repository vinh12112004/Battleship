import { useState } from "react";
import {
  CarrierShip,
  Battleship,
  Destroyer,
  Submarine,
  PatrolBoat,
} from "./Ships";

export default function GameBoard({
  gameState,
  onMove,
  onPlaceShip,
  draggedShip,
}) {
  const { yourBoard, opponentBoard, phase, currentTurn, ships } = gameState;
  const GRID_SIZE = 10;

  const [hoveredCell, setHoveredCell] = useState(null);
  const [dragOverCell, setDragOverCell] = useState(null);

  // Map ship size to component
  const shipComponents = {
    5: CarrierShip,
    4: Battleship,
    3: Destroyer,
    2: Submarine,
    1: PatrolBoat,
  };

  // Kiểm tra ô hiện tại có nằm trong vùng preview của tàu đang kéo không
  const isDragPreviewCell = (index) => {
    // Nếu không có ô đang hover hoặc không có thông tin tàu đang kéo -> return false
    if (dragOverCell === null || !draggedShip) return false;

    const dragRow = Math.floor(dragOverCell / GRID_SIZE);
    const dragCol = dragOverCell % GRID_SIZE;
    const cellRow = Math.floor(index / GRID_SIZE);
    const cellCol = index % GRID_SIZE;

    // Logic: Highlight các ô nằm ngang từ vị trí chuột sang phải theo độ dài tàu
    if (
      cellRow === dragRow &&
      cellCol >= dragCol &&
      cellCol < dragCol + draggedShip.size
    ) {
      return true;
    }

    return false;
  };

  // Kiểm tra vị trí đặt tàu có hợp lệ không (để tô màu Xanh/Đỏ)
  const isDragPreviewValid = () => {
    if (dragOverCell === null || !draggedShip) return true;

    const dragRow = Math.floor(dragOverCell / GRID_SIZE);
    const dragCol = dragOverCell % GRID_SIZE;

    // a. Kiểm tra tràn lề phải
    if (dragCol + draggedShip.size > GRID_SIZE) return false;

    // b. Kiểm tra đè lên tàu khác
    for (let i = 0; i < draggedShip.size; i++) {
      const checkIndex = dragRow * GRID_SIZE + dragCol + i;
      // board[index] != 0 nghĩa là ô đó đã có tàu
      if (yourBoard[checkIndex] !== 0) return false;
    }

    return true;
  };

  const isShipStart = (index, isYourBoard) => {
    if (!isYourBoard || !ships) return null;
    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;
    return ships.find((ship) => ship.startRow === row && ship.startCol === col);
  };

  const getShipAtCell = (index, isYourBoard) => {
    if (!isYourBoard || !ships) return null;
    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;
    return ships.find((ship) => {
      const { startRow, startCol, size, orientation } = ship;
      const isHorizontal = orientation === "horizontal";
      for (let i = 0; i < size; i++) {
        const shipRow = isHorizontal ? startRow : startRow + i;
        const shipCol = isHorizontal ? startCol + i : startCol;
        if (shipRow === row && shipCol === col) return true;
      }
      return false;
    });
  };

  const handleDragOver = (e, index) => {
    e.preventDefault(); // Bắt buộc để cho phép Drop
    // Chỉ update state nếu vị trí thay đổi để tránh re-render quá nhiều
    if (dragOverCell !== index) {
      setDragOverCell(index);
    }
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

  const renderCell = (value, index, isYourBoard) => {
    const row = Math.floor(index / GRID_SIZE);
    const col = index % GRID_SIZE;
    const isHovered = hoveredCell === index;

    const shipAtCell = isShipStart(index, isYourBoard);
    const belongsToShip = getShipAtCell(index, isYourBoard);

    // Tính toán trạng thái Preview
    const isPreviewCell = isDragPreviewCell(index);
    const isValidPreview = isDragPreviewValid();

    let cellContent = null;
    let cellColorClass = "";
    let cellBorderClass = "border-slate-700/50";

    // DEFAULT CELL BACKGROUND
    if (isYourBoard) {
      cellColorClass = "bg-slate-900/50";
    } else {
      cellColorClass = "bg-slate-900/50";
    }

    // 2. Hiển thị nền cho các ô đã có tàu (chỉ tăng nhẹ độ sáng)
    if (isYourBoard && belongsToShip && value !== "hit") {
      cellColorClass = "bg-cyan-900/30"; 
      cellBorderClass = "border-cyan-400/60";
    }

    // 3. Hiển thị hình ảnh Tàu (chỉ render ở ô đầu tiên)
    if (isYourBoard && shipAtCell) {
      const ShipComponent = shipComponents[shipAtCell.size];
      if (ShipComponent) {
        cellContent = (
          <div
            className="absolute inset-0 pointer-events-none"
            style={{
              width: `${shipAtCell.size * 100}%`,
              left: 0,
              top: 0,
              zIndex: 5,
            }}
          >
            <ShipComponent
              style={{
                width: "100%",
                height: "auto",
                filter: "drop-shadow(0 0 10px rgba(0, 217, 255, 0.8))", // Tăng glow
                opacity: 1, // Đảm bảo ship không bị mờ
              }}
            />
          </div>
        );
      }
    }

    // 4. Hiển thị Trúng (Hit) hoặc Trượt (Miss)
    if (value === "hit") {
      cellContent = (
        <>
          {cellContent && <div style={{ opacity: 0.6 }}>{cellContent}</div>}
          <div
            className="absolute inset-0 flex items-center justify-center"
            style={{ zIndex: 10 }}
          >
            <div className="absolute inset-0 bg-red-900/50 animate-pulse backdrop-blur-[1px]" />
            <span className="relative z-10 text-3xl animate-bounce">💥</span>
          </div>
        </>
      );
      cellColorClass = "bg-red-900/40";
      cellBorderClass = "border-red-500/80";
    } else if (value === "miss") {
      cellContent = (
        <div className="relative w-full h-full">
          <div className="absolute inset-0 bg-blue-900/30" />{" "}
          <span className="relative z-10 text-xl opacity-80">💦</span>{" "}
        </div>
      );
      cellColorClass = "bg-blue-900/40";
      cellBorderClass = "border-blue-500/40";
    } else if (!isYourBoard && value === "hidden") {
      cellColorClass = "bg-slate-900/50";
    }

    // ✅ 5. HIỂN THỊ PREVIEW KHI KÉO TÀU (QUAN TRỌNG)
    if (isYourBoard && phase === "placing_ships" && isPreviewCell) {
      if (isValidPreview) {
        cellColorClass = "bg-cyan-500/50 ring-2 ring-cyan-400";
        cellBorderClass = "border-cyan-400";
      } else {
        cellColorClass = "bg-red-500/50 ring-2 ring-red-400";
        cellBorderClass = "border-red-400";
      }
    }

    // Hiệu ứng Hover khi đang chơi (bắn địch)
    if (
      isHovered &&
      !isYourBoard &&
      phase === "playing" &&
      currentTurn === "you"
    ) {
      cellColorClass += " ring-2 ring-red-500/50";
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

  const renderColumnLabels = () => (
    <div className="grid grid-cols-10 gap-1 mb-2">
      {[0, 1, 2, 3, 4, 5, 6, 7, 8, 9].map((col) => (
        <div
          key={col}
          className="text-center text-cyan-400/60 text-xs font-mono"
        >
          {" "}
          {col}{" "}
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
          {" "}
          {row}{" "}
        </div>
      ))}
    </div>
  );

  return (
    <div className="space-y-6">
      {/* ✅ Turn Indicator - THÊM Ở ĐẦU */}
      {phase === "playing" && (
        <div className="text-center">
          <div
            className={`inline-flex items-center gap-3 px-6 py-3 rounded-lg backdrop-blur-sm border ${
              currentTurn === "you"
                ? "bg-cyan-900/30 border-cyan-400/50 text-cyan-400"
                : "bg-orange-900/30 border-orange-400/50 text-orange-400"
            }`}
          >
            <div
              className={`w-3 h-3 rounded-full ${
                currentTurn === "you" ? "bg-cyan-400" : "bg-orange-400"
              } animate-pulse shadow-[0_0_10px_currentColor]`}
            />
            <span className="text-lg font-bold tracking-wider uppercase">
              {currentTurn === "you" ? "🎯 YOUR TURN" : "⏳ OPPONENT'S TURN"}
            </span>
          </div>
        </div>
      )}

      {/* Boards Grid */}
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
        </div>
      </div>
    </div>
  );
}