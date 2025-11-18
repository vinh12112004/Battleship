import { Link } from "react-router-dom";

export default function LeaderboardPreview() {
  const topPlayers = [
    { rank: 1, username: "ShadowFleet", eloRating: 2450 },
    { rank: 2, username: "TidalWave", eloRating: 2380 },
    { rank: 3, username: "NauticalKing", eloRating: 2310 },
  ];

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
      <div className="flex justify-between items-center mb-4">
        <h2 className="text-xl font-bold text-[#00d9ff]">Top Players</h2>
        <Link
          to="/leaderboard"
          className="text-xs text-[#00d9ff] hover:text-[#00ffd9]"
        >
          View All
        </Link>
      </div>

      <div className="space-y-3">
        {topPlayers.map((player) => (
          <div
            key={player.rank}
            className="flex items-center justify-between p-2 bg-[#0f1419] rounded"
          >
            <span className="text-[#9d4edd] font-bold w-6">#{player.rank}</span>
            <span className="text-[#e8f0ff] flex-1">{player.username}</span>
            <span className="text-[#00d9ff] font-semibold">
              {player.eloRating}
            </span>
          </div>
        ))}
      </div>
    </div>
  );
}
