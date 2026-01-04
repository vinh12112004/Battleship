from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel,
    QTextEdit, QLineEdit, QPushButton
)
from PyQt6.QtCore import pyqtSignal
from PyQt6.QtGui import QFont

from ..core.protocol import MessageType, TCPMessage
from ..utils.logger import logger
from ..utils.constants import COLORS


class ChatWidget(QWidget):
    """Chat widget for in-game messaging"""

    # Signal để cập nhật UI an toàn từ thread mạng
    sig_chat_received = pyqtSignal(dict)

    def __init__(self, tcp_client, game_id):
        super().__init__()
        self.tcp_client = tcp_client
        self.game_id = game_id

        self.init_ui()

        # Kết nối signal UI
        self.sig_chat_received.connect(self.handle_chat_ui)

        # Đăng ký handler network
        self.setup_handlers()

    # ================= UI =================

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(10)

        container = QWidget()
        container.setStyleSheet(f"""
            background-color: {COLORS['card']};
            border: 2px solid {COLORS['border']};
            border-radius: 8px;
        """)

        container_layout = QVBoxLayout(container)
        container_layout.setContentsMargins(10, 10, 10, 10)
        container_layout.setSpacing(8)

        # Title
        title = QLabel("💬 Game Chat")
        title.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']};")
        container_layout.addWidget(title)

        # Message display
        self.messages_display = QTextEdit()
        self.messages_display.setReadOnly(True)
        self.messages_display.setStyleSheet(f"""
            QTextEdit {{
                background-color: {COLORS['background']};
                color: {COLORS['foreground']};
                border: 1px solid {COLORS['border']};
                border-radius: 4px;
                padding: 6px;
                font-size: 13px;
            }}
        """)
        container_layout.addWidget(self.messages_display)

        # Input area
        input_layout = QHBoxLayout()
        input_layout.setSpacing(6)

        self.message_input = QLineEdit()
        self.message_input.setPlaceholderText("Type a message...")
        self.message_input.setStyleSheet(f"""
            QLineEdit {{
                background-color: {COLORS['background']};
                color: {COLORS['foreground']};
                border: 1px solid {COLORS['border']};
                border-radius: 4px;
                padding: 8px;
            }}
            QLineEdit:focus {{
                border-color: {COLORS['primary']};
            }}
        """)
        self.message_input.returnPressed.connect(self.send_message)
        input_layout.addWidget(self.message_input)

        send_btn = QPushButton("Send")
        send_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {COLORS['primary']};
                color: {COLORS['background']};
                border: none;
                border-radius: 4px;
                padding: 8px 14px;
                font-weight: bold;
            }}
            QPushButton:hover {{
                background-color: {COLORS['primary_dark']};
            }}
        """)
        send_btn.clicked.connect(self.send_message)
        input_layout.addWidget(send_btn)

        container_layout.addLayout(input_layout)
        layout.addWidget(container)

    # ================= Network =================

    def setup_handlers(self):
        """
        Đăng ký handler nhận chat từ server
        Handler emit signal để đảm bảo UI chạy trên main thread
        """
        self._chat_handler = self.sig_chat_received.emit
        self.tcp_client.on_message(
            MessageType.MSG_CHAT_MESSAGE,
            self._chat_handler
        )

    def closeEvent(self, event):
        """Hủy đăng ký handler khi widget bị đóng"""
        try:
            self.tcp_client.off_message(
                MessageType.MSG_CHAT_MESSAGE,
                self._chat_handler
            )
        except Exception as e:
            logger.error(f"ChatWidget cleanup error: {e}")
        event.accept()

    # ================= Chat Logic =================

    def send_message(self):
        """Gửi tin nhắn – KHÔNG tự hiển thị"""
        text = self.message_input.text().strip()
        if not text:
            return

        msg = TCPMessage(
            type=MessageType.MSG_CHAT,
            payload={
                "game_id": self.game_id,
                "message": text
            },
            token=self.tcp_client.token
        )

        if self.tcp_client.send_message(msg):
            self.message_input.clear()
            logger.debug(f"Chat sent: {text}")

    def handle_chat_ui(self, payload: dict):
        """
        Nhận chat từ server (luôn hiển thị, kể cả tin của chính mình)
        """
        username = payload.get("username", "Unknown")
        text = payload.get("text", "")

        self.add_message(username, text)
        logger.debug(f"Chat received: {username}: {text}")

    def add_message(self, username: str, text: str):
        """Hiển thị tin nhắn lên UI"""
        html = f"""
        <p style="margin: 4px 0;">
            <span style="color: {COLORS['accent']}; font-weight: bold;">
                {username}:
            </span> {text}
        </p>
        """
        self.messages_display.append(html)

        # Auto scroll
        scrollbar = self.messages_display.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
