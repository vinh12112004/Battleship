from PyQt6.QtWidgets import QFrame, QLabel, QGridLayout, QVBoxLayout
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QFont
from ..utils.icon_loader import IconManager
from ..utils.constants import SHIP_TYPES, COLORS


class ReplayBoard(QFrame):
    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        self.cells = {}
        self.init_ui(title)

    def init_ui(self, title):
        layout = QVBoxLayout(self)
        layout.setSpacing(8)

        title_lbl = QLabel(title)
        title_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title_lbl.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        layout.addWidget(title_lbl)

        grid = QGridLayout()
        grid.setSpacing(2)

        for r in range(10):
            for c in range(10):
                cell = QLabel()
                cell.setFixedSize(32, 32)
                cell.setAlignment(Qt.AlignmentFlag.AlignCenter)
                cell.setScaledContents(True)
                cell.setStyleSheet("""
                    background-color: #0f172a;
                    border: 1px solid #334155;
                """)
                grid.addWidget(cell, r, c)
                self.cells[(r, c)] = cell

        layout.addLayout(grid)

    # ===== HIT / MISS =====

    def mark_hit(self, r, c):
        self.cells[(r, c)].setStyleSheet(
            "background-color:#ef4444; border:1px solid #fb7185;"
        )

    def mark_miss(self, r, c):
        self.cells[(r, c)].setStyleSheet(
            "background-color:#475569; border:1px solid #64748b;"
        )

    def mark_sunk(self, r, c):
        self.cells[(r, c)].setStyleSheet(
            "background-color:#facc15; border:1px solid #fde047;"
        )

    def reset(self):
        for cell in self.cells.values():
            cell.clear()
            cell.setStyleSheet(
                "background-color:#0f172a; border:1px solid #334155;"
            )

    # ===== DRAW SHIPS – FIX ĐÚNG ICON =====

    def draw_ships(self, ships):
        for ship_info in ships:
            ship_type = ship_info.get("type")      # ✅ SỐ
            start_row = ship_info.get("start_row")
            start_col = ship_info.get("start_col")
            is_horizontal = ship_info.get("is_horizontal", True)

            if not ship_type:
                continue

            size = ship_type   # ✅ QUAN TRỌNG: type == size

            icon = IconManager.SHIPS.get(ship_type)
            if not icon:
                continue

            for i in range(size):
                r = start_row + (0 if is_horizontal else i)
                c = start_col + (i if is_horizontal else 0)

                cell = self.cells.get((r, c))
                if not cell:
                    continue

                cell.setPixmap(icon.pixmap(28, 28))
                cell.setAlignment(Qt.AlignmentFlag.AlignCenter)
                cell.setStyleSheet("""
                    background-color: #3A6EA5;
                    border: 1px solid #1E3A5F;
                """)

