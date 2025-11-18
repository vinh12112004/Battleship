export default function DashboardHeader({ userName }) {
  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-8 glow-box">
      <div className="flex justify-between items-start">
        <div>
          <h1 className="text-4xl font-bold text-[#00d9ff] mb-2">
            Welcome, {userName}
          </h1>
          <p className="text-gray-400">
            Ready to command your fleet and dominate the seas?
          </p>
        </div>
        <div className="text-right">
          <div className="text-3xl font-bold text-[#9d4edd]">2,450</div>
          <p className="text-gray-400 text-sm">ELO Rating</p>
        </div>
      </div>
    </div>
  );
}
