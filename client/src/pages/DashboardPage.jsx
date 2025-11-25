"use client";

import { useState } from "react";
// import NavBar from "../components/common/NavBar";
import NavBar from "../components/common/Navbar";
import DashboardHeader from "../components/game/DashboardHeader";
import OnlinePlayers from "../components/game/OnlinePlayer";
import MatchmakingPanel from "../components/game/MatchmakingPanel";
import ActiveGames from "../components/game/ActiveGames";
import LeaderboardPreview from "../components/game/LeaderboardPreview";

export default function DashboardPage() {
  const [userName] = useState("Player One");

  return (
    <div className="min-h-screen bg-[#0f1419]">
      <NavBar />
      <div className="p-6 max-w-7xl mx-auto">
        <DashboardHeader userName={userName} />

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 mt-8">
          {/* Main column */}
          <div className="lg:col-span-2 space-y-6">
            <MatchmakingPanel />
            <ActiveGames />
          </div>

          {/* Sidebar */}
          <div className="space-y-6">
            <OnlinePlayers />
            <LeaderboardPreview />
          </div>
        </div>
      </div>
    </div>
  );
}
