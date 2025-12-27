import sys
import os
from PyQt6.QtWidgets import QApplication, QMessageBox
from src.core.bridge import TCPClientBridge
from src.ui.login_window import LoginWindow
from src.ui.dashboard_window import DashboardWindow
from src.ui.ship_placement_window import ShipPlacementWindow
from src.ui.game_window import GameWindow
from src.utils.logger import logger
from src.utils.constants import DEFAULT_HOST, DEFAULT_PORT

class BattleshipApp:
    """Main application controller"""
    
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.app.setApplicationName("Battleship")
        self.app.setOrganizationName("BattleshipGame")
        
        self.tcp_client = None
        # Biến này cực kỳ quan trọng để giữ cửa sổ không bị GC xóa
        self.current_window = None 
        
        self.username = None
        self.current_game_id = None

    def get_library_path(self):
        """Lấy đường dẫn tuyệt đối tới file .so"""
        base_path = os.path.dirname(os.path.abspath(__file__))
        # Giả sử cấu trúc: root/main.py và root/src/client/libtcp_client.so
        lib_path = os.path.join(base_path, "src", "client", "libtcp_client.so")
        return lib_path
    
    def run(self):
        """Run application"""
        lib_path = self.get_library_path()
        if not os.path.exists(lib_path):
            QMessageBox.critical(None, "Error", f"Library not found at: {lib_path}")
            return 1

        # Create TCP client
        self.tcp_client = TCPClientBridge(lib_path)
        
        # Connect to server
        logger.info(f"Connecting to {DEFAULT_HOST}:{DEFAULT_PORT}...")
        if not self.tcp_client.connect(DEFAULT_HOST, DEFAULT_PORT):
            QMessageBox.critical(None, "Connection Error",
                                f"Failed to connect to server at {DEFAULT_HOST}:{DEFAULT_PORT}\n\n"
                                "Please make sure the server is running.")
            return 1
        
        # Show login window
        self.switch_to_window(LoginWindow(self.tcp_client))
        
        # Kết nối tín hiệu cho Login Window
        # Lưu ý: Vì LoginWindow nằm trong self.current_window, ta truy cập nó
        self.current_window.login_success.connect(self.on_login_success)
        
        # Run application
        exit_code = self.app.exec()
        
        # Cleanup
        self.tcp_client.disconnect()
        return exit_code
    
    def switch_to_window(self, new_window):
        """Hàm trung gian để chuyển đổi cửa sổ an toàn"""
        if self.current_window:
            self.current_window.close()
            # Tùy chọn: Xóa tham chiếu cũ
            self.current_window.deleteLater() 
        
        self.current_window = new_window
        self.current_window.show()

    def on_login_success(self, username, token):
        """Handle successful login"""
        self.username = username
        self.tcp_client.token = token
        logger.info(f"Logged in as {username}")
        
        # Tạo Dashboard và chuyển cảnh
        dashboard = DashboardWindow(self.tcp_client, self.username)
        dashboard.start_game.connect(self.on_game_start)
        self.switch_to_window(dashboard)
    
    def on_game_start(self, game_id, opponent):
        """Handle game start"""
        logger.info(f"Starting game: {game_id} vs {opponent}")
        self.current_game_id = game_id
        
        # Chuyển sang màn hình xếp tàu
        placement_window = ShipPlacementWindow(self.tcp_client, self.username, game_id)
        placement_window.placement_complete.connect(lambda: self.show_game(opponent))
        self.switch_to_window(placement_window)
    
    def show_game(self, opponent):
        """Show game window"""
        # Chuyển sang màn hình chơi game chính
        game_window = GameWindow(self.tcp_client, self.username, self.current_game_id, opponent)
        self.switch_to_window(game_window)

def main():
    """Main entry point"""
    # Đảm bảo đường dẫn import đúng nếu chạy từ root
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    
    app = BattleshipApp()
    sys.exit(app.run())

if __name__ == "__main__":
    main()