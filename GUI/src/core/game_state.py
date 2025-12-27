"""Game state management"""
from dataclasses import dataclass, field
from typing import List, Optional
from ..utils.constants import GRID_SIZE, GameState, CellState

@dataclass
class Ship:
    """Ship representation"""
    ship_type: int
    row: int
    col: int
    length: int
    is_horizontal: bool
    hits: int = 0
    is_sunk: bool = False

@dataclass
class GameStateManager:
    """Manages local game state"""
    game_id: Optional[str] = None
    opponent: Optional[str] = None
    state: str = GameState.LOBBY
    
    # Boards (10x10 grids)
    your_board: List[int] = field(default_factory=lambda: [CellState.EMPTY] * 100)
    opponent_board: List[int] = field(default_factory=lambda: [CellState.EMPTY] * 100)
    
    # Ships
    your_ships: List[Ship] = field(default_factory=list)
    
    # Turn management
    is_your_turn: bool = False
    
    # Stats
    your_hits: int = 0
    your_misses: int = 0
    opponent_hits: int = 0
    opponent_misses: int = 0
    
    def reset(self):
        """Reset game state"""
        self.game_id = None
        self.opponent = None
        self.state = GameState.LOBBY
        self.your_board = [CellState.EMPTY] * 100
        self.opponent_board = [CellState.EMPTY] * 100
        self.your_ships = []
        self.is_your_turn = False
        self.your_hits = 0
        self.your_misses = 0
        self.opponent_hits = 0
        self.opponent_misses = 0
    
    def place_ship(self, ship: Ship) -> bool:
        """Place ship on board"""
        # Validate placement
        if not self._is_valid_placement(ship):
            return False
        
        # Mark cells
        for i in range(ship.length):
            if ship.is_horizontal:
                idx = ship.row * GRID_SIZE + (ship.col + i)
            else:
                idx = (ship.row + i) * GRID_SIZE + ship.col
            
            self.your_board[idx] = CellState.SHIP
        
        self.your_ships.append(ship)
        return True
    
    def _is_valid_placement(self, ship: Ship) -> bool:
        """Check if ship placement is valid"""
        # Check bounds
        if ship.is_horizontal:
            if ship.col + ship.length > GRID_SIZE:
                return False
        else:
            if ship.row + ship.length > GRID_SIZE:
                return False
        
        # Check overlap
        for i in range(ship.length):
            if ship.is_horizontal:
                idx = ship.row * GRID_SIZE + (ship.col + i)
            else:
                idx = (ship.row + i) * GRID_SIZE + ship.col
            
            if self.your_board[idx] != CellState.EMPTY:
                return False
        
        return True
    
    def process_shot(self, row: int, col: int, is_hit: bool, is_your_shot: bool):
        """Process shot result"""
        idx = row * GRID_SIZE + col
        
        if is_your_shot:
            # Your shot on opponent's board
            self.opponent_board[idx] = CellState.HIT if is_hit else CellState.MISS
            if is_hit:
                self.your_hits += 1
            else:
                self.your_misses += 1
        else:
            # Opponent's shot on your board
            self.your_board[idx] = CellState.HIT if is_hit else CellState.MISS
            if is_hit:
                self.opponent_hits += 1
            else:
                self.opponent_misses += 1
    
    def can_shoot(self, row: int, col: int) -> bool:
        """Check if can shoot at cell"""
        if not self.is_your_turn:
            return False
        
        idx = row * GRID_SIZE + col
        cell = self.opponent_board[idx]
        
        return cell not in (CellState.HIT, CellState.MISS)