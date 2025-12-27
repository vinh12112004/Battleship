from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QLabel, QPushButton, QGridLayout, QMessageBox, QListWidget)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..core.game_state import GameStateManager, Ship
from ..utils.logger import logger
from ..utils.constants import COLORS, GRID_SIZE, SHIP_TYPES

class ShipPlacementWindow(QMainWindow):
    """Ship placement window before game starts"""
    
    placement_complete = pyqtSignal()  # Signal when all ships placed
    
    def __init__(self, tcp_client, username, game_id):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.game_id = game_id
        self.game_state = GameStateManager()
        
        # Current ship being placed
        self.current_ship_type = None
        self.current_orientation = "horizontal"  # or "vertical"
        
        # Cell references
        self.cells = {}
        
        self.setWindowTitle(f"⚓ Ship Placement - {username}")
        self.setGeometry(200, 100, 900, 700)
        
        self.init_ui()
        self.apply_dark_theme()
        
        logger.info(f"Ship placement window opened for game {game_id}")
    
    def init_ui(self):
        """Initialize UI"""
        central = QWidget()
        self.setCentralWidget(central)
        
        main_layout = QHBoxLayout(central)
        main_layout.setSpacing(20)
        main_layout.setContentsMargins(20, 20, 20, 20)
        
        # Left: Ship list
        left_panel = self.create_ship_list_panel()
        main_layout.addWidget(left_panel, stretch=1)
        
        # Center: Board
        center_panel = self.create_board_panel()
        main_layout.addWidget(center_panel, stretch=2)
        
        # Right: Instructions
        right_panel = self.create_instructions_panel()
        main_layout.addWidget(right_panel, stretch=1)
    
    def create_ship_list_panel(self):
        """Create ship selection panel"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        title = QLabel("🚢 Fleet to Deploy")
        title.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']};")
        layout.addWidget(title)
        
        # Ship list
        self.ship_list = QListWidget()
        self.ship_list.setStyleSheet(f"""
            QListWidget {{
                background-color: {COLORS['card']};
                border: 2px solid {COLORS['border']};
                border-radius: 8px;
                padding: 5px;
                color: {COLORS['foreground']};
            }}
            QListWidget::item {{
                padding: 10px;
                border-bottom: 1px solid {COLORS['border']};
            }}
            QListWidget::item:selected {{
                background-color: {COLORS['primary']};
                color: {COLORS['background']};
            }}
        """)
        
        # Add ships to list
        for ship_type, info in SHIP_TYPES.items():
            item_text = f"{info['name']} (Length: {info['length']})"
            self.ship_list.addItem(item_text)
            self.ship_list.item(self.ship_list.count() - 1).setData(Qt.ItemDataRole.UserRole, ship_type)
        
        self.ship_list.itemClicked.connect(self.on_ship_selected)
        layout.addWidget(self.ship_list)
        
        # Orientation toggle
        self.orientation_btn = QPushButton("🔄 Horizontal")
        self.orientation_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['accent']};
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px;
                font-weight: bold;
            }}
            QPushButton:hover {{
                background-color: {COLORS['accent_light']};
            }}
        """)
        self.orientation_btn.clicked.connect(self.toggle_orientation)
        layout.addWidget(self.orientation_btn)
        
        return panel
    
    def create_board_panel(self):
        """Create placement board"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        title = QLabel("📍 Click on the grid to place ships")
        title.setFont(QFont("Arial", 14))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet(f"color: {COLORS['foreground']};")
        layout.addWidget(title)
        
        # Board
        board_container = QWidget()
        board_container.setStyleSheet(f"""
            background-color: {COLORS['card']};
            border: 2px solid {COLORS['primary']};
            border-radius: 8px;
            padding: 10px;
        """)
        grid_layout = QGridLayout(board_container)
        grid_layout.setSpacing(2)
        
        # Column labels
        for col in range(GRID_SIZE):
            label = QLabel(str(col))
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setStyleSheet(f"color: {COLORS['primary']}; font-weight: bold;")
            grid_layout.addWidget(label, 0, col + 1)
        
        # Row labels + cells
        for row in range(GRID_SIZE):
            # Row label
            label = QLabel(str(row))
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setStyleSheet(f"color: {COLORS['primary']}; font-weight: bold;")
            grid_layout.addWidget(label, row + 1, 0)
            
            # Cells
            for col in range(GRID_SIZE):
                cell = QPushButton("")
                cell.setFixedSize(50, 50)
                cell.setStyleSheet(f"""
                    QPushButton {{
                        background-color: {COLORS['water']};
                        border: 1px solid {COLORS['border']};
                    }}
                    QPushButton:hover {{
                        background-color: {COLORS['primary']};
                        border: 2px solid {COLORS['primary']};
                    }}
                """)
                
                cell.clicked.connect(lambda r=row, c=col: self.on_cell_click(r, c))
                
                grid_layout.addWidget(cell, row + 1, col + 1)
                self.cells[(row, col)] = cell
        
        layout.addWidget(board_container)
        
        # Ready button
        self.ready_btn = QPushButton("✅ READY TO BATTLE")
        self.ready_btn.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        self.ready_btn.setEnabled(False)
        self.ready_btn.setStyleSheet(f"""
            QPushButton {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 {COLORS['success']}, stop:1 {COLORS['primary']});
                color: white;
                border: none;
                border-radius: 8px;
                padding: 15px;
                margin-top: 10px;
            }}
            QPushButton:hover {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 {COLORS['primary']}, stop:1 {COLORS['success']});
            }}
            QPushButton:disabled {{
                background-color: {COLORS['border']};
                color: {COLORS['foreground']};
            }}
        """)
        self.ready_btn.clicked.connect(self.submit_placement)
        layout.addWidget(self.ready_btn)
        
        return panel
    
    def create_instructions_panel(self):
        """Create instructions panel"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        title = QLabel("📋 Instructions")
        title.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']};")
        layout.addWidget(title)
        
        instructions = QLabel(
            "1. Select a ship from the list\n\n"
            "2. Toggle orientation (H/V)\n\n"
            "3. Click on the grid to place\n\n"
            "4. Place all ships\n\n"
            "5. Click READY when done"
        )
        instructions.setWordWrap(True)
        instructions.setStyleSheet(f"""
            background-color: {COLORS['card']};
            color: {COLORS['foreground']};
            border: 2px solid {COLORS['border']};
            border-radius: 8px;
            padding: 15px;
            line-height: 1.6;
        """)
        layout.addWidget(instructions)
        
        layout.addStretch()
        
        return panel
    
    def on_ship_selected(self, item):
        """Handle ship selection"""
        self.current_ship_type = item.data(Qt.ItemDataRole.UserRole)
        ship_info = SHIP_TYPES.get(self.current_ship_type, {})
        logger.debug(f"Selected ship: {ship_info.get('name', 'Unknown')}")
    
    def toggle_orientation(self):
        """Toggle ship orientation"""
        if self.current_orientation == "horizontal":
            self.current_orientation = "vertical"
            self.orientation_btn.setText("🔄 Vertical")
        else:
            self.current_orientation = "horizontal"
            self.orientation_btn.setText("🔄 Horizontal")
        
        logger.debug(f"Orientation: {self.current_orientation}")
    
    def on_cell_click(self, row, col):
        """Handle cell click for placement"""
        if self.current_ship_type is None:
            QMessageBox.warning(self, "No Ship Selected", "Please select a ship from the list first!")
            return
        
        ship_info = SHIP_TYPES.get(self.current_ship_type, {})
        ship_length = ship_info.get('length', 0)
        
        # Create ship object
        ship = Ship(
            ship_type=self.current_ship_type,
            row=row,
            col=col,
            length=ship_length,
            is_horizontal=(self.current_orientation == "horizontal")
        )
        
        # Try to place ship
        if self.game_state.place_ship(ship):
            # Update UI
            self.highlight_ship_on_board(ship)
            
            # Remove from list
            for i in range(self.ship_list.count()):
                item = self.ship_list.item(i)
                if item.data(Qt.ItemDataRole.UserRole) == self.current_ship_type:
                    self.ship_list.takeItem(i)
                    break
            
            # Reset selection
            self.current_ship_type = None
            
            # Check if all ships placed
            if len(self.game_state.your_ships) == len(SHIP_TYPES):
                self.ready_btn.setEnabled(True)
                QMessageBox.information(self, "Fleet Ready", "All ships placed! Click READY to start the battle.")
            
            logger.info(f"Placed ship at ({row}, {col})")
        else:
            QMessageBox.warning(self, "Invalid Placement", "Cannot place ship here!\n\nCheck for:\n- Out of bounds\n- Overlapping ships")
    
    def highlight_ship_on_board(self, ship):
        """Highlight placed ship on board"""
        color = SHIP_TYPES.get(ship.ship_type, {}).get('color', COLORS['ship'])
        
        for i in range(ship.length):
            if ship.is_horizontal:
                cell = self.cells.get((ship.row, ship.col + i))
            else:
                cell = self.cells.get((ship.row + i, ship.col))
            
            if cell:
                cell.setStyleSheet(f"""
                    QPushButton {{
                        background-color: {color};
                        border: 2px solid {COLORS['foreground']};
                    }}
                """)
                cell.setText("🚢")
    
    def submit_placement(self):
        """Submit ship placement to server"""
        # Send PLAYER_READY message
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_READY,
            payload={
                'game_id': self.game_id,
                'board_state': self.game_state.your_board
            },
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            logger.info("Ship placement submitted")
            QMessageBox.information(self, "Waiting", "Placement submitted! Waiting for opponent...")
            self.placement_complete.emit()
            self.close()
        else:
            QMessageBox.critical(self, "Error", "Failed to submit placement. Please try again.")
    
    def apply_dark_theme(self):
        """Apply dark theme"""
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)