import ctypes
import struct
import threading
import queue
import time
from typing import Callable, Dict, Optional
from .protocol import MessageType, TCPMessage
from ..utils.logger import logger

class TCPClientBridge:
    """Bridge between Python and C TCP client using ctypes"""
    
    def __init__(self, lib_path: str = "./src/client/libtcp_client.so"):
        # Load C library
        self.lib = ctypes.CDLL(lib_path)
        
        # Define C function signatures
        self.lib.tcp_connect.argtypes = [ctypes.c_char_p, ctypes.c_uint16]
        self.lib.tcp_connect.restype = ctypes.c_int
        
        self.lib.tcp_disconnect.argtypes = [ctypes.c_int]
        self.lib.tcp_disconnect.restype = ctypes.c_int
        
        # ✅ FIX: tcp_send nhận buffer, không phải byref
        self.lib.tcp_send.argtypes = [ctypes.c_int, ctypes.c_void_p]
        self.lib.tcp_send.restype = ctypes.c_int
        
        # ✅ FIX: tcp_recv nhận buffer, không phải byref
        self.lib.tcp_recv.argtypes = [ctypes.c_int, ctypes.c_void_p]
        self.lib.tcp_recv.restype = ctypes.c_int
        
        self.lib.tcp_set_nonblocking.argtypes = [ctypes.c_int]
        self.lib.tcp_set_nonblocking.restype = ctypes.c_int
        
        # State
        self.sockfd = -1
        self.connected = False
        self.running = False
        self.recv_thread: Optional[threading.Thread] = None
        self.ping_thread: Optional[threading.Thread] = None
        self.message_queue = queue.Queue()
        self.handlers: Dict[int, list] = {}
        self.token = ""
        self.lock = threading.Lock()
        self.on_message(MessageType.MSG_PONG, self._handle_pong)
        logger.info("TCP Client Bridge initialized")
    
    def connect(self, host: str = "10.147.20.5", port: int = 9090) -> bool:
        """Connect to TCP server"""
        try:
            self.sockfd = self.lib.tcp_connect(host.encode(), port)
            
            if self.sockfd < 0:
                logger.error(f"Failed to connect to {host}:{port}")
                return False
            
            # Set non-blocking mode
            if self.lib.tcp_set_nonblocking(self.sockfd) < 0:
                logger.warning("Failed to set non-blocking mode")
            
            self.connected = True
            
            # Start receive thread
            self.running = True
            self.recv_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.recv_thread.start()
            
            self.ping_thread = threading.Thread(target=self._ping_loop, daemon=True)
            self.ping_thread.start()
            
            logger.info(f"Connected to {host}:{port} (fd={self.sockfd})")
            return True
            
        except Exception as e:
            logger.error(f"Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from server"""
        self.running = False
        
        if self.recv_thread and self.recv_thread.is_alive():
            self.recv_thread.join(timeout=2.0)
            
        if self.ping_thread and self.ping_thread.is_alive():
            self.ping_thread.join(timeout=2.0)
        
        if self.sockfd >= 0:
            self.lib.tcp_disconnect(self.sockfd)
            self.sockfd = -1
        
        self.connected = False
        logger.info("Disconnected from server")
    
    def send_message(self, msg: TCPMessage) -> bool:
        """Send message to server"""
        if not self.connected or self.sockfd < 0:
            logger.error("Not connected")
            return False
        
        try:
            # Serialize message to bytes
            data = msg.serialize()
            
            # ✅ FIX: Tạo buffer và truyền thẳng địa chỉ
            buffer = ctypes.create_string_buffer(data, len(data))
            
            # ✅ ĐÚNG: Truyền buffer trực tiếp (ctypes tự động convert sang void*)
            result = self.lib.tcp_send(self.sockfd, buffer)
            
            if result < 0:
                logger.error(f"Failed to send message type={msg.type}")
                return False
            
            logger.debug(f"Sent message: type={msg.type.name}")
            return True
            
        except Exception as e:
            logger.error(f"Send error: {e}")
            return False
        
    def _ping_loop(self):
        """Background thread to send PING every 30 seconds"""
        logger.info("Ping thread started")
        
        while self.running: # Bỏ check self.connected ở vòng lặp ngoài để thread không chết hẳn nếu rớt mạng tạm thời
            try:
                time.sleep(30)  # Đợi 30 giây
                
                if not self.connected or self.sockfd < 0:
                    continue # Nếu chưa kết nối thì đợi tiếp, không break
                
                # ✅ FIX: Truyền thêm self.token và payload=None
                # Cấu trúc: TCPMessage(type, token, payload) hoặc dùng keyword arguments
                ping_msg = TCPMessage(
                    type=MessageType.MSG_PING, 
                    token=self.token,  # Nên gửi kèm token để server biết ai đang ping
                    payload=None       # BẮT BUỘC PHẢI CÓ
                )
                
                if self.send_message(ping_msg):
                    logger.debug("Sent PING to server")
                else:
                    logger.warning("Failed to send PING")
                    
            except Exception as e:
                logger.error(f"Ping loop error: {e}")
                time.sleep(1) # Nghỉ 1 chút nếu lỗi để tránh spam log
        
        logger.info("Ping thread stopped")
    
    def _receive_loop(self):
        """Background thread to receive messages"""
        logger.info("Receive thread started")

        buffer_size = TCPMessage.MESSAGE_SIZE 
        
        while self.running:
            try:
                # ✅ FIX: Tạo buffer MỚI mỗi lần để tránh data corruption
                buffer = ctypes.create_string_buffer(buffer_size)
                
                # ✅ ĐÚNG: Truyền buffer trực tiếp (không dùng byref)
                result = self.lib.tcp_recv(self.sockfd, buffer)
                
                if result == 0:
                    # Non-blocking: No data available
                    time.sleep(0.01)
                    continue
                
                if result < 0:
                    if result == -2:
                        logger.info("Server closed connection")
                        self.connected = False
                        break
                    time.sleep(0.1)
                    continue
                
                # ✅ Parse data từ buffer
                data = buffer.raw[:buffer_size]
                msg = TCPMessage.deserialize(data)
                
                if msg:
                    self._handle_message(msg)
                    
            except Exception as e:
                logger.error(f"Receive loop error: {e}")
                import traceback
                traceback.print_exc()
                time.sleep(0.1)
        
        logger.info("Receive thread stopped")
    
    def _handle_message(self, msg: TCPMessage):
        """Dispatch message to registered handlers"""
        try:
            msg_type = msg.type.value
            
            handlers_to_call = []
            with self.lock:
                if msg_type in self.handlers:
                    handlers_to_call = self.handlers[msg_type][:]
            
            for handler in handlers_to_call:
                try:
                    handler(msg.payload)
                except Exception as e:
                    logger.error(f"Handler error for type={msg.type.name}: {e}")
                    import traceback
                    traceback.print_exc()
        except Exception as e:
            logger.error(f"_handle_message error: {e}")
            import traceback
            traceback.print_exc()
    
    def on_message(self, msg_type: MessageType, handler: Callable):
        """Register message handler"""
        type_val = msg_type.value
        
        with self.lock:
            if type_val not in self.handlers:
                self.handlers[type_val] = []
            self.handlers[type_val].append(handler)
        
        logger.debug(f"Registered handler for {msg_type.name}")
    
    def off_message(self, msg_type: MessageType, handler: Callable):
        """Unregister message handler"""
        type_val = msg_type.value
        
        with self.lock:
            if type_val in self.handlers:
                if handler in self.handlers[type_val]:
                    self.handlers[type_val].remove(handler)
                    logger.debug(f"Unregistered handler for {msg_type.name}")
                else:
                    logger.warning(f"Handler not found for {msg_type.name} to remove")
                    
    def _handle_pong(self, payload):
        logger.debug("✅ Received PONG. Network OK.")