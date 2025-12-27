from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                              QPushButton, QScrollArea, QFrame)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont
from ..utils.constants import COLORS

class PlayerCard(QWidget):
    """Individual player card"""
    
    challenge_clicked = pyqtSignal(str, str)  # username, user_id
    
    def __init__(self, username, user_id, elo, rank):
        super().__init__()
        self.username = username
        self.user_id = user_id
        
        self.setStyleSheet(f"""
            QWidget {{
                background-color: {COLORS['card']};
                border: 2px solid {COLORS['border']};
                border-radius: 8px;
                padding: 10px;
            }}
            QWidget:hover {{
                border-color: {COLORS['primary']};
            }}
        """)
        
        layout = QHBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Player info
        info_layout = QVBoxLayout()
        
        name_label = QLabel(username)
        name_label.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        name_label.setStyleSheet(f"color: {COLORS['primary']};")
        info_layout.addWidget(name_label)
        
        stats_label = QLabel(f"ELO: {elo} | {rank}")
        stats_label.setStyleSheet(f"color: {COLORS['foreground']}; font-size: 11px;")
        info_layout.addWidget(stats_label)
        
        layout.addLayout(info_layout)
        layout.addStretch()
        
        # Challenge button
        challenge_btn = QPushButton("⚔️ Challenge")
        challenge_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['accent']};
                color: white;
                border: none;
                border-radius: 6px;
                padding: 8px 15px;
                font-weight: bold;
            }}
            QPushButton:hover {{
                background-color: {COLORS['accent_light']};
            }}
        """)
        challenge_btn.clicked.connect(lambda: self.challenge_clicked.emit(self.username, self.user_id))
        layout.addWidget(challenge_btn)

class OnlinePlayersWidget(QWidget):
    """Widget showing online players"""
    
    challenge_player = pyqtSignal(str, str)  # username, user_id
    
    def __init__(self, tcp_client):
        super().__init__()
        self.tcp_client = tcp_client
        self.player_cards = []
        
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout(self)
        layout.setSpacing(10)
        
        # Scroll area for players
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet(f"""
            QScrollArea {{
                background-color: {COLORS['background']};
                border: none;
            }}
        """)
        
        self.players_container = QWidget()
        self.players_layout = QVBoxLayout(self.players_container)
        self.players_layout.setSpacing(10)
        self.players_layout.addStretch()
        
        scroll.setWidget(self.players_container)
        layout.addWidget(scroll)
    
    def update_players(self, players):
        """Update players list"""
        # Clear existing cards
        for card in self.player_cards:
            card.deleteLater()
        self.player_cards.clear()
        
        # Remove stretch
        if self.players_layout.count() > 0:
            self.players_layout.takeAt(self.players_layout.count() - 1)
        
        # Add new cards
        for player in players:
            if player['username'] == self.tcp_client.username:
                continue  # Skip self
            
            card = PlayerCard(
                player['username'],
                player.get('user_id', ''),
                player.get('elo_rating', 1500),
                player.get('rank', 'Ensign')
            )
            card.challenge_clicked.connect(self.challenge_player.emit)
            
            self.players_layout.addWidget(card)
            self.player_cards.append(card)
        
        # Add stretch at end
        self.players_layout.addStretch()