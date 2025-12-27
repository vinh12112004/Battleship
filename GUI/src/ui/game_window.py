from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                              QLabel, QPushButton, QGridLayout, QMessageBox)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..core.game_state import GameStateManager, CellState
from ..utils.logger import logger
from ..utils.constants import COLORS, GRID_SIZE, SHIP_TYPES
from .chat_widget import ChatWidget

class GameWindow(QMainWindow):
    """Main game window with boards and chat"""
    
    # 1. SIGNALS: Định nghĩa tín hiệu để giao tiếp Thread an toàn
    sig_move_result = pyqtSignal(dict)
    sig_game_over = pyqtSignal(dict)
    sig_turn_warning = pyqtSignal(dict)
    
    def __init__(self, tcp_client, username, game_id, opponent):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.game_id = game_id
        self.opponent = opponent
        
        # Game State (Dữ liệu cục bộ)
        self.game_state = GameStateManager()
        self.game_state.game_id = game_id
        self.game_state.opponent = opponent
        
        # Lưu tham chiếu đến các nút bấm (Button) trên bàn cờ để update màu sau này
        self.your_cells = {}      # (row, col) -> QPushButton
        self.opponent_cells = {}  # (row, col) -> QPushButton
        
        self.setWindowTitle(f"⚓ Battleship - {username} vs {opponent}")
        self.setGeometry(50, 50, 1400, 900)
        
        self.init_ui()
        self.connect_signals_to_slots()
        self.setup_handlers()
        self.apply_dark_theme()
        
        logger.info(f"Game window opened: {game_id}")
    
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
        
        # Main game area (Boards + Chat)
        game_layout = QHBoxLayout()
        
        # Left: Your board
        your_board_panel = self.create_board_panel("🛡️ Your Fleet", is_your_board=True)
        game_layout.addWidget(your_board_panel, stretch=2)
        
        # Center: Opponent board
        opponent_board_panel = self.create_board_panel("🎯 Enemy Waters", is_your_board=False)
        game_layout.addWidget(opponent_board_panel, stretch=2)
        
        # Right: Info + Chat
        right_panel = self.create_right_panel()
        game_layout.addWidget(right_panel, stretch=1)
        
        main_layout.addLayout(game_layout)
        
        # Footer with stats
        footer = self.create_footer()
        main_layout.addWidget(footer)
    
    def create_header(self):
        header = QWidget()
        layout = QHBoxLayout(header)
        
        game_info = QLabel(f"Game ID: {self.game_id}")
        game_info.setStyleSheet(f"color: {COLORS['primary']}; font-size: 14px; font-family: monospace;")
        layout.addWidget(game_info)
        
        layout.addStretch()
        
        self.turn_indicator = QLabel("⏳ Waiting...")
        self.turn_indicator.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        self.update_turn_indicator_ui(False) # Default state
        layout.addWidget(self.turn_indicator)
        
        layout.addStretch()
        
        resign_btn = QPushButton("🏳️ Resign")
        resign_btn.setStyleSheet(f"""
            QPushButton {{ background-color: {COLORS['error']}; color: white; border: none; border-radius: 8px; padding: 10px 20px; font-weight: bold; }}
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
                cell.setStyleSheet(f"""
                    QPushButton {{ background-color: {COLORS['water']}; border: 1px solid {COLORS['border']}; color: white; font-size: 20px; }}
                    QPushButton:hover {{ background-color: {COLORS['primary']}; }}
                """)
                
                if not is_your_board:
                    # Chỉ cho phép click vào bàn cờ đối thủ
                    cell.clicked.connect(lambda _, r=row, c=col: self.on_opponent_cell_click(r, c))
                else:
                    # Bàn cờ của mình thì không click được (chỉ hiển thị)
                    cell.setEnabled(False)
                
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
        
        stats_title = QLabel("📊 Game Statistics")
        stats_title.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        stats_title.setStyleSheet(f"color: {COLORS['primary']};")
        stats_layout.addWidget(stats_title)
        
        self.your_hits_label = QLabel("Your Hits: 0")
        self.your_hits_label.setStyleSheet(f"color: {COLORS['success']};")
        stats_layout.addWidget(self.your_hits_label)
        
        self.your_misses_label = QLabel("Your Misses: 0")
        self.your_misses_label.setStyleSheet(f"color: {COLORS['miss']};")
        stats_layout.addWidget(self.your_misses_label)
        
        self.opponent_hits_label = QLabel(f"{self.opponent} Hits: 0")
        self.opponent_hits_label.setStyleSheet(f"color: {COLORS['error']};")
        stats_layout.addWidget(self.opponent_hits_label)
        
        self.opponent_misses_label = QLabel(f"{self.opponent} Misses: 0")
        self.opponent_misses_label.setStyleSheet(f"color: {COLORS['miss']};")
        stats_layout.addWidget(self.opponent_misses_label)
        
        layout.addWidget(stats_widget)
        
        # Chat Widget (Giả định class ChatWidget đã handle thread safe của riêng nó hoặc không cần)
        self.chat_widget = ChatWidget(self.tcp_client, self.game_id)
        layout.addWidget(self.chat_widget, stretch=1)
        
        return panel

    def create_footer(self):
        footer = QWidget()
        layout = QHBoxLayout(footer)
        self.ships_label = QLabel("Ships Status: Active")
        self.ships_label.setStyleSheet(f"color: {COLORS['success']}; font-size: 14px;")
        layout.addWidget(self.ships_label)
        return footer

    # =========================================================================
    # 2. SIGNAL CONNECTIONS & CLEANUP
    # =========================================================================

    def connect_signals_to_slots(self):
        """Kết nối tín hiệu vào các hàm xử lý UI"""
        self.sig_move_result.connect(self.handle_move_result_ui)
        self.sig_game_over.connect(self.handle_game_over_ui)
        self.sig_turn_warning.connect(self.handle_turn_warning_ui)

    def setup_handlers(self):
        """Đăng ký wrapper để gửi signal khi có tin nhắn"""
        # Lưu lại function object để dùng cho off_message
        self._h_move = self.sig_move_result.emit
        self._h_game_over = self.sig_game_over.emit
        self._h_warning = self.sig_turn_warning.emit

        self.tcp_client.on_message(MessageType.MSG_MOVE_RESULT, self._h_move)
        self.tcp_client.on_message(MessageType.MSG_GAME_OVER, self._h_game_over)
        self.tcp_client.on_message(MessageType.MSG_TURN_WARNING, self._h_warning)

    def closeEvent(self, event):
        """Dọn dẹp handler khi đóng cửa sổ"""
        logger.info(f"Closing GameWindow {self.game_id}, cleaning up handlers...")
        try:
            self.tcp_client.off_message(MessageType.MSG_MOVE_RESULT, self._h_move)
            self.tcp_client.off_message(MessageType.MSG_GAME_OVER, self._h_game_over)
            self.tcp_client.off_message(MessageType.MSG_TURN_WARNING, self._h_warning)
            
            # Đóng luôn chat widget nếu cần thiết
            if hasattr(self.chat_widget, 'close'):
                self.chat_widget.close()
                
        except Exception as e:
            logger.error(f"Error cleaning up GameWindow: {e}")
        event.accept()

    # =========================================================================
    # 3. UI LOGIC (Handle Signals from Main Thread)
    # =========================================================================

    def on_opponent_cell_click(self, row, col):
        """Xử lý khi người chơi bấm vào bàn cờ đối thủ"""
        if not self.game_state.is_your_turn:
            QMessageBox.warning(self, "Wait", "It's not your turn!")
            return
        
        if not self.game_state.can_shoot(row, col):
            return # Đã bắn ô này rồi
        
        # Gửi tin nhắn lên Server
        msg = TCPMessage(
            type=MessageType.MSG_PLAYER_MOVE,
            payload={'game_id': self.game_id, 'row': row, 'col': col},
            token=self.tcp_client.token
        )
        if self.tcp_client.send_message(msg):
            logger.info(f"Fired at ({row}, {col})")
            # Tạm khóa lượt để tránh spam click
            self.game_state.is_your_turn = False 
            self.update_turn_indicator_ui(False)

    def handle_move_result_ui(self, payload):
        """Xử lý kết quả bắn đạn (cập nhật UI)"""
        row = payload.get('row', 0)
        col = payload.get('col', 0)
        is_hit = payload.get('is_hit', False)
        is_sunk = payload.get('is_sunk', False)
        sunk_ship_type = payload.get('sunk_ship_type', 0)
        game_over = payload.get('game_over', False)
        is_your_shot = payload.get('is_your_shot', False)
        
        logger.info(f"Result: ({row},{col}) hit={is_hit} my_shot={is_your_shot}")

        # 1. Update Game State Data
        self.game_state.process_shot(row, col, is_hit, is_your_shot)
        
        # 2. Update Cell UI
        cell = None
        if is_your_shot:
            cell = self.opponent_cells.get((row, col))
            # Logic lượt: Nếu bắn trúng -> Có thể được bắn tiếp (tùy luật), 
            # nhưng ở đây ta cứ set theo server trả về hoặc logic cơ bản.
            # Giả sử: Bắn trượt -> Mất lượt. Bắn trúng -> Còn lượt.
            self.game_state.is_your_turn = is_hit 
        else:
            cell = self.your_cells.get((row, col))
            # Nếu đối thủ bắn trượt -> Đến lượt mình
            # Nếu đối thủ bắn trúng -> Họ bắn tiếp (Mình mất lượt)
            self.game_state.is_your_turn = not is_hit

        if cell:
            if is_hit:
                cell.setText("🔥") # Fire emoji
                cell.setStyleSheet(f"background-color: {COLORS['hit']}; border: 1px solid {COLORS['error']}; font-size: 24px;")
                if is_sunk and is_your_shot:
                     QMessageBox.information(self, "Sunk!", "You sunk an enemy ship!")
            else:
                cell.setText("🌊") # Water wave
                cell.setStyleSheet(f"background-color: {COLORS['miss']}; border: 1px solid {COLORS['border']};")

        # 3. Update Stats & Indicator
        self.update_stats_display()
        self.update_turn_indicator_ui(self.game_state.is_your_turn)
        
        if game_over:
            # Game over sẽ được xử lý ở handler game_over riêng, 
            # hoặc xử lý luôn ở đây nếu Server không gửi gói MSG_GAME_OVER riêng.
            pass

    def handle_game_over_ui(self, payload):
        """Xử lý kết thúc game"""
        winner_id = payload.get('winner_id', '')
        reason = payload.get('reason', '')
        
        is_winner = (winner_id == self.username)
        msg = f"Winner: {winner_id}\nReason: {reason}"
        
        if is_winner:
            QMessageBox.information(self, "Victory!", "🏆 YOU WON!\n" + msg)
        else:
            QMessageBox.critical(self, "Defeat", "💀 YOU LOST.\n" + msg)
            
        self.close()

    def handle_turn_warning_ui(self, payload):
        """Cảnh báo sắp hết giờ"""
        sec = payload.get('seconds_remaining', 0)
        self.turn_indicator.setText(f"⚠️ {sec}s LEFT!")
        self.turn_indicator.setStyleSheet(f"background-color: {COLORS['error']}; color: white; padding: 10px; border-radius: 8px;")

    def update_turn_indicator_ui(self, is_my_turn):
        if is_my_turn:
            self.turn_indicator.setText("🎯 YOUR TURN")
            self.turn_indicator.setStyleSheet(f"background-color: {COLORS['success']}; color: white; padding: 10px 20px; border-radius: 8px; font-weight: bold;")
        else:
            self.turn_indicator.setText(f"⏳ {self.opponent}'S TURN")
            self.turn_indicator.setStyleSheet(f"background-color: {COLORS['card']}; color: {COLORS['warning']}; padding: 10px 20px; border-radius: 8px; border: 2px solid {COLORS['warning']};")

    def update_stats_display(self):
        self.your_hits_label.setText(f"Your Hits: {self.game_state.your_hits}")
        self.your_misses_label.setText(f"Your Misses: {self.game_state.your_misses}")
        self.opponent_hits_label.setText(f"{self.opponent} Hits: {self.game_state.opponent_hits}")
        self.opponent_misses_label.setText(f"{self.opponent} Misses: {self.game_state.opponent_misses}")

    def resign(self):
        reply = QMessageBox.question(self, "Resign", "Give up?", QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            # Gửi tin nhắn thua cuộc hoặc thoát
            # self.tcp_client.send_message(...)
            self.close()

    def apply_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)