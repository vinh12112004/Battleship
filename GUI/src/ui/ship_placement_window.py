from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QLabel, QPushButton, QGridLayout, QMessageBox, QListWidget)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..core.game_state import GameStateManager, Ship
from ..utils.logger import logger
from ..utils.constants import COLORS, GRID_SIZE, SHIP_TYPES

class ShipPlacementWindow(QMainWindow):
    """Ship placement window before game starts"""
    
    placement_complete = pyqtSignal(str, str)
    sig_start_game = pyqtSignal(dict)
    
    def __init__(self, tcp_client, username, game_id):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.game_id = game_id
        self.game_state = GameStateManager()
        
        # Current ship being placed
        self.current_ship_type = None
        self.current_orientation = "horizontal"
        
        # Cell references
        self.cells = {}
        self.is_submitted = False
        # Flag to prevent double-close
        self._is_closing = False
        
        # ✅ Unicode symbols thay vì emoji
        self.ANCHOR = "\u2693"  # ⚓
        self.SHIP = "\U0001F6A2"  # 🚢
        self.TARGET = "\U0001F3AF"  # 🎯
        self.ARROW_ROTATE = "\U0001F504"  # 🔄
        self.CHECK = "\u2705"  # ✅
        self.CLIPBOARD = "\U0001F4CB"  # 📋
        self.PIN = "\U0001F4CD"  # 📍
        self.PEOPLE = "\U0001F465"  # 👥
        
        self.setWindowTitle(f"{self.ANCHOR} Ship Placement - {username}")
        self.setGeometry(200, 100, 1100, 800)
        
        self.init_ui()
        self.apply_dark_theme()
        
        self.setup_start_game_handler()
        
        logger.info(f"Ship placement window opened for game {game_id}")
        
    def setup_start_game_handler(self):
        logger.info("[ShipPlacement] Setting up START_GAME handler...")
        
        # Store handler reference
        self._h_start_game = lambda payload: self.sig_start_game.emit(payload)
        
        # Register handler
        self.tcp_client.on_message(MessageType.MSG_START_GAME, self._h_start_game)
        
        # Connect signal to slot
        self.sig_start_game.connect(self.handle_start_game_ui)
        
        logger.info("[ShipPlacement] START_GAME handler registered")
    
    def handle_start_game_ui(self, payload):
        """✅ Xử lý khi nhận START_GAME"""
        if self._is_closing:
            logger.warning("[ShipPlacement] Already closing, ignoring START_GAME")
            return
        
        # ✅ Extract opponent và current_turn từ payload
        opponent = payload.get('opponent', '')
        current_turn = payload.get('current_turn', '')
        
        logger.info("[ShipPlacement] ========== START_GAME RECEIVED ==========")
        logger.info(f"  Opponent: {opponent}")
        logger.info(f"  Current Turn: {current_turn}")
        logger.info(f"  Game ID: {payload.get('game_id', '')}")
        
        print(f"[DEBUG] ShipPlacement received START_GAME:")
        print(f"  opponent={opponent}, current_turn={current_turn}")
        if not opponent or not current_turn:
                logger.error("[ShipPlacement] Invalid START_GAME payload: missing opponent or current_turn")
                return
        
        self._is_closing = True
        logger.info(f"[ShipPlacement] Emitting placement_complete({opponent}, {current_turn})")
        # ✅ Emit signal với opponent và current_turn
        self.placement_complete.emit(opponent, current_turn)
        
        
    def submit_placement(self):
        """Submit ship placement to server"""
        logger.info("Submitting ship placement...")
        
        # Convert board to flat array for server
        board_state = self.game_state.your_board.copy()
        
        # ✅ DEBUG: Print board state
        print(f"[DEBUG] Submitting board state (first 20): {board_state[:20]}")
        logger.info(f"Board has {sum(1 for x in board_state if x > 0)} ship cells")
        
        # Send PLAYER_READY message
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_READY,
            payload={
                'game_id': self.game_id,
                'board_state': board_state
            },
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            logger.info("Ship placement submitted successfully")
            
            # ✅ CRITICAL: Save board state to TCP client for Game Window
            if hasattr(self.tcp_client, 'game_boards'):
                self.tcp_client.game_boards[self.game_id] = board_state
            else:
                self.tcp_client.game_boards = {self.game_id: board_state}
            
            logger.info(f"[ShipPlacement] Saved board to tcp_client.game_boards[{self.game_id}]")
            
            # ✅ Mark as submitted
            self.is_submitted = True
            
            # ✅ Update UI
            self.ready_btn.setEnabled(False)
            self.ready_btn.setText(f"{self.HOURGLASS} Waiting for opponent...")
            self.status_label.setText(f"{self.HOURGLASS} Waiting for opponent to finish placement...")
            self.status_label.setStyleSheet(f"color: {COLORS['warning']}; font-weight: bold; padding: 10px;")
            
            # ✅ KHÔNG đóng window, đợi START_GAME
            logger.info("[ShipPlacement] Waiting for START_GAME...")
            
        else:
            logger.error("Failed to submit ship placement")
            QMessageBox.critical(self, "Error", "Failed to submit placement!")
    
    def closeEvent(self, event):
        """Cleanup handler when window closes"""
        logger.info("[ShipPlacement] Closing window, cleaning up...")
        try:
            # Remove START_GAME handler
            self.tcp_client.off_message(MessageType.MSG_START_GAME, self._h_start_game)
            logger.info("[ShipPlacement] START_GAME handler removed")
        except Exception as e:
            logger.error(f"Error removing handler: {e}")
        
        event.accept()
        
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
        panel.setFixedWidth(300)
        layout = QVBoxLayout(panel)
        layout.setSpacing(15)
        
        # Title with icon
        title = QLabel(f"{self.SHIP} Fleet to Deploy")
        title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']}; padding: 10px;")
        layout.addWidget(title)
        
        # Ship list
        self.ship_list = QListWidget()
        self.ship_list.setFont(QFont("Consolas", 11))
        self.ship_list.setStyleSheet(f"""
            QListWidget {{
                background-color: {COLORS['card']};
                border: 2px solid {COLORS['border']};
                border-radius: 10px;
                padding: 8px;
                color: {COLORS['foreground']};
            }}
            QListWidget::item {{
                padding: 12px;
                border-bottom: 1px solid {COLORS['border']};
                border-radius: 5px;
                margin: 2px 0;
            }}
            QListWidget::item:selected {{
                background-color: {COLORS['primary']};
                color: {COLORS['background']};
                font-weight: bold;
            }}
            QListWidget::item:hover {{
                background-color: {COLORS['border']};
            }}
        """)
        
        # ✅ Add ships with proper formatting
        for ship_type, info in SHIP_TYPES.items():
            # Get ship icon based on type
            ship_icon = {
                5: "🚢",  # Carrier
                4: "⚓",  # Battleship
                3: "🛥️",  # Destroyer
                2: "🚤",  # Submarine
                1: "⛵"   # Patrol
            }.get(info['length'], "🚢")
            
            item_text = f"{ship_icon} {info['name']}\n   Length: {info['length']} cells"
            self.ship_list.addItem(item_text)
            self.ship_list.item(self.ship_list.count() - 1).setData(Qt.ItemDataRole.UserRole, ship_type)
        
        self.ship_list.itemClicked.connect(self.on_ship_selected)
        layout.addWidget(self.ship_list)
        
        # Orientation toggle button
        self.orientation_btn = QPushButton(f"{self.ARROW_ROTATE} Horizontal")
        self.orientation_btn.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        self.orientation_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['accent']};
                color: white;
                border: none;
                border-radius: 10px;
                padding: 15px;
                font-weight: bold;
            }}
            QPushButton:hover {{
                background-color: {COLORS['accent_light']};
            }}
        """)
        self.orientation_btn.clicked.connect(self.toggle_orientation)
        layout.addWidget(self.orientation_btn)
        
        layout.addStretch()
        
        return panel
    
    def create_board_panel(self):
        """Create placement board"""
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setSpacing(15)
        
        # Title
        title = QLabel(f"{self.PIN} Click on the grid to place your ships")
        title.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet(f"color: {COLORS['primary']}; padding: 10px;")
        layout.addWidget(title)
        
        # Board container
        board_container = QWidget()
        board_container.setStyleSheet(f"""
            background-color: {COLORS['card']};
            border: 3px solid {COLORS['primary']};
            border-radius: 12px;
            padding: 15px;
        """)
        grid_layout = QGridLayout(board_container)
        grid_layout.setSpacing(3)
        
        # Column labels
        grid_layout.addWidget(QLabel(""), 0, 0)  # Top-left corner
        for col in range(GRID_SIZE):
            label = QLabel(str(col))
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setFont(QFont("Arial", 12, QFont.Weight.Bold))
            label.setStyleSheet(f"color: {COLORS['primary']};")
            grid_layout.addWidget(label, 0, col + 1)
        
        # Row labels + cells
        for row in range(GRID_SIZE):
            # Row label
            label = QLabel(chr(65 + row))  # A, B, C, ...
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setFont(QFont("Arial", 12, QFont.Weight.Bold))
            label.setStyleSheet(f"color: {COLORS['primary']};")
            grid_layout.addWidget(label, row + 1, 0)
            
            # Cells
            for col in range(GRID_SIZE):
                cell = QPushButton("")
                cell.setFixedSize(55, 55)
                cell.setFont(QFont("Arial", 16))
                cell.setStyleSheet(f"""
                    QPushButton {{
                        background-color: {COLORS['water']};
                        border: 2px solid {COLORS['border']};
                        border-radius: 5px;
                    }}
                    QPushButton:hover {{
                        background-color: {COLORS['primary']};
                        border: 2px solid {COLORS['accent']};
                        transform: scale(1.05);
                    }}
                """)
                
                # ✅ FIX: Store row, col properly with lambda default args
                cell.clicked.connect(lambda checked=False, r=row, c=col: self.on_cell_click(r, c))
                
                grid_layout.addWidget(cell, row + 1, col + 1)
                self.cells[(row, col)] = cell
        
        layout.addWidget(board_container)
        
        # Status label
        self.status_label = QLabel("Select a ship from the list to begin")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setFont(QFont("Arial", 12))
        self.status_label.setStyleSheet(f"color: {COLORS['foreground']}; padding: 10px;")
        layout.addWidget(self.status_label)
        
        # Ready button
        self.ready_btn = QPushButton(f"{self.CHECK} READY TO BATTLE")
        self.ready_btn.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        self.ready_btn.setEnabled(False)
        self.ready_btn.setStyleSheet(f"""
            QPushButton {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 {COLORS['success']}, stop:1 {COLORS['primary']});
                color: white;
                border: none;
                border-radius: 12px;
                padding: 18px;
                margin-top: 15px;
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
        panel.setFixedWidth(280)
        layout = QVBoxLayout(panel)
        layout.setSpacing(15)
        
        # Title
        title = QLabel(f"{self.CLIPBOARD} Instructions")
        title.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']}; padding: 10px;")
        layout.addWidget(title)
        
        # Instructions text
        instructions_text = """
<div style='line-height: 1.8; font-size: 13px;'>
<p><b>1.</b> Select a ship from the list</p>
<p><b>2.</b> Toggle orientation (H/V)</p>
<p><b>3.</b> Click on grid to place</p>
<p><b>4.</b> Place all 5 ships</p>
<p><b>5.</b> Click READY when done</p>
</div>

<hr style='border: 1px solid {0}; margin: 15px 0;'>

<div style='background: {1}; padding: 12px; border-radius: 8px; margin-top: 10px;'>
<p style='color: {2}; font-weight: bold; margin-bottom: 8px;'>Ship Symbols:</p>
<p>🚢 Empty Water</p>
<p>⚓ Your Ship</p>
<p style='color: {3}; font-weight: bold; margin-top: 8px;'>Note:</p>
<p style='font-size: 11px;'>Ships cannot overlap or touch each other!</p>
</div>
        """.format(COLORS['border'], COLORS['card'], COLORS['primary'], COLORS['accent'])
        
        instructions = QLabel(instructions_text)
        instructions.setWordWrap(True)
        instructions.setTextFormat(Qt.TextFormat.RichText)
        instructions.setStyleSheet(f"""
            QLabel {{
                background-color: {COLORS['card']};
                color: {COLORS['foreground']};
                border: 2px solid {COLORS['border']};
                border-radius: 10px;
                padding: 15px;
            }}
        """)
        layout.addWidget(instructions)
        
        layout.addStretch()
        
        return panel
    
    def on_ship_selected(self, item):
        """Handle ship selection"""
        self.current_ship_type = item.data(Qt.ItemDataRole.UserRole)
        ship_info = SHIP_TYPES.get(self.current_ship_type, {})
        ship_name = ship_info.get('name', 'Unknown')
        ship_length = ship_info.get('length', 0)
        
        self.status_label.setText(f"Selected: {ship_name} (Length: {ship_length})")
        self.status_label.setStyleSheet(f"color: {COLORS['accent']}; font-weight: bold; padding: 10px;")
        
        logger.debug(f"Selected ship: {ship_name}")
    
    def toggle_orientation(self):
        """Toggle ship orientation"""
        if self.current_orientation == "horizontal":
            self.current_orientation = "vertical"
            self.orientation_btn.setText(f"{self.ARROW_ROTATE} Vertical")
        else:
            self.current_orientation = "horizontal"
            self.orientation_btn.setText(f"{self.ARROW_ROTATE} Horizontal")
        
        logger.debug(f"Orientation: {self.current_orientation}")
    
    def on_cell_click(self, row, col):
        """Handle cell click for placement"""
        # ✅ DEBUG: Print clicked coordinates
        logger.info(f"Cell clicked: row={row}, col={col}")
        print(f"[DEBUG] Clicked cell: ({row}, {col})")
        
        if self.current_ship_type is None:
            QMessageBox.warning(self, "No Ship Selected", 
                              "Please select a ship from the list first!")
            return
        
        ship_info = SHIP_TYPES.get(self.current_ship_type, {})
        ship_length = ship_info.get('length', 0)
        ship_name = ship_info.get('name', 'Unknown')
        
        # Create ship object
        ship = Ship(
            ship_type=self.current_ship_type,
            row=row,
            col=col,
            length=ship_length,
            is_horizontal=(self.current_orientation == "horizontal")
        )
        
        # ✅ DEBUG: Print ship details
        print(f"[DEBUG] Trying to place: {ship_name} at ({row},{col}), "
              f"orientation: {self.current_orientation}, length: {ship_length}")
        
        # Try to place ship
        if self.game_state.place_ship(ship):
            logger.info(f"Ship placed successfully: {ship_name} at ({row}, {col})")
            
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
            self.status_label.setText(f"{ship_name} placed! Select next ship.")
            self.status_label.setStyleSheet(f"color: {COLORS['success']}; font-weight: bold; padding: 10px;")
            
            # Check if all ships placed
            placed_count = len(self.game_state.your_ships)
            total_ships = len(SHIP_TYPES)
            
            if placed_count == total_ships:
                self.ready_btn.setEnabled(True)
                self.status_label.setText(f"{self.CHECK} All ships placed! Click READY to start battle!")
                QMessageBox.information(self, "Fleet Ready", 
                    "All ships placed successfully!\n\nClick READY to start the battle!")
            else:
                self.status_label.setText(f"Ships placed: {placed_count}/{total_ships}")
        else:
            logger.warning(f"Invalid placement: {ship_name} at ({row}, {col})")
            QMessageBox.warning(self, "Invalid Placement", 
                f"Cannot place {ship_name} here!\n\n"
                "Reasons:\n"
                "• Ship goes out of bounds\n"
                "• Overlaps with another ship\n"
                "• Too close to another ship")
    
    def highlight_ship_on_board(self, ship):
        """Highlight placed ship on board"""
        color = SHIP_TYPES.get(ship.ship_type, {}).get('color', COLORS['ship'])
        
        for i in range(ship.length):
            if ship.is_horizontal:
                target_row = ship.row
                target_col = ship.col + i
            else:
                target_row = ship.row + i
                target_col = ship.col
            
            cell = self.cells.get((target_row, target_col))
            
            if cell:
                cell.setStyleSheet(f"""
                    QPushButton {{
                        background-color: {color};
                        border: 3px solid {COLORS['primary']};
                        border-radius: 5px;
                    }}
                """)
                cell.setText("⚓")
                cell.setFont(QFont("Arial", 18))
                
                # ✅ DEBUG
                print(f"[DEBUG] Highlighted cell ({target_row}, {target_col})")
    
    def submit_placement(self):
        """Submit ship placement to server"""
        logger.info("Submitting ship placement...")
        
        # Convert board to flat array for server
        board_state = self.game_state.your_board.copy()
        
        # ✅ DEBUG: Print board state
        print(f"[DEBUG] Submitting board state (first 20): {board_state[:20]}")
        logger.info(f"Board has {sum(1 for x in board_state if x > 0)} ship cells")
        
        # Send PLAYER_READY message
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_READY,
            payload={
                'game_id': self.game_id,
                'board_state': board_state
            },
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            logger.info("Ship placement submitted successfully")
            
            # ✅ CRITICAL: Save board state to TCP client for Game Window
            if hasattr(self.tcp_client, 'game_boards'):
                self.tcp_client.game_boards[self.game_id] = board_state
            else:
                self.tcp_client.game_boards = {self.game_id: board_state}
            
            logger.info(f"[ShipPlacement] Saved board to tcp_client.game_boards[{self.game_id}]")
            
            QMessageBox.information(self, "Waiting for Opponent", 
                "Your fleet is ready!\n\nWaiting for opponent to finish placement...")
            self.ready_btn.setEnabled(False)
            self.ready_btn.setText("⏳ WAITING FOR OPPONENT...")
            self.status_label.setText("Waiting for game start...")
        else:
            logger.error("Failed to submit ship placement")
            QMessageBox.critical(self, "Error", "Failed to submit placement!")
            
    def apply_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)