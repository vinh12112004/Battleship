"""Game constants matching C server definitions"""

# Grid
GRID_SIZE = 10
CELL_SIZE = 40  # pixels for UI

# Ships
SHIP_TYPES = {
    1: {"name": "Patrol Boat", "length": 2, "color": "#00d9ff"},
    2: {"name": "Submarine", "length": 3, "color": "#9d4edd"},
    3: {"name": "Destroyer", "length": 3, "color": "#fbbf24"},
    4: {"name": "Battleship", "length": 4, "color": "#ff6b35"},
    5: {"name": "Carrier", "length": 5, "color": "#00d084"},
}

# Game states
class GameState:
    LOBBY = "lobby"
    PLACING_SHIPS = "placing_ships"
    WAITING = "waiting"
    PLAYING = "playing"
    FINISHED = "finished"

# Cell states
class CellState:
    EMPTY = 0
    SHIP = 1
    HIT = 2
    MISS = 3

# Colors (matching web UI)
COLORS = {
    'background': '#0f1419',
    'foreground': '#e8f0ff',
    'primary': '#00d9ff',
    'primary_dark': '#0099cc',
    'accent': '#9d4edd',
    'accent_light': '#c77dff',
    'border': '#1e2a3a',
    'card': '#1a2332',
    'success': '#00d084',
    'error': '#ff4757',
    'warning': '#ffa502',
    'hit': '#ff4757',
    'miss': '#94a3b8',
    'ship': '#00d084',
    'water': '#1a2332',
}

# Network
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9090
CONNECTION_TIMEOUT = 10  # seconds
RECONNECT_DELAY = 2  # seconds
MAX_RECONNECT_ATTEMPTS = 3