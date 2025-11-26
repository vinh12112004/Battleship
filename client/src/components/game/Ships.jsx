import styles from "./ships.module.css"

// ==========================================
// 1. Aircraft Carrier (Dark Edition)
// ==========================================
export const CarrierShip = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 500 120"
    fill="currentColor"
    height="60px"
    width="auto"
    className={styles.ship}
    {...props}
  >
    <defs>
      <linearGradient id="carrierGradModern" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stopColor="#2a2a2a" stopOpacity="1" />
        <stop offset="100%" stopColor="#0a0a0a" stopOpacity="0.8" />
      </linearGradient>
      <filter id="carrierGlow">
        <feGaussianBlur stdDeviation="2" result="coloredBlur" />
        <feMerge>
          <feMergeNode in="coloredBlur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
    </defs>

    {/* Hull with modern glow */}
    <path
      d="M15,85 L470,85 L490,65 L485,58 L10,58 L5,65 Z"
      fill="url(#carrierGradModern)"
      filter="url(#carrierGlow)"
      className={styles.hullGlow}
    />

    {/* Deck with energy lines */}
    <path d="M0,52 L495,52 L498,48 L3,48 Z" fill="#1a1a1a" opacity="0.9" />
    <line
      x1="50"
      y1="50"
      x2="450"
      y2="50"
      stroke="#FF6B35"
      strokeWidth="2"
      opacity="0.8"
      className={styles.energyLine}
    />

    {/* Island (command tower) - Sleek modern design */}
    <g opacity="0.95" className={styles.islandPulse}>
      <rect x="360" y="20" width="75" height="28" fill="#0d0d0d" rx="2" />
      <polygon points="360,20 365,10 430,10 435,20" fill="#0d0d0d" />

      {/* Tech elements */}
      <rect x="370" y="5" width="8" height="5" fill="#00D9FF" className={styles.radarSpin} />
      <rect x="385" y="3" width="15" height="7" fill="#FF6B35" opacity="0.7" className={styles.radarPulse} />
      <rect x="410" y="5" width="8" height="5" fill="#00D9FF" />

      {/* Glowing windows */}
      <rect x="368" y="25" width="8" height="10" fill="#00D9FF" opacity="1" className={styles.windowGlow} />
      <rect x="380" y="25" width="8" height="10" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
      <rect x="392" y="25" width="8" height="10" fill="#00D9FF" opacity="1" className={styles.windowGlow} />
      <rect x="404" y="25" width="8" height="10" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
      <rect x="416" y="25" width="8" height="10" fill="#00D9FF" opacity="0.9" className={styles.windowGlow} />
    </g>

    {/* Runway lines - animated */}
    <line
      x1="30"
      y1="50"
      x2="340"
      y2="50"
      stroke="#FF6B35"
      strokeWidth="2"
      strokeDasharray="20,15"
      opacity="0.9"
      className={styles.runwayAnimated}
    />
    <line
      x1="440"
      y1="50"
      x2="480"
      y2="50"
      stroke="#FF6B35"
      strokeWidth="2"
      strokeDasharray="20,15"
      opacity="0.9"
      className={styles.runwayAnimated}
    />

    {/* Elevator platforms - glowing */}
    <rect x="120" y="48" width="40" height="4" fill="#00D9FF" opacity="1" className={styles.elevatorPulse} />
    <rect x="250" y="48" width="40" height="4" fill="#00D9FF" opacity="0.8" className={styles.elevatorPulse} />

    {/* Bow detail */}
    <polygon points="485,58 490,62 495,52 490,48" opacity="0.7" fill="#FF6B35" />
  </svg>
)

// ==========================================
// 2. Battleship (Dark Edition)
// ==========================================
export const Battleship = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 400 120"
    fill="currentColor"
    height="60px"
    width="auto"
    className={styles.ship}
    {...props}
  >
    <defs>
      <linearGradient id="battleGradModern" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stopColor="#1a1a1a" stopOpacity="1" />
        <stop offset="100%" stopColor="#0a0a0a" stopOpacity="0.8" />
      </linearGradient>
      <filter id="battleGlow">
        <feGaussianBlur stdDeviation="2" />
        <feMerge>
          <feMergeNode in="coloredBlur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
    </defs>

    {/* Hull */}
    <path
      d="M8,85 L375,85 L392,62 L385,55 L20,55 L12,62 Z"
      fill="url(#battleGradModern)"
      filter="url(#battleGlow)"
      className={styles.hullGlow}
    />

    {/* Superstructure tiers */}
    <rect x="150" y="40" width="100" height="15" fill="#0d0d0d" opacity="0.95" rx="2" />
    <rect x="170" y="28" width="60" height="12" fill="#0d0d0d" opacity="0.9" rx="1" />

    {/* Command tower */}
    <rect x="190" y="15" width="20" height="13" fill="#0d0d0d" opacity="0.85" rx="1" />

    {/* Mast and radar - Animated */}
    <rect x="197" y="5" width="6" height="10" fill="#FFB800" className={styles.mastGlow} />
    <rect x="192" y="5" width="16" height="4" fill="#FF6B35" opacity="0.9" className={styles.radarPulse} />

    {/* Main gun turrets - Triple barrel with glow */}
    <g opacity="0.95">
      <ellipse cx="75" cy="50" rx="16" ry="14" fill="#2a2a2a" filter="url(#battleGlow)" />
      <rect x="85" y="45" width="45" height="3" fill="#1a1a1a" opacity="0.9" className={styles.barrelGlow} />
      <rect x="85" y="49" width="45" height="3" fill="#1a1a1a" opacity="0.85" className={styles.barrelGlow} />
      <rect x="85" y="53" width="45" height="3" fill="#1a1a1a" opacity="0.9" className={styles.barrelGlow} />
    </g>

    <g opacity="0.95">
      <ellipse cx="315" cy="50" rx="16" ry="14" fill="#2a2a2a" filter="url(#battleGlow)" />
      <rect x="270" y="45" width="45" height="3" fill="#1a1a1a" opacity="0.9" className={styles.barrelGlow} />
      <rect x="270" y="49" width="45" height="3" fill="#1a1a1a" opacity="0.85" className={styles.barrelGlow} />
      <rect x="270" y="53" width="45" height="3" fill="#1a1a1a" opacity="0.9" className={styles.barrelGlow} />
    </g>

    {/* Secondary guns */}
    <circle cx="130" cy="48" r="8" fill="#2a2a2a" opacity="0.9" className={styles.secondaryGlow} />
    <rect x="135" y="46" width="20" height="4" fill="#1a1a1a" opacity="0.85" />

    <circle cx="260" cy="48" r="8" fill="#2a2a2a" opacity="0.9" className={styles.secondaryGlow} />
    <rect x="245" y="46" width="20" height="4" fill="#1a1a1a" opacity="0.85" />

    {/* Glowing windows */}
    <rect x="175" y="32" width="6" height="6" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
    <rect x="185" y="32" width="6" height="6" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
    <rect x="195" y="32" width="6" height="6" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
    <rect x="205" y="32" width="6" height="6" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
    <rect x="215" y="32" width="6" height="6" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
  </svg>
)

// ==========================================
// 3. Destroyer (Dark Edition)
// ==========================================
export const Destroyer = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 300 120"
    fill="currentColor"
    height="60px"
    width="auto"
    className={styles.ship}
    {...props}
  >
    <defs>
      <linearGradient id="destroyerGradModern" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stopColor="#2a2a2a" stopOpacity="1" />
        <stop offset="100%" stopColor="#0d0d0d" stopOpacity="0.8" />
      </linearGradient>
      <filter id="destroyerGlow">
        <feGaussianBlur stdDeviation="1.5" />
        <feMerge>
          <feMergeNode in="coloredBlur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
    </defs>

    {/* Hull - Sleek */}
    <path
      d="M5,85 L275,85 L293,60 L288,52 L25,52 L15,60 Z"
      fill="url(#destroyerGradModern)"
      filter="url(#destroyerGlow)"
      className={styles.hullGlow}
    />

    {/* Stealth superstructure */}
    <polygon points="120,52 135,30 175,30 190,52" fill="#0d0d0d" opacity="0.95" className={styles.stealthPulse} />
    <polygon points="140,30 145,20 165,20 170,30" fill="#0d0d0d" opacity="0.9" />

    {/* Radar array - Animated spin */}
    <rect x="150" y="12" width="10" height="8" fill="#00FF88" opacity="0.9" className={styles.radarSpin} />
    <polygon points="145,12 155,8 165,12" fill="#00FF88" opacity="0.8" className={styles.radarPulse} />

    {/* Glowing windows */}
    <rect x="143" y="24" width="5" height="4" fill="#00FF88" opacity="0.8" className={styles.windowGlow} />
    <rect x="152" y="24" width="5" height="4" fill="#00FF88" opacity="0.7" className={styles.windowGlow} />
    <rect x="161" y="24" width="5" height="4" fill="#00FF88" opacity="0.9" className={styles.windowGlow} />

    {/* Smokestack */}
    <rect x="195" y="35" width="12" height="17" rx="2" fill="#1a1a1a" opacity="0.85" className={styles.smokeGlow} />
    <rect x="193" y="32" width="16" height="3" fill="#2a2a2a" opacity="0.8" />

    {/* Main gun */}
    <g opacity="0.95">
      <polygon points="50,52 60,42 75,42 80,52" fill="#2a2a2a" filter="url(#destroyerGlow)" />
      <rect x="80" y="44" width="30" height="5" fill="#1a1a1a" opacity="0.9" />
      <circle cx="67" cy="47" r="8" fill="#2a2a2a" />
    </g>

    {/* VLS cells - Glowing */}
    <rect x="95" y="48" width="18" height="4" fill="#1a1a1a" opacity="0.8" className={styles.vlsPulse} />
    <line x1="99" y1="48" x2="99" y2="52" stroke="#FF6B35" strokeWidth="1" opacity="0.6" />
    <line x1="104" y1="48" x2="104" y2="52" stroke="#FF6B35" strokeWidth="1" opacity="0.6" />
    <line x1="109" y1="48" x2="109" y2="52" stroke="#FF6B35" strokeWidth="1" opacity="0.6" />

    {/* CIWS */}
    <circle cx="220" cy="45" r="6" fill="#2a2a2a" opacity="0.9" className={styles.ciws} />
    <rect x="223" y="43" width="12" height="4" fill="#1a1a1a" opacity="0.9" />
  </svg>
)

// ==========================================
// 4. Submarine (Dark Edition)
// ==========================================
export const Submarine = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 300 120"
    fill="currentColor"
    height="60px"
    width="auto"
    className={styles.ship}
    {...props}
  >
    <defs>
      <linearGradient id="subGradModern" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stopColor="#1a1a1a" stopOpacity="0.9" />
        <stop offset="50%" stopColor="#0d0d0d" stopOpacity="1" />
        <stop offset="100%" stopColor="#000000" stopOpacity="0.8" />
      </linearGradient>
      <radialGradient id="propGradModern" cx="50%" cy="50%">
        <stop offset="0%" stopColor="#00D9FF" stopOpacity="0.9" />
        <stop offset="100%" stopColor="#00A3CC" stopOpacity="0.4" />
      </radialGradient>
      <filter id="subGlow">
        <feGaussianBlur stdDeviation="2" />
        <feMerge>
          <feMergeNode in="coloredBlur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
    </defs>

    {/* Main pressure hull */}
    <ellipse
      cx="150"
      cy="65"
      rx="135"
      ry="20"
      fill="url(#subGradModern)"
      filter="url(#subGlow)"
      className={styles.hullGlow}
    />

    {/* Nose cone */}
    <path d="M15,65 Q5,65 3,65 L15,55 L15,75 Z" fill="#0d0d0d" opacity="0.95" />

    {/* Tail section */}
    <path d="M285,65 L295,55 L295,75 Z" fill="#000000" opacity="0.9" />

    {/* Conning tower */}
    <path d="M120,65 L125,35 L135,30 L175,30 L185,35 L190,65 Z" fill="#0a0a0a" opacity="0.95" />

    {/* Periscopes - Animated */}
    <rect x="145" y="15" width="4" height="15" fill="#00D9FF" opacity="0.9" className={styles.periscope} />
    <rect x="155" y="18" width="4" height="12" fill="#00D9FF" opacity="0.8" className={styles.periscope} />
    <rect x="165" y="20" width="3" height="10" fill="#00D9FF" opacity="0.7" className={styles.periscope} />

    {/* Periscope heads - Glowing */}
    <circle cx="147" cy="15" r="3" fill="#FF6B35" opacity="0.9" className={styles.windowGlow} />
    <circle cx="157" cy="18" r="2.5" fill="#FF6B35" opacity="0.85" className={styles.windowGlow} />

    {/* Tower windows */}
    <rect x="140" y="40" width="6" height="8" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
    <rect x="150" y="40" width="6" height="8" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
    <rect x="160" y="40" width="6" height="8" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
    <rect x="170" y="40" width="6" height="8" fill="#00D9FF" opacity="0.75" className={styles.windowGlow} />

    {/* Propeller */}
    <circle cx="290" cy="65" r="8" fill="url(#propGradModern)" className={styles.propeller} />
    <line
      x1="290"
      y1="57"
      x2="290"
      y2="73"
      stroke="#00D9FF"
      strokeWidth="2"
      opacity="0.7"
      className={styles.propeller}
    />
    <line
      x1="282"
      y1="65"
      x2="298"
      y2="65"
      stroke="#00D9FF"
      strokeWidth="2"
      opacity="0.7"
      className={styles.propeller}
    />

    {/* Rudders - Animated */}
    <polygon points="285,55 290,50 295,55" fill="#0d0d0d" opacity="0.8" className={styles.rudderMove} />
    <polygon points="285,75 290,80 295,75" fill="#0d0d0d" opacity="0.8" className={styles.rudderMove} />

    {/* Torpedo tubes - Glowing */}
    <circle cx="30" cy="60" r="3" fill="#FF6B35" opacity="0.7" className={styles.torpedoGlow} />
    <circle cx="30" cy="68" r="3" fill="#FF6B35" opacity="0.7" className={styles.torpedoGlow} />
    <circle cx="38" cy="64" r="3" fill="#FF6B35" opacity="0.6" className={styles.torpedoGlow} />
  </svg>
)

// ==========================================
// 5. Patrol Boat (Dark Edition)
// ==========================================
export const PatrolBoat = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 200 120"
    fill="currentColor"
    height="60px"
    width="auto"
    className={styles.ship}
    {...props}
  >
    <defs>
      <linearGradient id="patrolGradModern" x1="0%" y1="0%" x2="0%" y2="100%">
        <stop offset="0%" stopColor="#1a1a1a" stopOpacity="1" />
        <stop offset="100%" stopColor="#0d0d0d" stopOpacity="0.8" />
      </linearGradient>
      <filter id="patrolGlow">
        <feGaussianBlur stdDeviation="1.5" />
        <feMerge>
          <feMergeNode in="coloredBlur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
    </defs>

    {/* Hull */}
    <path
      d="M8,85 L175,85 L193,62 L188,55 L25,55 L15,62 Z"
      fill="url(#patrolGradModern)"
      filter="url(#patrolGlow)"
      className={styles.hullGlow}
    />

    {/* Cabin/Bridge */}
    <polygon points="100,55 110,35 160,35 168,55" fill="#0d0d0d" opacity="0.95" className={styles.bridgePulse} />
    <rect x="108" y="30" width="54" height="5" fill="#0d0d0d" opacity="0.9" />

    {/* Radar mast - Spinning */}
    <rect x="132" y="20" width="6" height="10" fill="#00D9FF" opacity="0.9" className={styles.radarSpin} />
    <rect x="128" y="20" width="14" height="3" fill="#00D9FF" opacity="0.85" className={styles.radarPulse} />

    {/* Bridge windows */}
    <rect x="115" y="40" width="8" height="10" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />
    <rect x="126" y="40" width="8" height="10" fill="#00D9FF" opacity="0.8" className={styles.windowGlow} />
    <rect x="137" y="40" width="8" height="10" fill="#00D9FF" opacity="0.75" className={styles.windowGlow} />
    <rect x="148" y="40" width="8" height="10" fill="#00D9FF" opacity="0.7" className={styles.windowGlow} />

    {/* Forward gun mount */}
    <g opacity="0.95">
      <polygon points="45,55 52,48 60,48 65,55" fill="#2a2a2a" filter="url(#patrolGlow)" />
      <rect x="65" y="50" width="22" height="4" fill="#1a1a1a" opacity="0.9" />
      <circle cx="57" cy="52" r="6" fill="#2a2a2a" />
    </g>

    {/* Machine gun mount */}
    <circle cx="85" cy="52" r="4" fill="#1a1a1a" opacity="0.9" className={styles.gunPulse} />
    <rect x="87" y="50" width="10" height="4" fill="#1a1a1a" opacity="0.85" />

    {/* Navigation lights - Animated */}
    <circle cx="188" cy="58" r="2" fill="#00FF88" opacity="0.9" className={styles.navLight} />
    <circle cx="20" cy="62" r="2" fill="#FF0000" opacity="0.9" className={styles.navLight} />
  </svg>
)