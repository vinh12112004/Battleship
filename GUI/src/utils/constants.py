"""Game constants matching C server definitions"""

# Grid
GRID_SIZE = 10
CELL_SIZE = 40  # pixels for UI

# Ships
SHIP_TYPES = {
    5: {'name': 'Carrier', 'length': 5, 'color': '#4CAF50'},
    4: {'name': 'Battleship', 'length': 4, 'color': '#2196F3'},
    3: {'name': 'Destroyer', 'length': 3, 'color': '#FFC107'},
    2: {'name': 'Submarine', 'length': 2, 'color': '#9C27B0'},
    1: {'name': 'Patrol Boat', 'length': 1, 'color': '#795548'}
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
    HIT = 6
    MISS = 7

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
    'water': '#1e3a5f', 
    'ship': '#00d9ff',
}

# Network
DEFAULT_HOST = "10.54.36.24"
DEFAULT_PORT = 9090
CONNECTION_TIMEOUT = 10  # seconds
RECONNECT_DELAY = 2  # seconds
MAX_RECONNECT_ATTEMPTS = 3