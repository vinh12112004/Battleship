from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                              QLineEdit, QPushButton, QMessageBox)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont, QPalette, QColor
from ..core.protocol import MessageType, TCPMessage
from ..utils.logger import logger
from ..utils.constants import COLORS

class LoginWindow(QWidget):
    """Login/Register window"""
    sig_auth_success = pyqtSignal(dict)
    sig_auth_failed = pyqtSignal(dict)
    login_success = pyqtSignal(str, str)  # username, token
    
    def __init__(self, tcp_client):
        super().__init__()
        self.tcp_client = tcp_client
        self.token = None
        
        self.init_ui()
        self.setup_handlers()
        self.sig_auth_success.connect(self.handle_auth_success_ui)
        self.sig_auth_failed.connect(self.handle_auth_failed_ui)
        
    def init_ui(self):
        """Initialize UI components"""
        self.setWindowTitle("⚓ Battleship - Login")
        self.setGeometry(100, 100, 450, 550)
        self.apply_dark_theme()
        
        # Main layout
        main_layout = QVBoxLayout(self)
        main_layout.setSpacing(20)
        main_layout.setContentsMargins(40, 40, 40, 40)
        
        # Title
        title = QLabel("⚓ BATTLESHIP")
        title.setFont(QFont("Arial", 36, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet(f"color: {COLORS['primary']}; margin-bottom: 10px;")
        main_layout.addWidget(title)
        
        subtitle = QLabel("Naval Warfare Command System")
        subtitle.setFont(QFont("Arial", 11))
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        subtitle.setStyleSheet(f"color: {COLORS['accent']}; margin-bottom: 30px;")
        main_layout.addWidget(subtitle)
        
        # Username field
        username_label = QLabel("Username")
        username_label.setStyleSheet(f"color: {COLORS['primary']}; font-size: 13px; font-weight: bold;")
        main_layout.addWidget(username_label)
        
        self.username_input = QLineEdit()
        self.username_input.setPlaceholderText("Enter your username")
        self.username_input.setStyleSheet(f"""
            QLineEdit {{
                background-color: {COLORS['card']};
                border: 2px solid {COLORS['border']};
                border-radius: 8px;
                padding: 12px;
                color: {COLORS['foreground']};
                font-size: 14px;
            }}
            QLineEdit:focus {{
                border-color: {COLORS['primary']};
            }}
        """)
        main_layout.addWidget(self.username_input)
        
        # Password field
        password_label = QLabel("Password")
        password_label.setStyleSheet(f"color: {COLORS['primary']}; font-size: 13px; font-weight: bold;")
        main_layout.addWidget(password_label)
        
        self.password_input = QLineEdit()
        self.password_input.setPlaceholderText("Enter your password")
        self.password_input.setEchoMode(QLineEdit.EchoMode.Password)
        self.password_input.setStyleSheet(self.username_input.styleSheet())
        self.password_input.returnPressed.connect(self.login)
        main_layout.addWidget(self.password_input)
        
        # Status label
        self.status_label = QLabel("")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet(f"color: {COLORS['error']}; font-size: 12px; margin-top: 10px;")
        self.status_label.setWordWrap(True)
        main_layout.addWidget(self.status_label)
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.setSpacing(15)
        
        self.login_btn = QPushButton("LOGIN")
        self.login_btn.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        self.login_btn.setStyleSheet(f"""
            QPushButton {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 {COLORS['primary']}, stop:1 {COLORS['accent']});
                color: {COLORS['background']};
                border: none;
                border-radius: 8px;
                padding: 15px 30px;
            }}
            QPushButton:hover {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 {COLORS['primary_dark']}, stop:1 {COLORS['accent']});
            }}
            QPushButton:pressed {{
                background-color: {COLORS['primary_dark']};
            }}
        """)
        self.login_btn.clicked.connect(self.login)
        button_layout.addWidget(self.login_btn)
        
        self.register_btn = QPushButton("REGISTER")
        self.register_btn.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        self.register_btn.setStyleSheet(f"""
            QPushButton {{
                background-color: transparent;
                color: {COLORS['primary']};
                border: 2px solid {COLORS['primary']};
                border-radius: 8px;
                padding: 15px 30px;
            }}
            QPushButton:hover {{
                background-color: {COLORS['primary']};
                color: {COLORS['background']};
            }}
        """)
        self.register_btn.clicked.connect(self.register)
        button_layout.addWidget(self.register_btn)
        
        main_layout.addLayout(button_layout)
        
        # Add stretch to push everything up
        main_layout.addStretch()
        
        # Connection info
        conn_label = QLabel(f"Server: {self.tcp_client.host if hasattr(self.tcp_client, 'host') else 'localhost'}:{self.tcp_client.port if hasattr(self.tcp_client, 'port') else 9090}")
        conn_label.setStyleSheet(f"color: {COLORS['border']}; font-size: 10px;")
        conn_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        main_layout.addWidget(conn_label)
    
    def setup_handlers(self):
        """Setup message handlers"""
        self.tcp_client.on_message(MessageType.MSG_AUTH_SUCCESS, self.on_auth_success)
        self.tcp_client.on_message(MessageType.MSG_AUTH_FAILED, self.on_auth_failed)
    
    def login(self):
        """Handle login button click"""
        username = self.username_input.text().strip()
        password = self.password_input.text().strip()
        
        if not username or not password:
            self.show_error("Please enter username and password")
            return
        
        if not self.tcp_client.connected:
            self.show_error("Not connected to server")
            return
        
        self.status_label.setText("Authenticating...")
        self.status_label.setStyleSheet(f"color: {COLORS['warning']};")
        
        # Send login message
        msg = TCPMessage(
            type=MessageType.MSG_LOGIN,
            payload={
                'username': username,
                'password': password
            }
        )
        
        if not self.tcp_client.send_message(msg):
            self.show_error("Failed to send login request")
    
    def register(self):
        """Handle register button click"""
        username = self.username_input.text().strip()
        password = self.password_input.text().strip()
        
        if not username or not password:
            self.show_error("Please enter username and password")
            return
        
        if len(username) < 3:
            self.show_error("Username must be at least 3 characters")
            return
        
        if len(password) < 6:
            self.show_error("Password must be at least 6 characters")
            return
        
        if not self.tcp_client.connected:
            self.show_error("Not connected to server")
            return
        
        self.status_label.setText("Creating account...")
        self.status_label.setStyleSheet(f"color: {COLORS['warning']};")
        
        # Send register message
        msg = TCPMessage(
            type=MessageType.MSG_REGISTER,
            payload={
                'username': username,
                'password': password
            }
        )
        
        if self.tcp_client.send_message(msg):
            QMessageBox.information(self, "Success", 
                                   "Account created! Please login.")
            self.status_label.setText("")
        else:
            self.show_error("Failed to send register request")
    
    def on_auth_success(self, payload):
        """Handle successful authentication"""
        token = payload.get('token', '')
        username = payload.get('username', '')
        
        logger.info(f"Login successful: {username}")
        
        self.token = token
        self.login_success.emit(username, token)
        self.close()
    
    def on_auth_failed(self, payload):
        """Handle failed authentication"""
        reason = payload.get('reason', 'Unknown error')
        logger.warning(f"Login failed: {reason}")
        self.show_error(f"Authentication failed: {reason}")
    
    def show_error(self, message: str):
        """Show error message"""
        self.status_label.setText(message)
        self.status_label.setStyleSheet(f"color: {COLORS['error']};")
    
    def apply_dark_theme(self):
        """Apply dark theme"""
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(COLORS['background']))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(COLORS['foreground']))
        self.setPalette(palette)
        
    def handle_auth_success_ui(self, payload):
        token = payload.get('token', '')
        username = payload.get('username', '')
        logger.info(f"Login successful: {username}")
        
        self.token = token
        self.login_success.emit(username, token)
        self.close() # An toàn vì đang ở Main Thread

    def handle_auth_failed_ui(self, payload):
        reason = payload.get('reason', 'Unknown error')
        self.show_error(f"Authentication failed: {reason}")