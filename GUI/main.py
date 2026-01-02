import sys
import os
from PyQt6.QtWidgets import QApplication, QMessageBox
from PyQt6.QtCore import QTimer
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
        self.current_opponent = None
        self.current_turn = None

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
        
        self.current_window = new_window
        self.current_window.show()

    def on_login_success(self, username, token):
        """Handle successful login"""
        self.username = username
        self.tcp_client.token = token
        if self.tcp_client:
            self.tcp_client.token = token
            logger.info(f"Token saved to TCP Client: {token[:15]}...")
            
        logger.info(f"Logged in as {username}")
        
        # Tạo Dashboard và chuyển cảnh
        dashboard = DashboardWindow(self.tcp_client, self.username)
        dashboard.start_game_signal.connect(self.on_game_start)
        self.switch_to_window(dashboard)
    
    def on_game_start(self, game_id, opponent):
        """Handle game start"""
        logger.info(f"Starting game: {game_id} vs {opponent}")
        self.current_game_id = game_id
        self.current_opponent = opponent
        
        # Chuyển sang màn hình xếp tàu
        placement_window = ShipPlacementWindow(self.tcp_client, self.username, game_id)
        placement_window.placement_complete.connect(self.on_placement_complete)
        self.switch_to_window(placement_window)
        
    def on_placement_complete(self, opponent: str, current_turn: str):
        logger.info("[Main] Both players ready! Switching to GameWindow...")
        logger.info(f"  Opponent: {opponent}")
        logger.info(f"  Current Turn: {current_turn}")
        
        # ✅ Lưu opponent và current_turn
        self.current_opponent = opponent
        self.current_turn = current_turn
        
        # Delay 200ms để cleanup
        QTimer.singleShot(200, self._create_game_window)
        
    def _create_game_window(self):
        """✅ Tạo GameWindow sau khi cleanup xong"""
        opponent = self.current_opponent
        current_turn = self.current_turn
        
        # Get saved board from TCP client
        board_state = None
        if hasattr(self.tcp_client, 'game_boards') and self.current_game_id in self.tcp_client.game_boards:
            board_state = self.tcp_client.game_boards[self.current_game_id]
            logger.info(f"[Main] Retrieved board for game {self.current_game_id}: {len(board_state)} cells")
        else:
            logger.warning(f"[Main] No board found for game {self.current_game_id}")
        
        # Chuyển sang màn hình chơi game chính
        game_window = GameWindow(
            self.tcp_client, 
            self.username, 
            self.current_game_id, 
            opponent,
            current_turn,
            board_state=board_state 
        )
        
        # Set board state vào GameWindow
        if board_state:
            game_window.game_state.your_board = board_state
            logger.info(f"[Main] Set your_board to GameWindow")
        
        self.switch_to_window(game_window)
    
    def show_game(self, opponent):
        """Show game window"""
        # ✅ Get saved board from TCP client
        board_state = None
        if hasattr(self.tcp_client, 'game_boards') and self.current_game_id in self.tcp_client.game_boards:
            board_state = self.tcp_client.game_boards[self.current_game_id]
            logger.info(f"[Main] Retrieved board for game {self.current_game_id}: {len(board_state)} cells")
        else:
            logger.warning(f"[Main] No board found for game {self.current_game_id}")
        
        # Chuyển sang màn hình chơi game chính
        game_window = GameWindow(self.tcp_client, self.username, self.current_game_id, opponent)
        
        # ✅ Set board state vào GameWindow
        if board_state:
            game_window.game_state.your_board = board_state
            logger.info(f"[Main] Set your_board to GameWindow")
        
        self.switch_to_window(game_window)

def main():
    """Main entry point"""
    # Đảm bảo đường dẫn import đúng nếu chạy từ root
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    
    app = BattleshipApp()
    sys.exit(app.run())

if __name__ == "__main__":
    main()