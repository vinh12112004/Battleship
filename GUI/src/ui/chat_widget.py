from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                              QTextEdit, QLineEdit, QPushButton)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont
from ..core.protocol import MessageType, TCPMessage
from ..utils.logger import logger
from ..utils.constants import COLORS

class ChatWidget(QWidget):
    """Chat widget for in-game messaging"""
    
    # 1. Khai báo Signal để giao tiếp luồng an toàn
    sig_chat_received = pyqtSignal(dict)
    
    def __init__(self, tcp_client, game_id):
        super().__init__()
        self.tcp_client = tcp_client
        self.game_id = game_id
        
        self.init_ui()
        
        # 2. Kết nối Signal vào hàm xử lý UI
        self.sig_chat_received.connect(self.handle_chat_ui)
        self.setup_handlers()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout(self)
        layout.setSpacing(10)
        
        # Container with border
        container = QWidget()
        container.setStyleSheet(f"""
            background-color: {COLORS['card']};
            border: 2px solid {COLORS['border']};
            border-radius: 8px;
        """)
        container_layout = QVBoxLayout(container)
        container_layout.setContentsMargins(10, 10, 10, 10)
        
        # Title
        title = QLabel("💬 Game Chat")
        title.setFont(QFont("Arial", 14, QFont.Weight.Bold))
        title.setStyleSheet(f"color: {COLORS['primary']};")
        container_layout.addWidget(title)
        
        # Messages display
        self.messages_display = QTextEdit()
        self.messages_display.setReadOnly(True)
        self.messages_display.setStyleSheet(f"""
            QTextEdit {{
                background-color: {COLORS['background']};
                color: {COLORS['foreground']};
                border: 1px solid {COLORS['border']};
                border-radius: 4px;
                padding: 5px;
                font-family: sans-serif;
                font-size: 13px;
            }}
        """)
        container_layout.addWidget(self.messages_display)
        
        # Input area
        input_layout = QHBoxLayout()
        input_layout.setSpacing(5)
        
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
                padding: 8px 15px;
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
    
    def setup_handlers(self):
        """Setup message handlers"""
        # Lưu tham chiếu hàm emit để dùng cho việc hủy đăng ký sau này
        self._chat_handler = self.sig_chat_received.emit
        self.tcp_client.on_message(MessageType.MSG_CHAT_MESSAGE, self._chat_handler)
    
    def closeEvent(self, event):
        """Dọn dẹp khi widget bị đóng"""
        try:
            self.tcp_client.off_message(MessageType.MSG_CHAT_MESSAGE, self._chat_handler)
        except Exception as e:
            logger.error(f"Error cleaning up ChatWidget: {e}")
        event.accept()

    def send_message(self):
        """Send chat message"""
        text = self.message_input.text().strip()
        
        if not text:
            return
        
        msg = TCPMessage(
            type=MessageType.MSG_CHAT,
            payload={
                'game_id': self.game_id,
                'message': text
            },
            token=self.tcp_client.token
        )
        
        if self.tcp_client.send_message(msg):
            # Hiển thị tin nhắn của chính mình ngay lập tức
            self.add_message("You", text, is_own=True)
            self.message_input.clear()
            logger.debug(f"Sent chat message: {text}")
    
    def handle_chat_ui(self, payload):
        """Handle incoming chat message (Chạy trên Main Thread)"""
        username = payload.get('username', 'Unknown')
        text = payload.get('text', '')
        
        # Chỉ hiển thị tin nhắn từ người khác (tin của mình đã hiện lúc gửi)
        # Tuy nhiên nếu server echo lại tin nhắn của mình thì cần check username
        # Ở đây tạm thời cứ hiển thị, hàm add_message sẽ lo việc format
        if username != "You": # Đơn giản hóa, thực tế nên so sánh ID hoặc username login
             self.add_message(username, text, is_own=False)
        
        logger.debug(f"Received chat: {username}: {text}")
    
    def add_message(self, username, text, is_own=False):
        """Add message to display"""
        if is_own:
            color = COLORS['primary']
            prefix = "You"
        else:
            color = COLORS['accent']
            prefix = username
        
        # Format HTML đơn giản
        html = f'<p style="margin: 5px 0;"><span style="color: {color}; font-weight: bold;">{prefix}:</span> {text}</p>'
        self.messages_display.append(html)
        
        # Auto-scroll to bottom
        scrollbar = self.messages_display.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())