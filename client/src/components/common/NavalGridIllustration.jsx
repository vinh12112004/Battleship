export default function NavalGridIllustration() {
  return (
    <svg viewBox="0 0 400 400" className="w-full max-w-md drop-shadow-lg">
      {/* Grid background */}
      <defs>
        <linearGradient id="gridGrad" x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stopColor="#00d9ff" stopOpacity="0.3" />
          <stop offset="100%" stopColor="#9d4edd" stopOpacity="0.3" />
        </linearGradient>
        <filter id="glow">
          <feGaussianBlur stdDeviation="3" result="coloredBlur" />
          <feMerge>
            <feMergeNode in="coloredBlur" />
            <feMergeNode in="SourceGraphic" />
          </feMerge>
        </filter>
      </defs>

      {/* Grid lines */}
      {[...Array(11)].map((_, i) => (
        <g key={`grid-${i}`}>
          <line x1={i * 40} y1="0" x2={i * 40} y2="400" stroke="#00d9ff" strokeWidth="1" opacity="0.2" />
          <line x1="0" y1={i * 40} x2="400" y2={i * 40} stroke="#00d9ff" strokeWidth="1" opacity="0.2" />
        </g>
      ))}

      {/* Sonar circles */}
      <circle cx="200" cy="200" r="100" fill="none" stroke="#00d9ff" strokeWidth="2" opacity="0.3" />
      <circle cx="200" cy="200" r="150" fill="none" stroke="#9d4edd" strokeWidth="1" opacity="0.2" />

      {/* Ships */}
      <g filter="url(#glow)">
        <rect x="60" y="80" width="120" height="20" fill="#00d9ff" rx="4" />
        <circle cx="70" cy="90" r="4" fill="#9d4edd" />
      </g>

      <g filter="url(#glow)">
        <rect x="240" y="200" width="80" height="15" fill="#c77dff" rx="3" />
      </g>

      {/* Target markers */}
      <circle cx="150" cy="120" r="8" fill="none" stroke="#ff4757" strokeWidth="2" />
      <line x1="150" y1="110" x2="150" y2="130" stroke="#ff4757" strokeWidth="2" />
      <line x1="140" y1="120" x2="160" y2="120" stroke="#ff4757" strokeWidth="2" />

      {/* Hit marker */}
      <circle cx="280" cy="260" r="10" fill="#00d084" opacity="0.7" />
      <path d="M 275 255 L 285 265 M 285 255 L 275 265" stroke="#00d084" strokeWidth="2" />

      {/* Miss marker */}
      <circle cx="320" cy="140" r="6" fill="none" stroke="#ffa502" strokeWidth="2" />
    </svg>
  )
}
