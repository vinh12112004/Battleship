from PyQt6.QtWidgets import QDialog, QVBoxLayout, QHBoxLayout, QLabel, QPushButton
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont
from ..core.protocol import MessageType, TCPMessage
from ..utils.logger import logger
from ..utils.constants import COLORS
import time

class ChallengeDialog(QDialog):
    """Dialog for incoming challenge"""
    
    def __init__(self, challenger, challenge_id, expires_at, tcp_client):
        super().__init__()
        self.challenger = challenger
        self.challenge_id = challenge_id
        self.expires_at = expires_at
        self.tcp_client = tcp_client
        
        self.setWindowTitle("⚔️ Challenge Received")
        self.setModal(True)
        self.setFixedSize(400, 300)
        
        self.init_ui()
        
        # Countdown timer
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_countdown)
        self.timer.start(1000)
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout(self)
        layout.setSpacing(20)
        layout.setContentsMargins(30, 30, 30, 30)
        
        self.setStyleSheet(f"""
            QDialog {{
                background-color: {COLORS['card']};
                border: 3px solid {COLORS['primary']};
                border-radius: 12px;
            }}
        """)
        
        # Title
        title = QLabel("⚔️ CHALLENGE RECEIVED")
        title.setFont(QFont("Arial", 20, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet(f"color: {COLORS['primary']};")
        layout.addWidget(title)
        
        # Challenger name
        challenger_label = QLabel(f"from {self.challenger}")
        challenger_label.setFont(QFont("Arial", 16))
        challenger_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        challenger_label.setStyleSheet(f"color: {COLORS['accent']};")
        layout.addWidget(challenger_label)
        
        # Countdown
        self.countdown_label = QLabel("60s")
        self.countdown_label.setFont(QFont("Arial", 24, QFont.Weight.Bold))
        self.countdown_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.countdown_label.setStyleSheet(f"color: {COLORS['warning']};")
        layout.addWidget(self.countdown_label)
        
        layout.addStretch()
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.setSpacing(15)
        
        accept_btn = QPushButton("✅ ACCEPT")
        accept_btn.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        accept_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['success']};
                color: white;
                border: none;
                border-radius: 8px;
                padding: 15px;
            }}
            QPushButton:hover {{
                background-color: #00e094;
            }}
        """)
        accept_btn.clicked.connect(self.accept_challenge)
        button_layout.addWidget(accept_btn)
        
        decline_btn = QPushButton("❌ DECLINE")
        decline_btn.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        decline_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['error']};
                color: white;
                border: none;
                border-radius: 8px;
                padding: 15px;
            }}
            QPushButton:hover {{
                background-color: #ff6b6b;
            }}
        """)
        decline_btn.clicked.connect(self.decline_challenge)
        button_layout.addWidget(decline_btn)
        
        layout.addLayout(button_layout)
    
    def update_countdown(self):
        """Update countdown timer"""
        remaining = self.expires_at - int(time.time())
        
        if remaining <= 0:
            self.timer.stop()
            logger.warning("Challenge expired")
            self.close()
            return
        
        self.countdown_label.setText(f"{remaining}s")
        
        # Change color when time is running out
        if remaining <= 10:
            self.countdown_label.setStyleSheet(f"color: {COLORS['error']};")
    
    def accept_challenge(self):
        """Accept challenge"""
        msg = TCPMessage(
            type=MessageType.MSG_CHALLENGE_ACCEPT,
            payload={'challenge_id': self.challenge_id},
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            logger.info(f"Accepted challenge from {self.challenger}")
            self.timer.stop()
            self.close()
    
    def decline_challenge(self):
        """Decline challenge"""
        msg = TCPMessage(
            type=MessageType.MSG_CHALLENGE_DECLINE,
            payload={'challenge_id': self.challenge_id},
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            logger.info(f"Declined challenge from {self.challenger}")
            self.timer.stop()
            self.close()
    
    def closeEvent(self, event):
        """Handle dialog close"""
        self.timer.stop()
        event.accept()