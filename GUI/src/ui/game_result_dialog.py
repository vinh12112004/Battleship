from PyQt6.QtWidgets import (QDialog, QWidget, QGridLayout, QVBoxLayout, QHBoxLayout, QLabel, 
                             QPushButton, QFrame, QTabWidget, QTableWidget,
                             QTableWidgetItem, QHeaderView, QAbstractItemView)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont, QColor
from ..utils.constants import COLORS
from ..utils.logger import logger
from datetime import datetime

class GameResultDialog(QDialog):
    """Dialog hiển thị kết quả trận đấu - Đã Fix UI"""
    
    def __init__(self, result_data, logs_data, my_username,reason=None, parent=None):
        super().__init__(parent)
        self.result = result_data
        self.logs = logs_data
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
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setSpacing(20)
        layout.setContentsMargins(20, 20, 20, 20)
        
        # 1. Match Info Grid
        info_group = QFrame()
        info_group.setObjectName("StatsFrame")
        g_layout = QGridLayout(info_group)
        g_layout.setVerticalSpacing(15)
        g_layout.setContentsMargins(15, 15, 15, 15)
        
        # Helper row func
        def add_row(row, label, value, color=None):
            l = QLabel(label)
            l.setFont(self.font_norm)
            l.setStyleSheet("color: #cbd5e1;")
            l.setMinimumHeight(25)
            
            v = QLabel(str(value))
            v.setFont(self.font_bold)
            v.setMinimumHeight(25)
            if color: v.setStyleSheet(f"color: {color};")
            
            g_layout.addWidget(l, row, 0)
            g_layout.addWidget(v, row, 1)

        # ✅ KHỞI TẠO BIẾN ĐẾM DÒNG (QUAN TRỌNG)
        current_row = 0

        # Row 1: Match ID
        game_id = self.result.get('game_id', 'N/A')
        add_row(current_row, "Match ID:", game_id[:8] + "..." if len(game_id) > 8 else game_id)
        current_row += 1
        
        # ✅ Row 2: END REASON (MỚI - Chỉ hiện nếu có lý do)
        if self.reason:
            add_row(current_row, "End Reason:", self.reason, "#facc15") # Màu vàng cảnh báo
            current_row += 1
        
        # Row 3: Duration
        raw_duration = self.result.get('game_duration', 0)
        try:
            dur_seconds = int(raw_duration)
            mins, secs = divmod(dur_seconds, 60)
            dur_str = f"{mins}m {secs}s"
        except:
            dur_str = "N/A"
        add_row(current_row, "Duration:", dur_str)
        current_row += 1
        
        # Row 4: Total Turns
        add_row(current_row, "Total Turns:", self.result.get('total_turns', 0))
        current_row += 1
        
        # Row 5: Winner ELO
        w_old = self.result.get('winner_old_elo', 0)
        w_new = self.result.get('winner_new_elo', 0)
        w_diff = w_new - w_old
        w_sign = "+" if w_diff >= 0 else ""
        add_row(current_row, "Winner ELO:", f"{w_old} -> {w_new} ({w_sign}{w_diff})", "#4ade80")
        current_row += 1
        
        # Row 6: Loser ELO
        l_old = self.result.get('loser_old_elo', 0)
        l_new = self.result.get('loser_new_elo', 0)
        l_diff = l_new - l_old
        l_sign = "+" if l_diff >= 0 else ""
        add_row(current_row, "Loser ELO:", f"{l_old} -> {l_new} ({l_sign}{l_diff})", "#ef4444")
        current_row += 1

        layout.addWidget(info_group)
        
        # 2. Detailed Stats Table
        stats_group = QFrame()
        stats_group.setObjectName("StatsFrame")
        s_layout = QGridLayout(stats_group)
        s_layout.setVerticalSpacing(10)
        
        # Header Row
        headers = ["Player", "Hits", "Misses", "Accuracy"]
        for c, h in enumerate(headers):
            lbl = QLabel(h)
            lbl.setFont(self.font_bold)
            lbl.setStyleSheet("color: #60a5fa; border-bottom: 1px solid #475569; padding-bottom: 8px;")
            s_layout.addWidget(lbl, 0, c)
            
        # Data Rows
        def add_stat_row(r, username, hits, misses, is_winner):
            total = hits + misses
            acc = (hits / total * 100) if total > 0 else 0.0
            color = "#4ade80" if is_winner else "#ef4444"
            
            uname_lbl = QLabel(username)
            uname_lbl.setStyleSheet(f"color: {color}; font-weight: bold;")
            uname_lbl.setMinimumHeight(30)
            s_layout.addWidget(uname_lbl, r, 0)
            
            s_layout.addWidget(QLabel(str(hits)), r, 1)
            s_layout.addWidget(QLabel(str(misses)), r, 2)
            s_layout.addWidget(QLabel(f"{acc:.1f}%"), r, 3)

        add_stat_row(1, self.result.get('winner_username', 'Winner'), 
                     self.result.get('winner_hits', 0), 
                     self.result.get('winner_misses', 0), True)
                     
        add_stat_row(2, self.result.get('loser_username', 'Loser'), 
                     self.result.get('loser_hits', 0), 
                     self.result.get('loser_misses', 0), False)
                     
        layout.addWidget(stats_group)
        layout.addStretch()
        return tab

    def create_logs_tab(self):
        tab = QWidget()
        layout = QVBoxLayout(tab)
        layout.setContentsMargins(0, 10, 0, 0)
        
        table = QTableWidget()
        table.setColumnCount(5)
        table.setHorizontalHeaderLabels(["#", "Player", "Pos", "Result", "Time"])
        
        # Style headers
        header = table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents) # Turn
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)          # Player
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents) # Pos
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.Stretch)          # Result
        header.setSectionResizeMode(4, QHeaderView.ResizeMode.ResizeToContents) # Time
        
        table.setRowCount(len(self.logs))
        table.setAlternatingRowColors(True) # Bật tính năng đổi màu dòng chẵn lẻ
        table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        table.setShowGrid(False) # Bỏ grid line để nhìn thoáng hơn
        
        for i, log in enumerate(self.logs):
            # 1. Turn
            t_item = QTableWidgetItem(str(log.get('turn_number', i+1)))
            t_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            table.setItem(i, 0, t_item)
            
            # 2. Player
            p_name = log.get('player_username', 'Unknown')
            p_item = QTableWidgetItem(p_name)
            if p_name == self.my_username:
                p_item.setForeground(QColor("#60a5fa")) # Blue for me
                p_item.setFont(self.font_bold)
            table.setItem(i, 1, p_item)
            
            # 3. Action (Position format: (row, col))
            row = log.get('row', 0)
            col = log.get('col', 0)
            # Backend lưu 0-9, hiển thị 0-9 cho đồng bộ với yêu cầu
            pos_str = f"({row}, {col})"
            pos_item = QTableWidgetItem(pos_str)
            pos_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            table.setItem(i, 2, pos_item)
            
            # 4. Result (Hit/Miss/Sunk)
            res_str = ""
            res_color = QColor("#94a3b8") # Grey default
            
            if log.get('is_sunk'):
                ship_type = log.get('sunk_ship_type', 0)
                ship_map = {1: "Patrol", 2: "Submarine", 3: "Destroyer", 4: "Battleship", 5: "Carrier"}
                ship_name = ship_map.get(ship_type, "Ship")
                res_str = f"SUNK {ship_name}"
                res_color = QColor("#facc15") # Yellow
            elif log.get('is_hit'):
                res_str = "HIT"
                res_color = QColor("#ef4444") # Red
            else:
                res_str = "MISS"
            
            res_item = QTableWidgetItem(res_str)
            res_item.setForeground(res_color)
            res_item.setFont(self.font_bold)
            res_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            table.setItem(i, 3, res_item)
            
            # 5. Time (Format HH:MM:SS)
            ts = log.get('timestamp', 0)
            if ts > 0:
                try:
                    # Nếu timestamp là milliseconds
                    t_obj = datetime.fromtimestamp(ts / 1000)
                    time_str = t_obj.strftime("%H:%M:%S")
                except:
                    time_str = "Err"
            else:
                time_str = "-"
            
            time_item = QTableWidgetItem(time_str)
            time_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            table.setItem(i, 4, time_item)

        layout.addWidget(table)
        return tab

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