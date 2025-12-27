import struct
from enum import IntEnum
from typing import Dict, Any, Optional
from dataclasses import dataclass

class MessageType(IntEnum):
    MSG_REGISTER = 1
    MSG_LOGIN = 2
    MSG_AUTH_SUCCESS = 3
    MSG_AUTH_FAILED = 4
    MSG_JOIN_QUEUE = 5
    MSG_LEAVE_QUEUE = 6
    MSG_START_GAME = 7
    MSG_PLAYER_MOVE = 8
    MSG_MOVE_RESULT = 9
    MSG_GAME_OVER = 10
    MSG_CHAT = 11
    MSG_LOGOUT = 12
    MSG_PING = 13
    MSG_PONG = 14
    MSG_PLACE_SHIP = 15
    MSG_PLAYER_READY = 16
    MSG_GET_ONLINE_PLAYERS = 17
    MSG_ONLINE_PLAYERS_LIST = 18
    MSG_CHALLENGE_PLAYER = 19
    MSG_CHALLENGE_RECEIVED = 20
    MSG_CHALLENGE_ACCEPT = 21
    MSG_CHALLENGE_DECLINE = 22
    MSG_CHALLENGE_DECLINED = 23
    MSG_CHALLENGE_EXPIRED = 24
    MSG_CHALLENGE_CANCEL = 25
    MSG_CHALLENGE_CANCELLED = 26
    MSG_AUTH_TOKEN = 27
    MSG_TURN_WARNING = 28
    MSG_GAME_TIMEOUT = 29
    MSG_CHAT_MESSAGE = 30

@dataclass
class TCPMessage:
    """TCP message wrapper"""
    type: MessageType
    payload: Dict[str, Any]
    token: str = ""  # Token xác thực
    
    # Kích thước định nghĩa (Phải khớp với C header)
    MAX_JWT_LEN = 512
    MAX_PAYLOAD_SIZE = 5004
    MESSAGE_SIZE = 5520 # 4 (Type) + 512 (Token) + 5004 (Payload)
    
    def serialize(self) -> bytes:
        """
        Đóng gói dữ liệu thành Binary (Little Endian)
        Cấu trúc: [Type (4)] + [Token (512)] + [Union Payload (5004)]
        """
        # 1. Xử lý Payload (Dữ liệu thực tế)
        raw_payload = self._serialize_payload()
        
        if len(raw_payload) > self.MAX_PAYLOAD_SIZE:
            raise ValueError(f"Payload too large: {len(raw_payload)} > {self.MAX_PAYLOAD_SIZE}")
        
        # 2. Padding Payload: Chèn byte rỗng vào đuôi cho đủ 5004 bytes
        padded_payload = raw_payload.ljust(self.MAX_PAYLOAD_SIZE, b'\x00')
        
        # 3. Xử lý Token (512 bytes)
        token_bytes = self.token.encode('utf-8')[:self.MAX_JWT_LEN]
        padded_token = token_bytes.ljust(self.MAX_JWT_LEN, b'\x00')
        
        # 4. Đóng gói Header + Token
        # '<i' = Little-endian signed int (4 bytes) cho msg_type
        # Lưu ý: file C dùng int32_t nên dùng 'i', nếu uint32_t dùng 'I'
        header_part = struct.pack('<i', self.type.value) + padded_token
        
        # 5. Ghép toàn bộ
        return header_part + padded_payload
    
    @staticmethod
    def deserialize(data: bytes) -> Optional['TCPMessage']:
        """
        Giải mã Binary nhận từ Server (Little Endian)
        """
        if len(data) != TCPMessage.MESSAGE_SIZE:
            # Nếu kích thước không khớp 5520 bytes -> Bỏ qua
            return None
            
        try:
            # 1. Tách Header (4 bytes Type + 512 bytes Token)
            header_size = 4 + TCPMessage.MAX_JWT_LEN
            header_part = data[:header_size]
            payload_part = data[header_size:] # Phần còn lại là Payload (5004 bytes)
            
            # Unpack Type ('<i' cho Little Endian int)
            msg_type_val = struct.unpack('<i', header_part[:4])[0]
            
            # Unpack Token
            token_raw = header_part[4:]
            token = token_raw.decode('utf-8', errors='ignore').rstrip('\x00')
            
            try:
                msg_type_enum = MessageType(msg_type_val)
            except ValueError:
                # Type không tồn tại trong Enum
                return None 
            
            # 2. Parse Payload
            payload = TCPMessage._parse_payload(msg_type_enum, payload_part)
            
            return TCPMessage(type=msg_type_enum, payload=payload, token=token)
            
        except Exception as e:
            print(f"Deserialize error: {e}")
            return None
    
    def _serialize_payload(self) -> bytes:
        """Serialize payload based on message type (Little Endian)"""
        p = self.payload
        
        # AUTH messages
        if self.type in (MessageType.MSG_REGISTER, MessageType.MSG_LOGIN):
            username = p.get('username', '').encode('utf-8')[:31] + b'\x00'
            password = p.get('password', '').encode('utf-8')[:31] + b'\x00'
            return username.ljust(32, b'\x00') + password.ljust(32, b'\x00')
        
        # PLAYER_MOVE
        elif self.type == MessageType.MSG_PLAYER_MOVE:
            game_id = p.get('game_id', '').encode('utf-8')[:64] + b'\x00'
            row = p.get('row', 0)
            col = p.get('col', 0)
            # '<ii' = Little Endian 2 integers
            return game_id.ljust(65, b'\x00') + struct.pack('<ii', row, col)
        
        # CHAT
        elif self.type == MessageType.MSG_CHAT:
            game_id = p.get('game_id', '').encode('utf-8')[:63] + b'\x00'
            message = p.get('message', '').encode('utf-8')[:127] + b'\x00'
            return game_id.ljust(64, b'\x00') + message.ljust(128, b'\x00')
        
        # PLACE_SHIP
        elif self.type == MessageType.MSG_PLACE_SHIP:
            ship_type = p.get('ship_type', 0)
            row = p.get('row', 0)
            col = p.get('col', 0)
            is_horizontal = p.get('is_horizontal', 0)
            # '<iiiB3x': Little Endian int, int, int, unsigned char, 3 pad
            return struct.pack('<iiiB3x', ship_type, row, col, is_horizontal)
        
        # PLAYER_READY
        elif self.type == MessageType.MSG_PLAYER_READY:
            game_id = p.get('game_id', '').encode('utf-8')[:64] + b'\x00'
            board_state = bytes(p.get('board_state', [0]*100))
            return game_id.ljust(65, b'\x00') + board_state
        
        # CHALLENGE_PLAYER
        elif self.type == MessageType.MSG_CHALLENGE_PLAYER:
            target_id = p.get('target_id', '').encode('utf-8')[:63] + b'\x00'
            game_mode = p.get('game_mode', 'casual').encode('utf-8')[:31] + b'\x00'
            time_control = p.get('time_control', 10)
            
            return (b'\x00' * 64 +  # challenger_id (server fills)
                    target_id.ljust(64, b'\x00') +
                    b'\x00' * 65 +  # challenge_id (server generates)
                    game_mode.ljust(32, b'\x00') +
                    struct.pack('<i', time_control)) # Little Endian
        
        # CHALLENGE response
        elif self.type in (MessageType.MSG_CHALLENGE_ACCEPT, 
                          MessageType.MSG_CHALLENGE_DECLINE,
                          MessageType.MSG_CHALLENGE_CANCEL):
            challenge_id = p.get('challenge_id', '').encode('utf-8')[:64] + b'\x00'
            return challenge_id.ljust(65, b'\x00')
        
        # Empty payload messages
        elif self.type in (MessageType.MSG_GET_ONLINE_PLAYERS, 
                          MessageType.MSG_JOIN_QUEUE,
                          MessageType.MSG_LEAVE_QUEUE,
                          MessageType.MSG_PING,
                          MessageType.MSG_LOGOUT):
            return b''
        
        # AUTH_TOKEN (Server gửi về Client)
        elif self.type == MessageType.MSG_AUTH_TOKEN:
            token = p.get('token', '').encode('utf-8')[:511] + b'\x00'
            return token.ljust(512, b'\x00')
        
        else:
            return b''
    
    @staticmethod
    def _parse_payload(msg_type: MessageType, data: bytes) -> Dict[str, Any]:
        """Parse payload based on message type (Little Endian)"""
        
        def read_cstring(offset: int, max_len: int) -> str:
            """Read null-terminated C string"""
            chunk = data[offset : offset+max_len]
            end = chunk.find(b'\x00')
            if end != -1:
                return chunk[:end].decode('utf-8', errors='ignore')
            return chunk.decode('utf-8', errors='ignore')
        
        # AUTH_SUCCESS
        if msg_type == MessageType.MSG_AUTH_SUCCESS:
            username = read_cstring(0, 32)
            return {'username': username}
        
        # AUTH_FAILED
        elif msg_type == MessageType.MSG_AUTH_FAILED:
            reason = read_cstring(0, 64)
            return {'reason': reason}
        
        # START_GAME
        elif msg_type == MessageType.MSG_START_GAME:
            opponent = read_cstring(0, 32)
            game_id = read_cstring(32, 64)
            current_turn = read_cstring(96, 32)
            return {'opponent': opponent, 'game_id': game_id, 'current_turn': current_turn}
        
        # MOVE_RESULT
        elif msg_type == MessageType.MSG_MOVE_RESULT:
            # '<ii' = Little Endian ints
            row, col = struct.unpack('<ii', data[0:8])
            # '<BBI' = Little Endian unsigned char, unsigned char, unsigned int
            is_hit, is_sunk, sunk_ship_type = struct.unpack('<BBI', data[8:13])
            game_over, is_your_shot = struct.unpack('<BB', data[13:15])
            return {
                'row': row,
                'col': col,
                'is_hit': bool(is_hit),
                'is_sunk': bool(is_sunk),
                'sunk_ship_type': sunk_ship_type,
                'game_over': bool(game_over),
                'is_your_shot': bool(is_your_shot)
            }
        
        # CHAT_MESSAGE
        elif msg_type == MessageType.MSG_CHAT_MESSAGE:
            username = read_cstring(0, 64)
            text = read_cstring(64, 128)
            return {'username': username, 'text': text}
        
        # ONLINE_PLAYERS_LIST
        elif msg_type == MessageType.MSG_ONLINE_PLAYERS_LIST:
            # '<i' = Little Endian int
            count = struct.unpack('<i', data[0:4])[0]
            
            players = []
            offset = 4
            
            # Read usernames (50 * 64 bytes)
            usernames = []
            for i in range(50):
                username = read_cstring(offset, 64)
                usernames.append(username)
                offset += 64
            
            # Read elo_ratings (50 * 4 bytes)
            elos = []
            for i in range(50):
                elo = struct.unpack('<i', data[offset:offset+4])[0]
                elos.append(elo)
                offset += 4
            
            # Read ranks (50 * 32 bytes)
            ranks = []
            for i in range(50):
                rank = read_cstring(offset, 32)
                ranks.append(rank)
                offset += 32
            
            # Build player list
            for i in range(count):
                if i < 50: # Safety check
                    players.append({
                        'username': usernames[i],
                        'elo_rating': elos[i],
                        'rank': ranks[i]
                    })
            
            return {'count': count, 'players': players}
        
        # CHALLENGE_RECEIVED
        elif msg_type == MessageType.MSG_CHALLENGE_RECEIVED:
            challenger_username = read_cstring(0, 64)
            challenger_id = read_cstring(64, 64)
            challenge_id = read_cstring(128, 65)
            game_mode = read_cstring(193, 32)
            time_control = struct.unpack('<i', data[225:229])[0]
            expires_at = struct.unpack('<q', data[229:237])[0] # q for long long
            return {
                'challenger_username': challenger_username,
                'challenger_id': challenger_id,
                'challenge_id': challenge_id,
                'game_mode': game_mode,
                'time_control': time_control,
                'expires_at': expires_at
            }
        
        # CHALLENGE_DECLINED
        elif msg_type == MessageType.MSG_CHALLENGE_DECLINED:
            challenge_id = read_cstring(0, 65)
            return {'challenge_id': challenge_id}
        
        # TURN_WARNING
        elif msg_type == MessageType.MSG_TURN_WARNING:
            seconds_remaining = struct.unpack('<i', data[0:4])[0]
            return {'seconds_remaining': seconds_remaining}
        
        # GAME_TIMEOUT
        elif msg_type == MessageType.MSG_GAME_TIMEOUT:
            winner_id = read_cstring(0, 64)
            loser_id = read_cstring(64, 64)
            reason = read_cstring(128, 64)
            return {'winner_id': winner_id, 'loser_id': loser_id, 'reason': reason}
        
        else:
            return {}