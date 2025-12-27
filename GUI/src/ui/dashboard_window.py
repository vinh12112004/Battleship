from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                              QLabel, QPushButton, QListWidget, QListWidgetItem,
                              QMessageBox)
from PyQt6.QtCore import Qt, pyqtSignal, QTimer
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..utils.logger import logger
from ..utils.constants import COLORS
from .challenge_dialog import ChallengeDialog
from .online_players_widget import OnlinePlayersWidget

class DashboardWindow(QMainWindow):
    """Main dashboard window after login"""
    
    # 1. SIGNALS: Định nghĩa tín hiệu để giao tiếp giữa Thread Mạng và Thread UI
    start_game_signal = pyqtSignal(str, str)  # Để main.py bắt sự kiện chuyển cảnh
    
    # Tín hiệu nội bộ để update UI an toàn
    sig_server_start_game = pyqtSignal(dict)
    sig_challenge_received = pyqtSignal(dict)
    sig_challenge_declined = pyqtSignal(dict)
    sig_challenge_cancelled = pyqtSignal(dict)
    sig_online_players = pyqtSignal(dict)
    
    def __init__(self, tcp_client, username):
        super().__init__()
        self.tcp_client = tcp_client
        self.username = username
        self.token = self.tcp_client.token
        self.challenge_dialog = None
        
        self.setWindowTitle(f"⚓ Battleship - {username}")
        self.setGeometry(100, 100, 1200, 800)
        
        self.init_ui()
        self.connect_signals_to_slots() # Kết nối tín hiệu vào hàm xử lý UI
        self.setup_handlers()           # Đăng ký nhận tin từ Server
        self.apply_dark_theme()
        
        # Auto-refresh online players every 10s
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self.refresh_online_players)
        self.refresh_timer.start(10000)
        
        # Initial load
        self.refresh_online_players()
    
    def init_ui(self):
        """Initialize UI components"""
        central = QWidget()
        self.setCentralWidget(central)
        
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(20)
        main_layout.setContentsMargins(20, 20, 20, 20)
        
        # Header
        header = self.create_header()
        main_layout.addWidget(header)
        
        # Content area
        content_layout = QHBoxLayout()
        
        # Left panel: Matchmaking + Active Games
        left_panel = self.create_left_panel()
        content_layout.addWidget(left_panel, stretch=2)
        
        # Right panel: Online Players
        right_panel = self.create_right_panel()
        content_layout.addWidget(right_panel, stretch=1)
        
        main_layout.addLayout(content_layout)

    def create_header(self):
        header = QWidget()
        layout = QHBoxLayout(header)
        
        welcome_label = QLabel(f"Welcome, {self.username}")
        welcome_label.setFont(QFont("Arial", 24, QFont.Weight.Bold))
        welcome_label.setStyleSheet(f"color: {COLORS['primary']};")
        layout.addWidget(welcome_label)
        
        layout.addStretch()
        
        logout_btn = QPushButton("Logout")
        logout_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['error']};
                color: white; border: none; border-radius: 8px; padding: 10px 20px; font-weight: bold;
            }}
            QPushButton:hover {{ background-color: #ff6b6b; }}
        """)
        logout_btn.clicked.connect(self.logout)
        layout.addWidget(logout_btn)
        return header

    def create_left_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        # Matchmaking section
        matchmaking = QWidget()
        matchmaking.setStyleSheet(f"background-color: {COLORS['card']}; border: 2px solid {COLORS['primary']}; border-radius: 12px; padding: 20px;")
        mm_layout = QVBoxLayout(matchmaking)
        
        mm_title = QLabel("🎯 Quick Match")
        mm_title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        mm_title.setStyleSheet(f"color: {COLORS['primary']};")
        mm_layout.addWidget(mm_title)
        
        self.queue_btn = QPushButton("🔍 FIND MATCH")
        self.queue_btn.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        self.queue_btn.setStyleSheet(f"""
            QPushButton {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 {COLORS['primary']}, stop:1 {COLORS['accent']});
                color: {COLORS['background']}; border: none; border-radius: 8px; padding: 15px; margin-top: 10px;
            }}
            QPushButton:hover {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 {COLORS['accent']}, stop:1 {COLORS['primary']});
            }}
        """)
        self.queue_btn.clicked.connect(self.toggle_queue)
        mm_layout.addWidget(self.queue_btn)
        
        self.queue_status = QLabel("")
        self.queue_status.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.queue_status.setStyleSheet(f"color: {COLORS['warning']}; margin-top: 10px;")
        mm_layout.addWidget(self.queue_status)
        
        layout.addWidget(matchmaking)
        
        # Active games section
        games_label = QLabel("📋 Active Games")
        games_label.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        games_label.setStyleSheet(f"color: {COLORS['primary']}; margin-top: 20px;")
        layout.addWidget(games_label)
        
        self.games_list = QListWidget()
        self.games_list.setStyleSheet(f"""
            QListWidget {{ background-color: {COLORS['card']}; border: 2px solid {COLORS['border']}; border-radius: 8px; padding: 10px; color: {COLORS['foreground']}; }}
            QListWidget::item {{ padding: 10px; border-bottom: 1px solid {COLORS['border']}; }}
            QListWidget::item:hover {{ background-color: {COLORS['primary']}; color: {COLORS['background']}; }}
        """)
        self.games_list.itemDoubleClicked.connect(self.rejoin_game)
        layout.addWidget(self.games_list)
        layout.addStretch()
        return panel

    def create_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        
        title = QLabel("👥 Online Players")
        title.setFont(QFont("Arial", 16, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']};")
        layout.addWidget(title)
        
        self.players_widget = OnlinePlayersWidget(self.tcp_client)
        self.players_widget.challenge_player.connect(self.send_challenge)
        layout.addWidget(self.players_widget)
        
        refresh_btn = QPushButton("🔄 Refresh")
        refresh_btn.setStyleSheet(f"QPushButton {{ background-color: {COLORS['accent']}; color: white; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }}")
        refresh_btn.clicked.connect(self.refresh_online_players)
        layout.addWidget(refresh_btn)
        return panel

    # =========================================================================
    # 2. SIGNAL CONNECT & HANDLER SETUP (QUAN TRỌNG)
    # =========================================================================

    def connect_signals_to_slots(self):
        """Kết nối signal nội bộ vào hàm xử lý UI (Chạy trên Main Thread)"""
        self.sig_server_start_game.connect(self.handle_start_game_ui)
        self.sig_challenge_received.connect(self.handle_challenge_received_ui)
        self.sig_challenge_declined.connect(self.handle_challenge_declined_ui)
        self.sig_challenge_cancelled.connect(self.handle_challenge_cancelled_ui)
        self.sig_online_players.connect(self.handle_online_players_ui)

    def setup_handlers(self):
        """Đăng ký nhận tin từ server (Lưu tham chiếu để cleanup sau này)"""
        # Lưu các hàm emit vào biến instance để dùng cho off_message
        self._h_start_game = self.sig_server_start_game.emit
        self._h_chall_recv = self.sig_challenge_received.emit
        self._h_chall_decl = self.sig_challenge_declined.emit
        self._h_chall_canc = self.sig_challenge_cancelled.emit
        self._h_players = self.sig_online_players.emit

        # Đăng ký với Client Bridge
        self.tcp_client.on_message(MessageType.MSG_START_GAME, self._h_start_game)
        self.tcp_client.on_message(MessageType.MSG_CHALLENGE_RECEIVED, self._h_chall_recv)
        self.tcp_client.on_message(MessageType.MSG_CHALLENGE_DECLINED, self._h_chall_decl)
        self.tcp_client.on_message(MessageType.MSG_CHALLENGE_CANCELLED, self._h_chall_canc)
        self.tcp_client.on_message(MessageType.MSG_ONLINE_PLAYERS_LIST, self._h_players)

    def closeEvent(self, event):
        """Dọn dẹp handler khi đóng cửa sổ"""
        logger.info("Closing Dashboard, cleaning up handlers...")
        self.refresh_timer.stop()
        
        # Gỡ bỏ handler để tránh memory leak và lỗi logic
        try:
            self.tcp_client.off_message(MessageType.MSG_START_GAME, self._h_start_game)
            self.tcp_client.off_message(MessageType.MSG_CHALLENGE_RECEIVED, self._h_chall_recv)
            self.tcp_client.off_message(MessageType.MSG_CHALLENGE_DECLINED, self._h_chall_decl)
            self.tcp_client.off_message(MessageType.MSG_CHALLENGE_CANCELLED, self._h_chall_canc)
            self.tcp_client.off_message(MessageType.MSG_ONLINE_PLAYERS_LIST, self._h_players)
        except Exception as e:
            logger.error(f"Error removing handlers: {e}")
            
        event.accept()

    # =========================================================================
    # 3. UI HANDLERS (Chạy trên Main Thread - An toàn)
    # =========================================================================

    def handle_start_game_ui(self, payload):
        """Xử lý khi bắt đầu game"""
        game_id = payload.get('game_id', '')
        opponent = payload.get('opponent', '')
        logger.info(f"Game started: {game_id} vs {opponent}")
        
        # Reset queue button
        self.queue_btn.setText("🔍 FIND MATCH")
        self.queue_status.setText("")
        
        # Bắn signal ra ngoài cho main.py chuyển cảnh
        self.start_game_signal.emit(game_id, opponent)

    def handle_challenge_received_ui(self, payload):
        """Hiển thị popup thách đấu"""
        challenger_username = payload.get('challenger_username', '')
        challenge_id = payload.get('challenge_id', '')
        expires_at = payload.get('expires_at', 0)
        
        logger.info(f"Challenge received from {challenger_username}")
        
        self.challenge_dialog = ChallengeDialog(
            challenger_username, challenge_id, expires_at, self.tcp_client
        )
        self.challenge_dialog.show()

    def handle_challenge_declined_ui(self, payload):
        challenge_id = payload.get('challenge_id', '')
        logger.info(f"Challenge {challenge_id} declined")
        QMessageBox.information(self, "Challenge Declined", "Your opponent declined the challenge.")

    def handle_challenge_cancelled_ui(self, payload):
        challenge_id = payload.get('challenge_id', '')
        logger.info(f"Challenge {challenge_id} cancelled")
        if self.challenge_dialog and self.challenge_dialog.isVisible():
             self.challenge_dialog.close()
        QMessageBox.information(self, "Challenge Cancelled", "The challenge was cancelled.")

    def handle_online_players_ui(self, payload):
        """Cập nhật danh sách online"""
        players = payload.get('players', [])
        self.players_widget.update_players(players)
        logger.debug(f"Updated online players: {len(players)} players")

    # =========================================================================
    # 4. USER ACTIONS (Các hành động người dùng bấm nút)
    # =========================================================================

    def toggle_queue(self):
        if self.queue_btn.text() == "🔍 FIND MATCH":
            msg = TCPMessage(type=MessageType.MSG_JOIN_QUEUE, payload={}, token=self.tcp_client.token)
            if self.tcp_client.send_message(msg):
                self.queue_btn.setText("❌ CANCEL SEARCH")
                self.queue_status.setText("🔍 Searching for opponent...")
        else:
            msg = TCPMessage(type=MessageType.MSG_LEAVE_QUEUE, payload={}, token=self.tcp_client.token)
            if self.tcp_client.send_message(msg):
                self.queue_btn.setText("🔍 FIND MATCH")
                self.queue_status.setText("")

    def refresh_online_players(self):
        msg = TCPMessage(type=MessageType.MSG_GET_ONLINE_PLAYERS, payload={}, token=self.tcp_client.token)
        self.tcp_client.send_message(msg)

    def send_challenge(self, target_username, target_id):
        msg = TCPMessage(
            type=MessageType.MSG_CHALLENGE_PLAYER,
            payload={'target_id': target_id, 'game_mode': 'casual', 'time_control': 10},
            token=self.tcp_client.token
        )
        if self.tcp_client.send_message(msg):
            QMessageBox.information(self, "Challenge Sent", f"Challenge sent to {target_username}!")

    def rejoin_game(self, item):
        game_id = item.data(Qt.ItemDataRole.UserRole)
        # Parse opponent name logic here if needed
        opponent = item.text().split(" vs ")[1] if " vs " in item.text() else "Opponent"
        self.start_game_signal.emit(game_id, opponent)

    def logout(self):
        reply = QMessageBox.question(self, "Logout", "Are you sure you want to logout?",
                                     QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            self.tcp_client.send_message(TCPMessage(type=MessageType.MSG_LOGOUT, payload={}, token=self.tcp_client.token))
            self.close()

    def apply_dark_theme(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)