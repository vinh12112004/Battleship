from PyQt6.QtGui import QIcon
from PyQt6.QtCore import QSize

ICON_SIZE = QSize(32, 32)

class IconManager:
    SHIPS = {
        5: QIcon("assets/ships/carrier.svg"),
        4: QIcon("assets/ships/battleship.svg"),
        3: QIcon("assets/ships/destroyer.svg"),
        2: QIcon("assets/ships/submarine.svg"),
        1: QIcon("assets/ships/patrol_boat.svg"),
    }

    HIT  = QIcon("assets/icons/hit.svg")
    MISS = QIcon("assets/icons/miss.svg")
