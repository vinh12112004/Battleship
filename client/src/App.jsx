import { BrowserRouter as Router, Routes, Route } from "react-router-dom";
import { AuthProvider } from "@/context/AuthContext";
import { GameProvider } from "@/context/GameContext";
import ConnectionStatus from "@/components/common/ConnectionStatus";
import LandingPage from "./pages/LandingPage";
import LoginPage from "./pages/LoginPage";
import RegisterPage from "./pages/RegisterPage";
import DashboardPage from "./pages/DashboardPage";
import GamePage from "./pages/GamePage"
import TestPage from "./pages/TestPage"
// import ProfilePage from "./pages/ProfilePage"
// import LeaderboardPage from "./pages/LeaderboardPage"

export default function App() {
  return (
    <Router>
      <AuthProvider>
        <GameProvider>
        <ConnectionStatus />
        <Routes>
          <Route path="/" element={<LandingPage />} />
          <Route path="/login" element={<LoginPage />} />
          <Route path="/register" element={<RegisterPage />} />
          <Route path="/dashboard" element={<DashboardPage />} />
          <Route path="/game/:id" element={<GamePage />} />
          <Route path="/test" element={<TestPage />} />
          {/* <Route path="/profile" element={<ProfilePage />} />
          <Route path="/leaderboard" element={<LeaderboardPage />} /> */}
        </Routes>
        </GameProvider>
      </AuthProvider>
    </Router>
  );
}
