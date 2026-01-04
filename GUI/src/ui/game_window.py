from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QLabel, QPushButton, QGridLayout, QMessageBox)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..core.game_state import GameStateManager, CellState
from ..utils.logger import logger
from ..utils.constants import COLORS, GRID_SIZE, SHIP_TYPES
from .chat_widget import ChatWidget
from .game_result_dialog import GameResultDialog
from ..utils.icon_loader import IconManager
from PyQt6.QtCore import QSize


class GameWindow(QMainWindow):
    """Main game window with boards and chat"""
    
    # Signals
    sig_move_result = pyqtSignal(dict)
    sig_game_over = pyqtSignal(dict)
    sig_turn_warning = pyqtSignal(dict)
    sig_game_timeout = pyqtSignal(dict)
    sig_start_game = pyqtSignal(dict)
    game_finished = pyqtSignal()
    sig_game_result = pyqtSignal(dict)
    sig_game_logs = pyqtSignal(dict)
    
    def __init__(self, tcp_client, username, game_id, opponent, current_turn, board_state=None):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.game_id = game_id
        self.opponent = opponent
        self.is_result_shown = False
        
        self.game_state = GameStateManager()
        self.game_state.game_id = game_id
        self.game_state.opponent = opponent
        self.game_logs = []
        self.game_end_reason = None
        
        if board_state:
            self.game_state.your_board = board_state
            logger.info(f"[GameWindow] Loaded board state with {sum(1 for x in board_state if x > 0)} ship cells")
        
        self.is_my_turn = (current_turn == self.username)
        self.game_ended = False
        
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
        # Gọi update lần đầu để đảm bảo trạng thái đúng
        self.update_opponent_board_state()
        
        logger.info(f"[GameWindow] Initialized for game {game_id}")
    
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
        
        # Turn indicator
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
                
                # Base style
                base_style = f"""
                    QPushButton {{ 
                        background-color: {COLORS['water']}; 
                        border: 1px solid {COLORS['border']}; 
                        color: white; 
                        font-size: 20px; 
                    }}
                """
                cell_id = f"cell_{row}_{col}_{'me' if is_your_board else 'opp'}"
                cell.setObjectName(cell_id)
                
                if not is_your_board:
                    # Opponent board: clickable
                    # ✅ Removed 'transform' to fix logs
                    cell.setStyleSheet(base_style + f"""
                        QPushButton#{cell_id}:hover:enabled {{ 
                            background-color: #FF4444; 
                            border: 3px solid white;
                        }}
                        QPushButton#{cell_id}:pressed:enabled {{
                            background-color: #CC0000;
                        }}
                        QPushButton#{cell_id}:disabled {{
                            background-color: {COLORS['water']};
                            opacity: 1.0;
                        }}
                    """)
                
                    cell.clicked.connect(lambda checked=False, r=row, c=col: self.on_opponent_cell_click(r, c))
                    self.opponent_cells[(row, col)] = cell
                    
                    # ✅ LUÔN ENABLE BAN ĐẦU
                    cell.setEnabled(True)
                else:
                    # Your board: show ships
                    cell.setEnabled(False)
                    
                    cell_value = self.game_state.your_board[row * GRID_SIZE + col]
                    if 1 <= cell_value <= 5:
                        icon = IconManager.SHIPS.get(cell_value)

                        cell.setText("")                 # ❌ bỏ emoji
                        cell.setIcon(icon)               # ✅ SVG
                        cell.setIconSize(QSize(32, 32))

                        ship_color = SHIP_TYPES.get(cell_value, {}).get('color', COLORS['primary'])
                        cell.setStyleSheet(f"""
                            QPushButton {{
                                background-color: {ship_color};
                                border: 2px solid {COLORS['accent']};
                            }}
                        """)
                    elif cell_value == 6: # HIT (nếu bạn định nghĩa HIT=6)
                        cell.setText(self.FIRE)
                        # ... style hit ...
                    elif cell_value == 7: # MISS (nếu bạn định nghĩa MISS=7)
                        cell.setText(self.WATER)
                        # ... style miss ...
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
        self.sig_game_timeout.connect(self.handle_game_timeout_ui)
        self.sig_game_result.connect(self.handle_game_result_ui)
        self.sig_game_logs.connect(self.handle_game_logs_ui)

    def setup_handlers(self):
        # Store references
        self._h_start = lambda payload: self.sig_start_game.emit(payload)
        self._h_move = lambda payload: self.sig_move_result.emit(payload)
        self._h_game_over = lambda payload: self.sig_game_over.emit(payload)
        self._h_warning = lambda payload: self.sig_turn_warning.emit(payload)
        self._h_timeout = lambda payload: self.sig_game_timeout.emit(payload)
        self._h_game_result = self.sig_game_result.emit
        self._h_game_logs = self.sig_game_logs.emit
        # Register
        self.tcp_client.on_message(MessageType.MSG_START_GAME, self._h_start)
        self.tcp_client.on_message(MessageType.MSG_MOVE_RESULT, self._h_move)
        self.tcp_client.on_message(MessageType.MSG_GAME_OVER, self._h_game_over)
        self.tcp_client.on_message(MessageType.MSG_TURN_WARNING, self._h_warning)
        self.tcp_client.on_message(MessageType.MSG_GAME_TIMEOUT, self._h_timeout)
        self.tcp_client.on_message(MessageType.MSG_GAME_RESULT, self._h_game_result)
        self.tcp_client.on_message(MessageType.MSG_GAME_LOGS, self._h_game_logs)
        
    def closeEvent(self, event):
        try:
            self.tcp_client.off_message(MessageType.MSG_START_GAME, self._h_start)
            self.tcp_client.off_message(MessageType.MSG_MOVE_RESULT, self._h_move)
            self.tcp_client.off_message(MessageType.MSG_GAME_OVER, self._h_game_over)
            self.tcp_client.off_message(MessageType.MSG_TURN_WARNING, self._h_warning)
            self.tcp_client.off_message(MessageType.MSG_GAME_TIMEOUT, self._h_timeout)
            self.tcp_client.off_message(MessageType.MSG_GAME_TIMEOUT, self._h_timeout)
            self.tcp_client.off_message(MessageType.MSG_GAME_RESULT, self._h_game_result)
            self.tcp_client.off_message(MessageType.MSG_GAME_LOGS, self._h_game_logs)
            
            if hasattr(self.chat_widget, 'close'):
                self.chat_widget.close()
        except Exception:
            pass
        event.accept()

    # =========================================================================
    # UI HANDLERS
    # =========================================================================

    def handle_start_game_ui(self, payload):
        current_turn = payload.get('current_turn', '')
        self.is_my_turn = (current_turn == self.username)
        
        self.update_turn_indicator()
        self.update_opponent_board_state()
        
        logger.info(f"[GameWindow] Game started! is_my_turn={self.is_my_turn}")

    def on_opponent_cell_click(self, row, col):
        """Xử lý click"""
        # Logic chặn click: Kiểm tra turn
        if not self.is_my_turn:
            QMessageBox.warning(self, "Not Your Turn", "Please wait for your turn!")
            return
        
        # Check đã bắn chưa
        cell = self.opponent_cells.get((row, col))
        if cell and cell.text():
            QMessageBox.warning(self, "Already Fired", "You already fired at this cell!")
            return
        
        # Gửi bắn
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_MOVE,
            payload={'game_id': self.game_id, 'row': row, 'col': col},
            token=self.tcp_client.token
        )
        self.tcp_client.send_message(msg)

    def handle_move_result_ui(self, payload):
        """✅ Xử lý kết quả bắn"""
        row = payload.get('row', 0)
        col = payload.get('col', 0)
        is_hit = payload.get('is_hit', False)
        is_sunk = payload.get('is_sunk', False)
        sunk_ship_type = payload.get('sunk_ship_type', 0)
        game_over = payload.get('game_over', False)
        is_your_shot = payload.get('is_your_shot', False)
        
        logger.info(f"[GameWindow] ========== MOVE_RESULT ==========")
        logger.info(f"  Position: ({row},{col})")
        logger.info(f"  Hit: {is_hit}, Sunk: {is_sunk}, Your shot: {is_your_shot}")
        logger.info(f"  Game Over: {game_over}")
        
        # ✅ Update game state
        self.game_state.process_shot(row, col, is_hit, is_your_shot)
        
        # ✅ Switch turn (nếu chưa end)
        if not game_over:
            if is_your_shot:
                self.is_my_turn = False
            else:
                self.is_my_turn = True
        
        # ✅ Update cell appearance
        if is_your_shot:
            cell = self.opponent_cells.get((row, col))
        else:
            cell = self.your_cells.get((row, col))
        
        if cell:
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
                
                if is_sunk and is_your_shot and not game_over:
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
            
            cell.update()
        
        # ✅ CRITICAL: Xử lý game over
        if game_over:
            logger.info(f"[GameWindow] ========== GAME OVER (from MOVE_RESULT) ==========")
            self.game_ended = True
            
            # Determine winner
            you_won = is_your_shot  # Nếu shot của bạn → bạn thắng
            
            # Show dialog
            # QTimer.singleShot(500, lambda: self.show_game_over_dialog(you_won))
            logger.info("Waiting for MSG_GAME_RESULT to show stats...")
            pass
        else:
            # ✅ Update UI nếu game chưa end
            self.update_stats_display()
            self.update_turn_indicator()
            self.update_opponent_board_state()
            
    def show_game_over_dialog(self, you_won, reason=""):  # <--- THÊM reason="" VÀO ĐÂY
            """Hiển thị dialog và emit signal về Dashboard"""
            logger.info("[GameWindow] Showing Game Over Dialog...")
            
            msg_text = ""
            if reason:
                msg_text += f"Reason: {reason}\n\n"
                
            if you_won:
                title = "VICTORY!"
                msg_text += f"CONGRATULATIONS!\nYOU WON THE BATTLE!"
                icon = QMessageBox.Icon.Information
            else:
                title = "DEFEAT"
                msg_text += f"MISSION FAILED.\nYour fleet was destroyed or Time Out."
                icon = QMessageBox.Icon.Critical
                
            msg_box = QMessageBox(self)
            msg_box.setWindowTitle(title)
            msg_box.setText(msg_text)
            msg_box.setIcon(icon)
            msg_box.setStandardButtons(QMessageBox.StandardButton.Ok)
            msg_box.exec()
            
            logger.info("[GameWindow] Dialog closed. Emitting game_finished signal...")
            self.game_finished.emit()

    def update_opponent_board_state(self):
        """✅ Enable/disable opponent cells"""
        # ✅ Nếu game đã end → DISABLE tất cả
        if self.game_ended:
            logger.info("[GameWindow] Game ended, disabling all opponent cells")
            for cell in self.opponent_cells.values():
                cell.setEnabled(False)
            return
        
        # ✅ Logic bình thường
        logger.info(f"[GameWindow] Updating opponent board: is_my_turn={self.is_my_turn}")
        
        for (row, col), cell in self.opponent_cells.items():
            if cell.text():  # Đã bắn
                cell.setEnabled(False)
            else:
                cell.setEnabled(self.is_my_turn)

    def handle_game_over_ui(self, payload):
        """✅ Xử lý khi game kết thúc"""
        winner = payload.get('winner', '')
        loser = payload.get('loser', '')
        
        logger.info("[GameWindow] ========== GAME OVER ==========")
        logger.info(f"  Winner: {winner}")
        logger.info(f"  Loser: {loser}")
        
        # ✅ Determine if you won
        you_won = (winner == self.username)
        
        # ✅ Show dialog
        if you_won:
            QMessageBox.information(
                self, 
                "Victory!", 
                f"{self.TROPHY} Congratulations!\n\nYou defeated {self.opponent}!",
                QMessageBox.StandardButton.Ok
            )
        else:
            QMessageBox.information(
                self, 
                "Defeat", 
                f"{self.SKULL} Game Over\n\n{self.opponent} won the battle.",
                QMessageBox.StandardButton.Ok
            )
        
        # ✅ Close window and return to dashboard
        logger.info("[GameWindow] Emitting game_finished signal (from MSG_GAME_OVER)...")
        self.game_finished.emit()

    def handle_turn_warning_ui(self, payload):
        sec = payload.get('seconds_remaining', 0)
        self.turn_indicator.setText(f"{self.WARNING} {sec}s LEFT!")
        self.turn_indicator.setStyleSheet(f"background-color: {COLORS['error']}; color: white; padding: 12px; border-radius: 8px;")
        
    def handle_game_timeout_ui(self, payload):
        winner = payload.get('winner_id', 'Unknown') # Backend gửi username vào field này
        reason = payload.get('reason', 'Time Out')
        
        logger.info(f"[GameWindow] TIMEOUT received. Winner: {winner}, Reason: {reason}")
        
        # 1. Lưu lý do lại để Dialog Result dùng
        self.game_end_reason = "⏱️ TIME OUT!" 
        
        # 2. Khóa bàn cờ ngay lập tức
        self.centralWidget().setEnabled(False)
        
        # 3. Cập nhật Text Indicator
        self.turn_indicator.setText("⏳ TIME OUT!")
        self.turn_indicator.setStyleSheet(f"background-color: {COLORS['error']}; color: white;")

    def update_turn_indicator(self):
        if self.is_my_turn:
            self.turn_indicator.setText(f"{self.TARGET} YOUR TURN")
            self.turn_indicator.setStyleSheet(f"""
                QLabel {{
                    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 {COLORS['success']}, stop:1 {COLORS['primary']});
                    color: white; padding: 12px 20px; border-radius: 8px; border: 2px solid {COLORS['success']}; font-weight: bold;
                }}
            """)
        else:
            self.turn_indicator.setText(f"{self.HOURGLASS} OPPONENT'S TURN")
            self.turn_indicator.setStyleSheet(f"""
                QLabel {{
                    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 {COLORS['error']}, stop:1 {COLORS['warning']});
                    color: white; padding: 12px 20px; border-radius: 8px; border: 2px solid {COLORS['error']}; font-weight: bold;
                }}
            """)

    def update_stats_display(self):
        self.your_hits_label.setText(f"{self.FIRE} Your Hits: {self.game_state.your_hits}")
        self.your_misses_label.setText(f"{self.WATER} Your Misses: {self.game_state.your_misses}")
        self.opponent_hits_label.setText(f"{self.FIRE} Opponent Hits: {self.game_state.opponent_hits}")
        self.opponent_misses_label.setText(f"{self.WATER} Opponent Misses: {self.game_state.opponent_misses}")

    def resign(self):
        reply = QMessageBox.question(self, "Resign", "Are you sure?", QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            self.game_finished.emit()
            
    def handle_game_logs_ui(self, payload):
        """✅ Xử lý logs (có thể nhận nhiều chunks)"""
        logger.info(f"[GameWindow] Received log chunk {payload['chunk_index']+1}/{payload['total_chunks']}")
        
        # Append logs
        self.game_logs.extend(payload['logs'])
        
        # Nếu đã nhận đủ tất cả chunks
        if payload['chunk_index'] + 1 == payload['total_chunks']:
            logger.info(f"[GameWindow] All log chunks received ({len(self.game_logs)} logs)")
            # Logs đã đủ, chờ result hoặc hiển thị ngay nếu result đã có
            
    def handle_game_result_ui(self, payload):
        """✅ Hiển thị kết quả + logs (Có cơ chế chống treo)"""
        logger.info(f"[GameWindow] Game result received. Reason: {self.game_end_reason}")
        if self.is_result_shown:
            logger.warning("[GameWindow] Result dialog already shown. Ignoring duplicate message.")
            return
        self.is_result_shown = True
        # Reset biến đếm số lần thử
        self.log_wait_attempts = 0
        
        # Lưu payload lại để dùng trong hàm con
        self.result_payload = payload

        def show_dialog():
            # 1. Kiểm tra logs
            # Nếu chưa có log VÀ số lần thử < 5 -> Đợi tiếp
            if len(self.game_logs) == 0 and self.log_wait_attempts < 5:
                self.log_wait_attempts += 1
                logger.warning(f"[GameWindow] No logs received yet, waiting... ({self.log_wait_attempts}/5)")
                QTimer.singleShot(500, show_dialog)
                return
            
            # 2. Đã đủ log HOẶC đã đợi quá lâu -> Hiện Dialog
            if len(self.game_logs) == 0:
                logger.warning("[GameWindow] Giving up on logs, showing result anyway.")

            # Xác định lý do (Ưu tiên lý do Timeout đã lưu, nếu không thì lấy mặc định)
            final_reason = self.game_end_reason
            if not final_reason:
                if payload.get('winner_id'):
                    final_reason = "Game Finished 🏁"
                else:
                    final_reason = "Draw / Unknown"

            # 3. Khởi tạo và hiện Dialog
            # Lưu ý: fallback username nếu server gửi chuỗi rỗng
            result_data = self.result_payload
            if not result_data.get('winner_username'):
                # Nếu username rỗng, tạm dùng winner_id hoặc "Unknown"
                result_data['winner_username'] = result_data.get('winner_id', 'Unknown')

            result_dialog = GameResultDialog(
                result_data, 
                self.game_logs, 
                self.username, 
                reason=final_reason, # ✅ Truyền lý do vào
                parent=self
            )
            result_dialog.exec()
            
            # 4. Sau khi đóng Dialog -> Bắn tín hiệu về Dashboard
            logger.info("[GameWindow] Dialog closed. Emitting game_finished...")
            self.game_finished.emit()
            self.close()

        # Bắt đầu quy trình hiển thị
        QTimer.singleShot(100, show_dialog)

    def apply_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)