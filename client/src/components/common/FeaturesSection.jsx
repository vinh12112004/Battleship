export default function FeaturesSection() {
  const features = [
    {
      icon: "🎯",
      title: "Real-Time Battles",
      description: "Play against other players in fast-paced tactical combat",
    },
    {
      icon: "📊",
      title: "Competitive Ranking",
      description: "Climb the leaderboard and earn prestigious titles",
    },
    {
      icon: "🛡️",
      title: "Strategic Depth",
      description: "Master fleet positioning and attack patterns",
    },
    {
      icon: "🌍",
      title: "Global Community",
      description: "Battle players from around the world",
    },
    {
      icon: "⚡",
      title: "Fast Matches",
      description: "Quick matchmaking for instant gameplay",
    },
    {
      icon: "🏆",
      title: "Achievements",
      description: "Unlock badges and special rewards",
    },
  ]

  return (
    <section className="py-20 px-6 border-t border-[#00d9ff] border-opacity-20">
      <div className="max-w-6xl mx-auto">
        <h2 className="text-4xl font-bold text-center mb-16 text-[#00d9ff]">Why Players Love Battleship</h2>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-8">
          {features.map((feature, idx) => (
            <div
              key={idx}
              className="p-6 bg-[#1a2332] rounded border border-[#00d9ff] border-opacity-20 hover:border-opacity-50 transition transform hover:scale-105 hover:glow-box"
            >
              <div className="text-4xl mb-4">{feature.icon}</div>
              <h3 className="text-xl font-bold text-[#00d9ff] mb-2">{feature.title}</h3>
              <p className="text-gray-400">{feature.description}</p>
            </div>
          ))}
        </div>
      </div>
    </section>
  )
}
