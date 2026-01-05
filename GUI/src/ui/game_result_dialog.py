from PyQt6.QtWidgets import (QDialog, QWidget, QGridLayout, QVBoxLayout, QHBoxLayout, QLabel, 
                             QPushButton, QFrame, QTabWidget, QTableWidget,
                             QTableWidgetItem, QHeaderView, QAbstractItemView)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont, QColor
from ..utils.constants import COLORS
from ..utils.logger import logger
from PyQt6.QtCore import QTimer
from .replay_board import ReplayBoard
from datetime import datetime

class GameResultDialog(QDialog):
    """Dialog hiển thị kết quả trận đấu - Đã Fix UI"""
    
    def __init__(self, result_data, logs_data, my_username,reason=None, parent=None):
        super().__init__(parent)
        self.result = result_data
        self.logs = logs_data
        # logs_data là dict
        raw_logs = logs_data.get("logs", [])

        # ✅ CHỈ LẤY LOG DICTIONARY (REPLAY)
        self.replay_logs = sorted(
            [x for x in raw_logs if isinstance(x, dict)],
            key=lambda x: x.get("turn_number", 0)
        )

        self.my_ships = logs_data.get("my_ships", [])
        self.enemy_ships = logs_data.get("enemy_ships", [])

        logger.critical("=== GAME RESULT SHIPS FINAL ===")
        logger.critical("My ships: %s", self.my_ships)
        logger.critical("Enemy ships: %s", self.enemy_ships)


        # logger.critical("=== GAME RESULT SHIPS FINAL ===")
        # logger.critical("P1 ships: %s", self.player1_ships)
        # logger.critical("P2 ships: %s", self.player2_ships)
        # logger.critical("P1 username: %s", self.player1_username)
        # logger.critical("P2 username: %s", self.player2_username)
        self.replay_index = 0 

        self.my_username = my_username
        self.reason = reason
        
        self.setWindowTitle("Match Report") # Bỏ icon unicode ở title bar để tránh lỗi win7/linux cũ
        self.setFixedSize(900, 650)
        self.setWindowFlags(Qt.WindowType.Dialog | Qt.WindowType.WindowStaysOnTopHint)
        
        # Font setup
        self.font_bold = QFont("Arial", 11, QFont.Weight.Bold)
        self.font_norm = QFont("Arial", 11)
        self.font_title = QFont("Arial", 24, QFont.Weight.Bold)
        
        self.init_ui()
        self.apply_style()
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(15)
        layout.setContentsMargins(20, 20, 20, 20)
        
        # --- HEADER ---
        winner_username = self.result.get('winner_username', 'Unknown')
        you_won = (winner_username == self.my_username)
        
        header_frame = QFrame()
        header_frame.setObjectName("HeaderFrame")
        h_layout = QVBoxLayout(header_frame)
        h_layout.setSpacing(5)
        
        # Icon & Title (Dùng text an toàn nếu icon lỗi)
        status_text = "VICTORY" if you_won else "DEFEAT"
        status_color = "#4ade80" if you_won else "#ef4444"
        
        title_label = QLabel(status_text)
        title_label.setFont(self.font_title)
        title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title_label.setStyleSheet(f"color: {status_color}; letter-spacing: 2px;")
        h_layout.addWidget(title_label)
        
        subtitle = QLabel(f"Winner: {winner_username}")
        subtitle.setFont(self.font_norm)
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        subtitle.setStyleSheet("color: #94a3b8;")
        h_layout.addWidget(subtitle)
        
        layout.addWidget(header_frame)
        
        # --- TAB WIDGET ---
        tab_widget = QTabWidget()
        tab_widget.setFont(self.font_bold)
        
        results_tab = self.create_results_tab(you_won)
        tab_widget.addTab(results_tab, "Statistics")
        
        logs_tab = self.create_logs_tab()
        tab_widget.addTab(logs_tab, "Battle Log")
        
        layout.addWidget(tab_widget)
        
        # --- BUTTONS ---
        btn_layout = QHBoxLayout()
        btn_layout.addStretch()
        
        close_btn = QPushButton("Return to Dashboard")
        close_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        close_btn.setFixedHeight(45)
        close_btn.setFixedWidth(200)
        close_btn.clicked.connect(self.accept)
        btn_layout.addWidget(close_btn)
        btn_layout.addStretch()
        
        layout.addLayout(btn_layout)

    def create_results_tab(self, you_won):
        """Tạo tab kết quả (Đã xóa Duration và Stats thừa)"""
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setSpacing(20)
        layout.setContentsMargins(40, 40, 40, 40) 
        
        # --- KHUNG THÔNG TIN CHÍNH ---
        info_group = QFrame()
        info_group.setObjectName("StatsFrame")
        g_layout = QGridLayout(info_group)
        g_layout.setVerticalSpacing(20)
        g_layout.setContentsMargins(20, 20, 20, 20)
        
        # Hàm hỗ trợ thêm dòng dữ liệu
        def add_row(row, label, value, color=None):
            l = QLabel(label)
            l.setFont(self.font_norm)
            l.setStyleSheet("color: #cbd5e1;")
            l.setMinimumHeight(30)
            
            v = QLabel(str(value))
            v.setFont(self.font_bold)
            v.setMinimumHeight(30)
            if color: v.setStyleSheet(f"color: {color}; font-size: 16px;")
            
            g_layout.addWidget(l, row, 0)
            g_layout.addWidget(v, row, 1)

        current_row = 0

        # 1. Match ID
        game_id = self.result.get('game_id', 'N/A')
        display_id = game_id[:8] + "..." if len(game_id) > 8 else game_id
        add_row(current_row, "Match ID:", display_id)
        current_row += 1
        
        # 2. Winner
        winner_name = self.result.get('winner_username', 'Unknown')
        add_row(current_row, "Winner:", winner_name, "#facc15" if winner_name == self.my_username else "#ef4444")
        current_row += 1

        # [ĐÃ XÓA PHẦN DURATION Ở ĐÂY]
        
        # 3. Total Turns
        add_row(current_row, "Total Turns:", self.result.get('total_turns', 0))
        current_row += 1
        
        # 4. Result Reason (Timeout/Normal)
        if self.reason:
             add_row(current_row, "Result:", self.reason, "#fbbf24")
             current_row += 1

        # 5. ELO Change
        w_old = self.result.get('winner_old_elo', 0)
        w_new = self.result.get('winner_new_elo', 0)
        w_diff = w_new - w_old
        
        if w_diff != 0:
            diff_str = f"+{w_diff}" if w_diff > 0 else str(w_diff)
            add_row(current_row, "ELO Change:", f"{w_old} ➝ {w_new} ({diff_str})", "#4ade80")
        else:
            add_row(current_row, "ELO Rating:", f"{w_new} (Unranked/No change)")

        layout.addWidget(info_group)
        
        # Đẩy nội dung lên trên
        layout.addStretch()
        return tab

    def create_logs_tab(self):
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setSpacing(10)
        layout.setContentsMargins(15, 15, 15, 15)

        # ===== Controls =====
        ctrl = QHBoxLayout()
        self.btn_prev = QPushButton("◀ Previous")
        self.btn_next = QPushButton("Next ▶")

        self.btn_prev.clicked.connect(self.replay_prev)
        self.btn_next.clicked.connect(self.replay_next)

        ctrl.addWidget(self.btn_prev)
        ctrl.addWidget(self.btn_next)
        ctrl.addStretch()

        layout.addLayout(ctrl)

        # ===== DEBUG SHIPS =====
        # logger.critical("=== GAME RESULT SHIPS DEBUG ===")
        # logger.critical("My username: %s", self.my_username)
        # logger.critical("Player1: %s", self.result.get("player1"))
        # logger.critical("Player2: %s", self.result.get("player2"))

        # logger.critical("Player1 ships count: %d", len(self.player1_ships))
        # logger.critical("Player1 ships data: %s", self.player1_ships)

        # logger.critical("Player2 ships count: %d", len(self.player2_ships))
        # logger.critical("Player2 ships data: %s", self.player2_ships)
        # ===== Boards =====
        boards = QHBoxLayout()
        self.enemy_board = ReplayBoard("Enemy Board")
        self.my_board = ReplayBoard("Your Board")
        self.my_board.draw_ships(self.my_ships)
        self.enemy_board.draw_ships(self.enemy_ships)



        boards.addWidget(self.enemy_board)
        boards.addWidget(self.my_board)
        layout.addLayout(boards)

        # ===== Info =====
        self.replay_info = QLabel("Turn: -")
        self.replay_info.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.replay_info.setFont(self.font_bold)
        layout.addWidget(self.replay_info)

        self.update_replay_buttons()
        return tab

    def reset_replay(self):
        self.enemy_board.reset()
        self.my_board.reset()

        if self.my_username == self.player1_username:
            self.my_board.draw_ships(self.player1_ships)
            self.enemy_board.draw_ships(self.player2_ships)
        else:
            self.my_board.draw_ships(self.player2_ships)
            self.enemy_board.draw_ships(self.player1_ships)


        self.replay_index = 0
        self.replay_info.setText("Turn: -")



    def apply_log(self, log):
        r, c = log["row"], log["col"]
        shooter = log["player_username"]

        # Ai bắn → bắn vào board đối diện
        target = (
            self.enemy_board
            if shooter == self.my_username
            else self.my_board
        )

        if log["is_sunk"]:
            target.mark_sunk(r, c)
            result = "SUNK"
        elif log["is_hit"]:
            target.mark_hit(r, c)
            result = "HIT"
        else:
            target.mark_miss(r, c)
            result = "MISS"

        self.replay_info.setText(
            f"Turn {log['turn_number']} | {shooter} → ({r},{c}) : {result}"
        )

    def replay_next(self):
        if self.replay_index >= len(self.replay_logs):
            return

        log = self.replay_logs[self.replay_index]
        self.apply_log(log)
        self.replay_index += 1
        self.update_replay_buttons()

    def replay_prev(self):
        if self.replay_index <= 0:
            return

        self.replay_index -= 1

        self.enemy_board.reset()
        self.my_board.reset()

        self.my_board.draw_ships(self.my_ships)
        self.enemy_board.draw_ships(self.enemy_ships)



        # ✅ APPLY LOGS TỚI TURN HIỆN TẠI
        for i in range(self.replay_index):
            self.apply_log(self.replay_logs[i])

        self.update_replay_buttons()


    def update_replay_buttons(self):
        self.btn_prev.setEnabled(self.replay_index > 0)
        self.btn_next.setEnabled(self.replay_index < len(self.replay_logs))


    def apply_style(self):
        self.setStyleSheet(f"""
            QDialog {{
                background-color: {COLORS['background']};
                color: {COLORS['foreground']};
            }}
            QFrame#HeaderFrame {{
                background-color: {COLORS['card']};
                border-radius: 10px;
                border: 1px solid {COLORS['border']};
            }}
            QFrame#StatsFrame {{
                background-color: {COLORS['card']};
                border-radius: 8px;
                border: 1px solid {COLORS['border']};
            }}
            QLabel {{ color: {COLORS['foreground']}; }}
            
            /* TAB STYLE */
            QTabWidget::pane {{
                border: 1px solid {COLORS['border']};
                background: {COLORS['card']};
                border-radius: 5px;
                margin-top: 10px;
            }}
            QTabBar::tab {{
                background: {COLORS['background']};
                color: #94a3b8;
                padding: 8px 20px;
                border: 1px solid {COLORS['border']};
                border-bottom: none;
                border-top-left-radius: 5px;
                border-top-right-radius: 5px;
                margin-right: 4px;
            }}
            QTabBar::tab:selected {{
                background: {COLORS['primary']};
                color: white;
            }}
            
            /* TABLE STYLE - QUAN TRỌNG CHO BATTLE LOG */
            QTableWidget {{
                background-color: {COLORS['card']};
                color: {COLORS['foreground']};
                gridline-color: {COLORS['border']};
                border: none;
                font-size: 13px;
                outline: none; /* Bỏ đường viền nét đứt khi click */
            }}
            
            /* Màu nền dòng chẵn (trắng/gốc) */
            QTableWidget::item {{
                padding: 5px;
                border-bottom: 1px solid #334155; /* Đường kẻ mờ giữa các dòng */
            }}
            
            /* Màu nền dòng lẻ (alternate) - Sửa màu đen khó nhìn thành xám nhẹ */
            QTableWidget::item:alternate {{
                background-color: #1e293b; /* Slate-800: Màu xám đậm dễ chịu hơn đen */
            }}
            
            /* Highlight dòng đang chọn */
            QTableWidget::item:selected {{
                background-color: {COLORS['primary']};
                color: white;
            }}

            QHeaderView::section {{
                background-color: {COLORS['background']};
                color: #94a3b8;
                padding: 5px;
                border: none;
                border-bottom: 2px solid {COLORS['primary']};
                font-weight: bold;
            }}
            
            /* BUTTON STYLE */
            QPushButton {{
                background-color: {COLORS['primary']};
                color: white;
                border-radius: 22px;
                font-weight: bold;
                font-size: 14px;
            }}
            QPushButton:hover {{
                background-color: {COLORS['accent']};
            }}
        """)