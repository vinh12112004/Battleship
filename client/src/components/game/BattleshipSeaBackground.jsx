import React from 'react';

const BattleshipSeaBackground = () => {
  return (
    <svg
      className="w-full h-full absolute top-0 left-0 -z-10"
      xmlns="http://www.w3.org/2000/svg"
      preserveAspectRatio="none"
    >
      <defs>
        {/* Sea Gradient - Bright Blue */}
        <linearGradient id="deepSeaGradient" x1="0%" y1="0%" x2="0%" y2="100%">
          <stop offset="0%" stopColor="#1e3a5f" />
          <stop offset="30%" stopColor="#2563eb" />
          <stop offset="60%" stopColor="#3b82f6" />
          <stop offset="100%" stopColor="#1e40af" />
        </linearGradient>

        {/* Water Ripple Effect */}
        <filter id="waterRipple">
          <feTurbulence
            type="fractalNoise"
            baseFrequency="0.006 0.006"
            numOctaves="3"
            result="noise"
          >
            <animate
              attributeName="baseFrequency"
              dur="50s"
              values="0.006 0.006; 0.009 0.009; 0.006 0.006"
              repeatCount="indefinite"
            />
          </feTurbulence>
          <feDisplacementMap
            in="SourceGraphic"
            in2="noise"
            scale="35"
            xChannelSelector="R"
            yChannelSelector="G"
          />
        </filter>

        {/* Spotlight */}
        <radialGradient id="spotlight" cx="50%" cy="30%">
          <stop offset="0%" stopColor="rgba(96, 165, 250, 0.5)" />
          <stop offset="40%" stopColor="rgba(96, 165, 250, 0.15)" />
          <stop offset="100%" stopColor="transparent" />
        </radialGradient>

        {/* Light Rays */}
        <linearGradient id="lightRay" x1="0%" y1="0%" x2="0%" y2="100%">
          <stop offset="0%" stopColor="rgba(147, 197, 253, 0.3)" />
          <stop offset="50%" stopColor="rgba(147, 197, 253, 0.1)" />
          <stop offset="100%" stopColor="transparent" />
        </linearGradient>

        {/* Glow Filter */}
        <filter id="glow">
          <feGaussianBlur stdDeviation="4" result="coloredBlur" />
          <feMerge>
            <feMergeNode in="coloredBlur" />
            <feMergeNode in="SourceGraphic" />
          </feMerge>
        </filter>
      </defs>

      {/* Base Gradient */}
      <rect width="100%" height="100%" fill="url(#deepSeaGradient)" />

      {/* Light Rays */}
      <g opacity="0.4">
        <rect x="8%" y="0" width="4%" height="100%" fill="url(#lightRay)">
          <animate attributeName="opacity" values="0.3;0.7;0.3" dur="7s" repeatCount="indefinite" />
          <animate attributeName="x" values="8%;10%;8%" dur="15s" repeatCount="indefinite" />
        </rect>
        <rect x="28%" y="0" width="2.5%" height="100%" fill="url(#lightRay)">
          <animate attributeName="opacity" values="0.4;0.8;0.4" dur="9s" repeatCount="indefinite" />
          <animate attributeName="x" values="28%;26%;28%" dur="18s" repeatCount="indefinite" />
        </rect>
        <rect x="55%" y="0" width="3%" height="100%" fill="url(#lightRay)">
          <animate attributeName="opacity" values="0.25;0.6;0.25" dur="11s" repeatCount="indefinite" />
          <animate attributeName="x" values="55%;57%;55%" dur="20s" repeatCount="indefinite" />
        </rect>
        <rect x="78%" y="0" width="3.5%" height="100%" fill="url(#lightRay)">
          <animate attributeName="opacity" values="0.35;0.75;0.35" dur="8s" repeatCount="indefinite" />
          <animate attributeName="x" values="78%;76%;78%" dur="16s" repeatCount="indefinite" />
        </rect>
      </g>

      {/* Spotlight */}
      <ellipse cx="50%" cy="35%" rx="45%" ry="40%" fill="url(#spotlight)">
        <animate attributeName="ry" values="40%;43%;40%" dur="5s" repeatCount="indefinite" />
        <animate attributeName="opacity" values="0.8;1;0.8" dur="5s" repeatCount="indefinite" />
      </ellipse>

      {/* Water Ripple */}
      <rect width="100%" height="100%" fill="transparent" filter="url(#waterRipple)" opacity="0.7" />

      {/* Floating Bubbles */}
      <g opacity="0.5" filter="url(#glow)">
        <circle cx="12%" cy="85%" r="4" fill="rgba(56, 189, 248, 0.7)">
          <animate attributeName="cy" values="85%;5%;85%" dur="22s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.7;0" dur="22s" repeatCount="indefinite" />
          <animate attributeName="r" values="4;5;4" dur="4s" repeatCount="indefinite" />
        </circle>
        <circle cx="38%" cy="95%" r="2.5" fill="rgba(56, 189, 248, 0.6)">
          <animate attributeName="cy" values="95%;8%;95%" dur="28s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.6;0" dur="28s" repeatCount="indefinite" />
        </circle>
        <circle cx="62%" cy="88%" r="3" fill="rgba(56, 189, 248, 0.8)">
          <animate attributeName="cy" values="88%;10%;88%" dur="25s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.8;0" dur="25s" repeatCount="indefinite" />
          <animate attributeName="r" values="3;4;3" dur="3s" repeatCount="indefinite" />
        </circle>
        <circle cx="82%" cy="78%" r="2" fill="rgba(56, 189, 248, 0.5)">
          <animate attributeName="cy" values="78%;12%;78%" dur="20s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.5;0" dur="20s" repeatCount="indefinite" />
        </circle>
        <circle cx="22%" cy="72%" r="2.8" fill="rgba(56, 189, 248, 0.65)">
          <animate attributeName="cy" values="72%;18%;72%" dur="26s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.65;0" dur="26s" repeatCount="indefinite" />
        </circle>
        <circle cx="48%" cy="82%" r="1.8" fill="rgba(56, 189, 248, 0.4)">
          <animate attributeName="cy" values="82%;15%;82%" dur="24s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.4;0" dur="24s" repeatCount="indefinite" />
        </circle>
        <circle cx="92%" cy="90%" r="3.2" fill="rgba(56, 189, 248, 0.7)">
          <animate attributeName="cy" values="90%;7%;90%" dur="23s" repeatCount="indefinite" />
          <animate attributeName="opacity" values="0;0.7;0" dur="23s" repeatCount="indefinite" />
        </circle>
      </g>

      {/* Radar Rings */}
      <g filter="url(#glow)">
        <circle cx="50%" cy="50%" r="0" fill="none" stroke="rgba(56, 189, 248, 0.4)" strokeWidth="3">
          <animate attributeName="r" from="0" to="700" dur="6s" repeatCount="indefinite" />
          <animate attributeName="opacity" from="1" to="0" dur="6s" repeatCount="indefinite" />
          <animate attributeName="strokeWidth" from="3" to="1" dur="6s" repeatCount="indefinite" />
        </circle>
        
        <circle cx="50%" cy="50%" r="0" fill="none" stroke="rgba(56, 189, 248, 0.3)" strokeWidth="3">
          <animate attributeName="r" from="0" to="700" dur="6s" begin="2s" repeatCount="indefinite" />
          <animate attributeName="opacity" from="0.8" to="0" dur="6s" begin="2s" repeatCount="indefinite" />
          <animate attributeName="strokeWidth" from="3" to="1" dur="6s" begin="2s" repeatCount="indefinite" />
        </circle>

        <circle cx="50%" cy="50%" r="0" fill="none" stroke="rgba(56, 189, 248, 0.25)" strokeWidth="3">
          <animate attributeName="r" from="0" to="700" dur="6s" begin="4s" repeatCount="indefinite" />
          <animate attributeName="opacity" from="0.6" to="0" dur="6s" begin="4s" repeatCount="indefinite" />
          <animate attributeName="strokeWidth" from="3" to="1" dur="6s" begin="4s" repeatCount="indefinite" />
        </circle>
      </g>

      {/* Scanning Line */}
      <line x1="50%" y1="50%" x2="50%" y2="0%" stroke="rgba(56, 189, 248, 0.6)" strokeWidth="2" filter="url(#glow)">
        <animateTransform
          attributeName="transform"
          attributeType="XML"
          type="rotate"
          from="0 50 50"
          to="360 50 50"
          dur="8s"
          repeatCount="indefinite"
        />
        <animate attributeName="opacity" values="0.6;0.3;0.6" dur="2s" repeatCount="indefinite" />
      </line>
    </svg>
  );
};

export default BattleshipSeaBackground;