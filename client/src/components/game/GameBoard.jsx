// File: GameBoard.jsx

export default function GameBoard({ gameState, onMove, onPlaceShip }) {
    const { yourBoard, opponentBoard, phase, currentTurn } = gameState;
    const GRID_SIZE = 10;

    // Hàm cần thiết để cho phép sự kiện drop (kéo thả)
    const handleDragOver = (e) => {
        e.preventDefault();
    };

    // Hàm xử lý khi thả thuyền vào ô trên board của bạn
    const handleCellDrop = (e, index) => {
        e.preventDefault();

        // 1. Lấy thông tin thuyền từ dataTransfer
        const shipData = e.dataTransfer.getData("ship");
        if (!shipData) return;

        const ship = JSON.parse(shipData);

        // 2. Tính toán vị trí (Hàng và Cột)
        const row = Math.floor(index / GRID_SIZE);
        const col = index % GRID_SIZE;

        // 3. Gọi hàm xử lý đặt thuyền (được truyền từ GamePage)
        if (onPlaceShip) {
            onPlaceShip(ship, row, col);
        }
    };

    const renderCell = (value, index, isYourBoard) => {
        const row = Math.floor(index / GRID_SIZE);
        const col = index % GRID_SIZE;

        // Nội dung hiển thị trong ô
        let cellContent = "";
        let cellColorClass = "";

        if (value === "hit") {
            cellContent = "💥";
            cellColorClass = "bg-red-900/50";
        } else if (value === "miss") {
            cellContent = "💦";
            cellColorClass = "bg-blue-900/50";
        } else if (value === "ship") {
            // Đây là trường hợp cũ (dùng string "ship"), nên thay bằng số kích thước
            cellContent = "🚢";
            cellColorClass = "bg-gray-700/50";
        }
        // Bổ sung: Nếu là board của bạn và giá trị là số (kích thước thuyền)
        else if (isYourBoard && typeof value === "number" && value > 0) {
            cellContent = value; // Hiển thị kích thước thuyền (5, 4, 3, 2)
            cellColorClass = "bg-gray-700/50 text-cyan-200"; // Màu nền cho thuyền đã đặt
        }

        let className = `w-full h-full border border-gray-500 flex items-center justify-center text-xs font-bold transition cursor-pointer hover:bg-white/10 ${cellColorClass}`;

        // Thêm xử lý kéo thả chỉ cho board của bạn và ở giai đoạn đặt thuyền
        const dropProps =
            isYourBoard && phase === "placing_ships"
                ? {
                      onDrop: (e) => handleCellDrop(e, index),
                      onDragOver: handleDragOver,
                  }
                : {};

        return (
            <div
                key={index}
                className={className}
                onClick={() => {
                    if (!isYourBoard && phase === "playing") {
                        onMove(row, col);
                    }
                }}
                {...dropProps} // Truyền props kéo thả
            >
                {cellContent}
            </div>
        );
    };

    return (
        <div className="bg-transparent border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
            {" "}
            <h2 className="text-2xl font-bold text-[#00d9ff] mb-4">
                {phase === "placing_ships"
                    ? "Place Your Ships"
                    : "Battle in Progress"}{" "}
            </h2>
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                {/* Your board */}
                <div>
                    <h3 className="text-lg font-semibold text-[#00d9ff] mb-2">
                        Your Fleet
                    </h3>
                    <div className="grid grid-cols-10 gap-1 aspect-square">
                        {yourBoard.map((cell, index) =>
                            renderCell(cell, index, true)
                        )}
                    </div>
                </div>

                {/* Opponent board */}
                <div>
                    <h3 className="text-lg font-semibold text-[#00d9ff] mb-2">
                        Enemy Waters
                    </h3>
                    <div className="grid grid-cols-10 gap-1 aspect-square">
                        {opponentBoard.map((cell, index) =>
                            renderCell(cell, index, false)
                        )}
                    </div>
                </div>
            </div>
            {/* Turn indicator */}
            <div className="mt-4 text-center">
                {phase === "playing" && (
                    <p className="text-lg font-semibold text-[#00d9ff]">
                        {currentTurn === "you"
                            ? "🎯 Your Turn"
                            : "⏳ Opponent's Turn"}
                    </p>
                )}
            </div>
        </div>
    );
}
