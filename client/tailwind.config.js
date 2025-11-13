/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        background: "#0f1419",
        foreground: "#e8f0ff",
        primary: "#00d9ff",
        "primary-dark": "#0099cc",
        accent: "#9d4edd",
        "accent-light": "#c77dff",
        border: "#1e2a3a",
        card: "#1a2332",
        success: "#00d084",
        error: "#ff4757",
        warning: "#ffa502",
      },
      fontFamily: {
        sans: [
          "-apple-system",
          "BlinkMacSystemFont",
          '"Segoe UI"',
          "Roboto",
          "sans-serif",
        ],
        mono: ["Menlo", "Monaco", '"Courier New"', "monospace"],
      },
    },
  },
  plugins: [],
};
