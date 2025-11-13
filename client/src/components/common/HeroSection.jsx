import { Link } from "react-router-dom"
import NavalGridIllustration from "./NavalGridIllustration"

export default function HeroSection() {
  return (
    <section className="relative py-20 px-6 overflow-hidden">
      {/* Background grid */}
      <div className="absolute inset-0 opacity-10">
        <div
          className="absolute inset-0 bg-gradient-to-br from-[#00d9ff] via-transparent to-[#9d4edd]"
          style={{
            backgroundImage: "linear-gradient(45deg, #00d9ff 1px, transparent 1px)",
            backgroundSize: "40px 40px",
          }}
        ></div>
      </div>

      <div className="max-w-6xl mx-auto relative z-10">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 items-center">
          {/* Left content */}
          <div>
            <h1 className="text-5xl lg:text-6xl font-bold mb-6">
              <span className="bg-gradient-to-r from-[#00d9ff] to-[#9d4edd] bg-clip-text text-transparent">
                Strategic Naval Warfare
              </span>
            </h1>
            <p className="text-xl text-gray-300 mb-8">
              Command the seas in tactical battles against opponents worldwide. Deploy your fleet, outmaneuver enemies,
              and climb the global rankings.
            </p>
            <div className="flex gap-4">
              <Link
                to="/register"
                className="px-8 py-3 bg-[#00d9ff] text-[#0f1419] rounded font-bold hover:bg-[#00ffd9] transition transform hover:scale-105"
              >
                Play Now
              </Link>
              <button className="px-8 py-3 border-2 border-[#00d9ff] text-[#00d9ff] rounded font-bold hover:bg-[#00d9ff] hover:text-[#0f1419] transition">
                Watch Demo
              </button>
            </div>
          </div>

          {/* Right illustration */}
          <div className="flex justify-center">
            <NavalGridIllustration />
          </div>
        </div>
      </div>
    </section>
  )
}
