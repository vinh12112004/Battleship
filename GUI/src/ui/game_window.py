from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QLabel, QPushButton, QGridLayout, QMessageBox)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..core.game_state import GameStateManager, CellState
from ..utils.logger import logger
from ..utils.constants import COLORS, GRID_SIZE, SHIP_TYPES
from .chat_widget import ChatWidget

class GameWindow(QMainWindow):
    """Main game window with boards and chat"""
    
    # Signals
    sig_move_result = pyqtSignal(dict)
    sig_game_over = pyqtSignal(dict)
    sig_turn_warning = pyqtSignal(dict)
    sig_start_game = pyqtSignal(dict)
    
    def __init__(self, tcp_client, username, game_id, opponent, current_turn, board_state=None):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.game_id = game_id
        self.opponent = opponent
        
        self.game_state = GameStateManager()
        self.game_state.game_id = game_id
        self.game_state.opponent = opponent
        
        if board_state:
            self.game_state.your_board = board_state
            logger.info(f"[GameWindow] Loaded board state with {sum(1 for x in board_state if x > 0)} ship cells")
        
        self.is_my_turn = (current_turn == self.username)
        
        # Cell references
        self.your_cells = {}
        self.opponent_cells = {}
        
        # Unicode icons
        self.FIRE = "🔥"
        self.WATER = "💧"
        self.SHIP = "🚢"
        self.TARGET = "🎯"
        self.HOURGLASS = "⏳"
        self.TROPHY = "🏆"
        self.SKULL = "💀"
        self.WARNING = "⚠️"
        self.SHIELD = "🛡️"
        self.CHART = "📊"
        
        self.setWindowTitle(f"⚓ Battleship - {username} vs {opponent}")
        self.setGeometry(50, 50, 1400, 900)
        
        self.init_ui()
        self.connect_signals_to_slots()
        self.setup_handlers()
        self.apply_dark_theme()
        
        self.update_turn_indicator()
        
        # ✅ DEBUG: Print để check game window được khởi tạo
        logger.info(f"[GameWindow] Initialized for game {game_id}")
        print(f"[DEBUG] GameWindow created: {game_id}")
    
    def init_ui(self):
        """Initialize UI components"""
        central = QWidget()
        self.setCentralWidget(central)
        
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(15)
        main_layout.setContentsMargins(15, 15, 15, 15)
        
        # Header
        header = self.create_header()
        main_layout.addWidget(header)
        
        # Main game area
        game_layout = QHBoxLayout()
        
        your_board_panel = self.create_board_panel(f"{self.SHIELD} Your Fleet", is_your_board=True)
        game_layout.addWidget(your_board_panel, stretch=2)
        
        opponent_board_panel = self.create_board_panel(f"{self.TARGET} Enemy Waters", is_your_board=False)
        game_layout.addWidget(opponent_board_panel, stretch=2)
        
        right_panel = self.create_right_panel()
        game_layout.addWidget(right_panel, stretch=1)
        
        main_layout.addLayout(game_layout)
        
        footer = self.create_footer()
        main_layout.addWidget(footer)
    
    def create_header(self):
        header = QWidget()
        layout = QHBoxLayout(header)
        
        game_info = QLabel(f"Game ID: {self.game_id}")
        game_info.setStyleSheet(f"color: {COLORS['primary']}; font-size: 14px; font-family: monospace;")
        layout.addWidget(game_info)
        
        layout.addStretch()
        
        # ✅ Turn indicator (KHÔNG hiển thị username)
        self.turn_indicator = QLabel(f"{self.HOURGLASS} Waiting for game to start...")
        self.turn_indicator.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        self.turn_indicator.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.turn_indicator.setFixedHeight(60)
        self.turn_indicator.setStyleSheet(f"""
            QLabel {{
                background-color: {COLORS['card']};
                color: {COLORS['foreground']};
                padding: 12px 20px;
                border-radius: 8px;
                border: 2px solid {COLORS['border']};
            }}
        """)
        layout.addWidget(self.turn_indicator)
        
        layout.addStretch()
        
        resign_btn = QPushButton("🏳️ Resign")
        resign_btn.setStyleSheet(f"""
            QPushButton {{ 
                background-color: {COLORS['error']}; 
                color: white; 
                border: none; 
                border-radius: 8px; 
                padding: 10px 20px; 
                font-weight: bold; 
            }}
            QPushButton:hover {{ background-color: #ff6b6b; }}
        """)
        resign_btn.clicked.connect(self.resign)
        layout.addWidget(resign_btn)
        
        return header
    
    def create_board_panel(self, title, is_your_board=False):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        title_label = QLabel(title)
        title_label.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title_label.setStyleSheet(f"color: {COLORS['success'] if is_your_board else COLORS['error']}; margin-bottom: 10px;")
        layout.addWidget(title_label)
        
        grid_container = QWidget()
        grid_container.setStyleSheet(f"background-color: {COLORS['card']}; border: 2px solid {COLORS['primary']}; border-radius: 8px; padding: 10px;")
        grid_layout = QGridLayout(grid_container)
        grid_layout.setSpacing(2)
        
        # Headers
        for i in range(GRID_SIZE):
            lbl_col = QLabel(str(i))
            lbl_col.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl_col.setStyleSheet(f"color: {COLORS['primary']}; font-weight: bold;")
            grid_layout.addWidget(lbl_col, 0, i + 1)
            
            lbl_row = QLabel(str(i))
            lbl_row.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl_row.setStyleSheet(f"color: {COLORS['primary']}; font-weight: bold;")
            grid_layout.addWidget(lbl_row, i + 1, 0)

        # Cells
        for row in range(GRID_SIZE):
            for col in range(GRID_SIZE):
                cell = QPushButton("")
                cell.setFixedSize(45, 45)
                cell.setFont(QFont("Arial", 18))
                
                # ✅ Base style for all cells
                base_style = f"""
                    QPushButton {{ 
                        background-color: {COLORS['water']}; 
                        border: 1px solid {COLORS['border']}; 
                        color: white; 
                        font-size: 20px; 
                    }}
                """
                
                if not is_your_board:
                    # Opponent board: clickable
                    cell.setStyleSheet(base_style + f"""
                        QPushButton:hover {{ 
                            background-color: {COLORS['primary']}; 
                            border: 2px solid {COLORS['accent']};
                        }}
                    """)
                    cell.clicked.connect(lambda checked=False, r=row, c=col: self.on_opponent_cell_click(r, c))
                else:
                    # Your board: show ships
                    cell.setEnabled(False)
                    
                    # ✅ Hiển thị ship nếu có
                    cell_value = self.game_state.your_board[row * GRID_SIZE + col]
                    if cell_value > 0:
                        cell.setText(self.SHIP)
                        cell.setStyleSheet(base_style + f"""
                            QPushButton {{
                                background-color: {COLORS['primary']};
                                border: 2px solid {COLORS['accent']};
                            }}
                        """)
                    else:
                        cell.setStyleSheet(base_style)
                
                grid_layout.addWidget(cell, row + 1, col + 1)
                
                if is_your_board:
                    self.your_cells[(row, col)] = cell
                else:
                    self.opponent_cells[(row, col)] = cell
        
        layout.addWidget(grid_container)
        return panel

    def create_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        stats_widget = QWidget()
        stats_widget.setStyleSheet(f"background-color: {COLORS['card']}; border: 2px solid {COLORS['border']}; border-radius: 8px; padding: 15px;")
        stats_layout = QVBoxLayout(stats_widget)
        
        stats_title = QLabel(f"{self.CHART} Game Statistics")
        stats_title.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        stats_title.setStyleSheet(f"color: {COLORS['primary']};")
        stats_layout.addWidget(stats_title)
        
        self.your_hits_label = QLabel(f"{self.FIRE} Your Hits: 0")
        self.your_hits_label.setStyleSheet(f"color: {COLORS['success']}; font-size: 12px;")
        stats_layout.addWidget(self.your_hits_label)
        
        self.your_misses_label = QLabel(f"{self.WATER} Your Misses: 0")
        self.your_misses_label.setStyleSheet(f"color: {COLORS['foreground']}; font-size: 12px;")
        stats_layout.addWidget(self.your_misses_label)
        
        self.opponent_hits_label = QLabel(f"{self.FIRE} Opponent Hits: 0")
        self.opponent_hits_label.setStyleSheet(f"color: {COLORS['error']}; font-size: 12px;")
        stats_layout.addWidget(self.opponent_hits_label)
        
        self.opponent_misses_label = QLabel(f"{self.WATER} Opponent Misses: 0")
        self.opponent_misses_label.setStyleSheet(f"color: {COLORS['foreground']}; font-size: 12px;")
        stats_layout.addWidget(self.opponent_misses_label)
        
        layout.addWidget(stats_widget)
        
        self.chat_widget = ChatWidget(self.tcp_client, self.game_id)
        layout.addWidget(self.chat_widget, stretch=1)
        
        return panel

    def create_footer(self):
        footer = QWidget()
        layout = QHBoxLayout(footer)
        self.ships_label = QLabel(f"{self.SHIP} Ships Status: Active")
        self.ships_label.setStyleSheet(f"color: {COLORS['success']}; font-size: 14px;")
        layout.addWidget(self.ships_label)
        return footer

    # =========================================================================
    # SIGNAL CONNECTIONS
    # =========================================================================

    def connect_signals_to_slots(self):
        self.sig_start_game.connect(self.handle_start_game_ui)
        self.sig_move_result.connect(self.handle_move_result_ui)
        self.sig_game_over.connect(self.handle_game_over_ui)
        self.sig_turn_warning.connect(self.handle_turn_warning_ui)

    def setup_handlers(self):
        # ✅ DEBUG: Print để check handlers được đăng ký
        logger.info(f"[GameWindow] Registering message handlers...")
        print(f"[DEBUG] Registering handlers for game {self.game_id}")
        
        # ✅ IMPORTANT: Store references to handler functions
        self._h_start = lambda payload: self.sig_start_game.emit(payload)
        self._h_move = lambda payload: self.sig_move_result.emit(payload)
        self._h_game_over = lambda payload: self.sig_game_over.emit(payload)
        self._h_warning = lambda payload: self.sig_turn_warning.emit(payload)

        # ✅ Register with unique handlers (not just .emit)
        self.tcp_client.on_message(MessageType.MSG_START_GAME, self._h_start)
        self.tcp_client.on_message(MessageType.MSG_MOVE_RESULT, self._h_move)
        self.tcp_client.on_message(MessageType.MSG_GAME_OVER, self._h_game_over)
        self.tcp_client.on_message(MessageType.MSG_TURN_WARNING, self._h_warning)
        
        logger.info(f"[GameWindow] Handlers registered successfully")
        
        # ✅ DEBUG: Verify handlers
        print(f"[DEBUG] START_GAME handler: {self._h_start}")
        print(f"[DEBUG] MOVE_RESULT handler: {self._h_move}")

    def closeEvent(self, event):
        logger.info(f"[GameWindow] Closing game {self.game_id}")
        try:
            self.tcp_client.off_message(MessageType.MSG_START_GAME, self._h_start)
            self.tcp_client.off_message(MessageType.MSG_MOVE_RESULT, self._h_move)
            self.tcp_client.off_message(MessageType.MSG_GAME_OVER, self._h_game_over)
            self.tcp_client.off_message(MessageType.MSG_TURN_WARNING, self._h_warning)
            
            if hasattr(self.chat_widget, 'close'):
                self.chat_widget.close()
        except Exception as e:
            logger.error(f"Error cleaning up GameWindow: {e}")
        event.accept()

    # =========================================================================
    # UI HANDLERS
    # =========================================================================

    def handle_start_game_ui(self, payload):
        """✅ Xử lý START_GAME - ĐƠN GIẢN"""
        current_turn = payload.get('current_turn', '')
        
        # ✅ DEBUG
        logger.info(f"[GameWindow] ========== START_GAME RECEIVED ==========")
        logger.info(f"  current_turn: {current_turn}")
        logger.info(f"  my_username: {self.username}")
        print(f"[DEBUG] ========== START_GAME ==========")
        print(f"[DEBUG] current_turn={current_turn}, me={self.username}")
        print(f"[DEBUG] Payload: {payload}")
        
        # ✅ So sánh đơn giản
        self.is_my_turn = (current_turn == self.username)
        
        # ✅ Update UI
        self.update_turn_indicator()
        
        logger.info(f"[GameWindow] Game started! My turn: {self.is_my_turn}")
        print(f"[DEBUG] is_my_turn: {self.is_my_turn}")

    def on_opponent_cell_click(self, row, col):
        """Xử lý click vào bàn cờ đối thủ"""
        # Check turn
        if not self.is_my_turn:
            QMessageBox.warning(self, "Not Your Turn", "Please wait for your turn!")
            logger.warning(f"[GameWindow] Tried to shoot but not my turn")
            return
        
        # Check đã bắn chưa
        cell = self.opponent_cells.get((row, col))
        if cell and cell.text():
            QMessageBox.warning(self, "Already Fired", "You already fired at this cell!")
            return
        
        # ✅ DEBUG
        logger.info(f"[GameWindow] Firing at ({row}, {col})")
        print(f"[DEBUG] Firing at ({row}, {col})")
        
        # Gửi message
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_MOVE,
            payload={'game_id': self.game_id, 'row': row, 'col': col},
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            # ✅ Tạm khóa turn để tránh spam
            self.is_my_turn = False
            self.update_turn_indicator()

    def handle_move_result_ui(self, payload):
        """Xử lý kết quả bắn"""
        row = payload.get('row', 0)
        col = payload.get('col', 0)
        is_hit = payload.get('is_hit', False)
        is_sunk = payload.get('is_sunk', False)
        sunk_ship_type = payload.get('sunk_ship_type', 0)
        game_over = payload.get('game_over', False)
        is_your_shot = payload.get('is_your_shot', False)
        
        # ✅ DEBUG
        logger.info(f"[GameWindow] ========== MOVE_RESULT ==========")
        logger.info(f"  Position: ({row},{col})")
        logger.info(f"  Hit: {is_hit}, Sunk: {is_sunk}, Your shot: {is_your_shot}")
        print(f"[DEBUG] ========== MOVE_RESULT ==========")
        print(f"[DEBUG] ({row},{col}) hit={is_hit} yours={is_your_shot}")

        # Update game state
        self.game_state.process_shot(row, col, is_hit, is_your_shot)
        
        # ✅ Get cell reference
        if is_your_shot:
            cell = self.opponent_cells.get((row, col))
            # Turn switches to opponent
            self.is_my_turn = False
            logger.info(f"[GameWindow] Your shot → Turn switches to opponent")
        else:
            cell = self.your_cells.get((row, col))
            # Turn switches to me
            self.is_my_turn = True
            logger.info(f"[GameWindow] Opponent's shot → Turn switches to you")

        # ✅ Update cell appearance (CRITICAL)
        if cell:
            logger.info(f"[GameWindow] Updating cell ({row},{col})")
            
            if is_hit:
                cell.setText(self.FIRE)
                cell.setStyleSheet(f"""
                    QPushButton {{ 
                        background-color: {COLORS['hit']}; 
                        border: 2px solid {COLORS['error']}; 
                        font-size: 24px;
                        color: white;
                    }}
                """)
                logger.info(f"[GameWindow] ✅ Cell marked as HIT (🔥)")
                
                if is_sunk and is_your_shot:
                    ship_name = SHIP_TYPES.get(sunk_ship_type, {}).get('name', 'Ship')
                    QMessageBox.information(self, "Ship Sunk!", f"{self.SKULL} You sunk their {ship_name}!")
            else:
                cell.setText(self.WATER)
                cell.setStyleSheet(f"""
                    QPushButton {{ 
                        background-color: {COLORS['miss']}; 
                        border: 1px solid {COLORS['border']};
                        font-size: 20px;
                        color: white;
                    }}
                """)
                logger.info(f"[GameWindow] ✅ Cell marked as MISS (💧)")
            
            # ✅ Force repaint
            cell.update()
        else:
            logger.error(f"[GameWindow] ❌ Cell ({row},{col}) not found!")

        # Update stats & turn indicator
        self.update_stats_display()
        self.update_turn_indicator()
        
        if game_over:
            # Game over handled by MSG_GAME_OVER
            pass

    def handle_game_over_ui(self, payload):
        """Xử lý game over"""
        winner_id = payload.get('winner_id', '')
        reason = payload.get('reason', 'Game ended')
        
        is_winner = (winner_id == self.username)
        
        logger.info(f"[GameWindow] GAME_OVER: winner={winner_id}, am_i_winner={is_winner}")
        
        if is_winner:
            QMessageBox.information(self, "Victory!", f"{self.TROPHY} YOU WON!\n\n{reason}")
        else:
            QMessageBox.critical(self, "Defeat", f"{self.SKULL} YOU LOST\n\n{reason}")
            
        self.close()

    def handle_turn_warning_ui(self, payload):
        """Cảnh báo timeout"""
        sec = payload.get('seconds_remaining', 0)
        
        logger.warning(f"[GameWindow] TURN_WARNING: {sec}s remaining")
        
        self.turn_indicator.setText(f"{self.WARNING} {sec}s LEFT!")
        self.turn_indicator.setStyleSheet(f"""
            QLabel {{
                background-color: {COLORS['error']}; 
                color: white; 
                padding: 12px 20px; 
                border-radius: 8px;
                font-weight: bold;
                font-size: 18px;
            }}
        """)
        
        if self.is_my_turn:
            QMessageBox.warning(self, "Time Warning!", 
                              f"{self.WARNING} Only {sec} seconds left!\n\nHurry up!")

    def update_turn_indicator(self):
        """✅ Cập nhật turn indicator - ĐƠN GIẢN (không hiển thị username)"""
        if self.is_my_turn:
            self.turn_indicator.setText(f"{self.TARGET} YOUR TURN")
            self.turn_indicator.setStyleSheet(f"""
                QLabel {{
                    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 {COLORS['success']}, stop:1 {COLORS['primary']});
                    color: white;
                    padding: 12px 20px;
                    border-radius: 8px;
                    font-weight: bold;
                    border: 2px solid {COLORS['success']};
                }}
            """)
        else:
            self.turn_indicator.setText(f"{self.HOURGLASS} OPPONENT'S TURN")
            self.turn_indicator.setStyleSheet(f"""
                QLabel {{
                    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 {COLORS['error']}, stop:1 {COLORS['warning']});
                    color: white;
                    padding: 12px 20px;
                    border-radius: 8px;
                    font-weight: bold;
                    border: 2px solid {COLORS['error']};
                }}
            """)

    def update_stats_display(self):
        """Cập nhật stats"""
        self.your_hits_label.setText(f"{self.FIRE} Your Hits: {self.game_state.your_hits}")
        self.your_misses_label.setText(f"{self.WATER} Your Misses: {self.game_state.your_misses}")
        self.opponent_hits_label.setText(f"{self.FIRE} Opponent Hits: {self.game_state.opponent_hits}")
        self.opponent_misses_label.setText(f"{self.WATER} Opponent Misses: {self.game_state.opponent_misses}")

    def resign(self):
        """Đầu hàng"""
        reply = QMessageBox.question(
            self, 
            "Resign", 
            "Are you sure you want to give up?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            logger.info("[GameWindow] Player resigned")
            self.close()

    def apply_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)