export default function GameBoard({ gameState, onMove }) {
  const { yourBoard, opponentBoard, phase, currentTurn } = gameState;

  const renderCell = (value, index, isYourBoard) => {
    let className =
      "w-full h-full border border-gray-700 flex items-center justify-center text-xs font-bold transition cursor-pointer hover:bg-opacity-20";

    if (value === "water") {
      className += " bg-blue-900";
    } else if (value === "ship") {
      className += " bg-gray-600";
    } else if (value === "hit") {
      className += " bg-red-600 text-white";
    } else if (value === "miss") {
      className += " bg-gray-400 text-white";
    } else if (value === "hidden") {
      className += " bg-gray-800";
    }

    return (
      <div
        key={index}
        className={className}
        onClick={() => {
          if (!isYourBoard && phase === "playing") {
            const row = Math.floor(index / 10);
            const col = index % 10;
            onMove(row, col);
          }
        }}
      >
        {value === "hit" && "💥"}
        {value === "miss" && "💦"}
        {value === "ship" && "🚢"}
      </div>
    );
  };

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
      <h2 className="text-2xl font-bold text-[#00d9ff] mb-4">
        {phase === "placing_ships" ? "Place Your Ships" : "Battle in Progress"}
      </h2>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Your board */}
        <div>
          <h3 className="text-lg font-semibold text-[#00d9ff] mb-2">
            Your Fleet
          </h3>
          <div className="grid grid-cols-10 gap-1 aspect-square">
            {yourBoard.map((cell, index) => renderCell(cell, index, true))}
          </div>
        </div>

        {/* Opponent board */}
        <div>
          <h3 className="text-lg font-semibold text-[#00d9ff] mb-2">
            Enemy Waters
          </h3>
          <div className="grid grid-cols-10 gap-1 aspect-square">
            {opponentBoard.map((cell, index) => renderCell(cell, index, false))}
          </div>
        </div>
      </div>

      {/* Turn indicator */}
      <div className="mt-4 text-center">
        {phase === "playing" && (
          <p className="text-lg font-semibold text-[#00d9ff]">
            {currentTurn === "you" ? "🎯 Your Turn" : "⏳ Opponent's Turn"}
          </p>
        )}
      </div>
    </div>
  );
}
