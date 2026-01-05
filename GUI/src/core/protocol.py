import struct
import logging
from enum import IntEnum
from typing import Dict, Any, Optional
from dataclasses import dataclass

logger = logging.getLogger(__name__)

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
    MSG_GAME_RESULT = 31
    MSG_GAME_LOGS = 32
    MSG_RESIGN = 33
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
                logger.error(f"❌ INVALID MESSAGE TYPE: {msg_type_val}. Not in Enum!")
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
        
        elif self.type == MessageType.MSG_RESIGN:
            game_id = p.get('game_id', '').encode('utf-8')[:64] + b'\x00'
            return game_id.ljust(65, b'\x00') 


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
            token = read_cstring(0, 512)
            username = read_cstring(512, 32)
            return {'token': token, 'username': username}
        
        # AUTH_FAILED
        elif msg_type == MessageType.MSG_AUTH_FAILED:
            reason = read_cstring(0, 64)
            return {'reason': reason}
        
        # START_GAME
        elif msg_type == MessageType.MSG_START_GAME:
            opponent = read_cstring(0, 32)
            game_id = read_cstring(32, 64)
            current_turn = read_cstring(96, 32)
                
                # ✅ DEBUG
            logger.info(f"[Protocol] Parsed START_GAME:")
            logger.info(f"  opponent={opponent}")
            logger.info(f"  game_id={game_id}")
            logger.info(f"  current_turn={current_turn}")
                
            return {
                    'opponent': opponent, 
                    'game_id': game_id, 
                    'current_turn': current_turn
                }
        
        # MOVE_RESULT
        elif msg_type == MessageType.MSG_MOVE_RESULT:
            try:
                # Read fields individually to avoid alignment issues
                row = struct.unpack('<i', data[0:4])[0]
                col = struct.unpack('<i', data[4:8])[0]
                is_hit = data[8]
                is_sunk = data[9]
                sunk_ship_type = struct.unpack('<i', data[10:14])[0]
                game_over = data[14]
                is_your_shot = data[15]
                
                return {
                    'row': row,
                    'col': col,
                    'is_hit': bool(is_hit),
                    'is_sunk': bool(is_sunk),
                    'sunk_ship_type': sunk_ship_type,
                    'game_over': bool(game_over),
                    'is_your_shot': bool(is_your_shot)
                }
            except Exception as e:
                logger.error(f"Failed to parse MOVE_RESULT: {e}")
                logger.error(f"Data length: {len(data)}, first 20 bytes: {data[:20].hex()}")
                return {}
        
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
        
        elif msg_type == MessageType.MSG_GAME_OVER:
            winner = read_cstring(0, 64)
            loser = read_cstring(64, 64)
            reason = read_cstring(128, 128)
            
            logger.info(f"[Protocol] Parsed GAME_OVER:")
            logger.info(f"  winner={winner}")
            logger.info(f"  loser={loser}")
            logger.info(f"  reason={reason}")
            
            return {
                'winner': winner,
                'loser': loser,
                'reason': reason
            }
        elif msg_type == MessageType.MSG_GAME_RESULT:
            try:
                # 1. Đọc String (Cố định)
                # 0->65: game_id
                # 65->129: winner_id
                # 129->161: winner_username
                # 161->193: loser_username
                game_id = data[0:65].decode('utf-8').rstrip('\x00')
                winner_id = data[65:129].decode('utf-8').rstrip('\x00')
                winner_username = data[129:161].decode('utf-8').rstrip('\x00')
                loser_username = data[161:193].decode('utf-8').rstrip('\x00')
                
                # 2. Đọc Số (Offset bắt đầu từ 193)
                # Lưu ý: Các offset này phải cách nhau 4 byte (vì int32)
                
                total_turns = struct.unpack_from('<i', data, 193)[0]     # Offset 193
                game_duration = struct.unpack_from('<I', data, 197)[0]   # Offset 197 (Dùng 'I' vì là uint32)
                
                winner_old_elo = struct.unpack_from('<i', data, 201)[0]  # Offset 201
                winner_new_elo = struct.unpack_from('<i', data, 205)[0]  # Offset 205
                loser_old_elo = struct.unpack_from('<i', data, 209)[0]   # Offset 209
                loser_new_elo = struct.unpack_from('<i', data, 213)[0]   # Offset 213
                
                winner_hits = struct.unpack_from('<i', data, 217)[0]     # Offset 217
                winner_misses = struct.unpack_from('<i', data, 221)[0]   # Offset 221
                loser_hits = struct.unpack_from('<i', data, 225)[0]      # Offset 225
                loser_misses = struct.unpack_from('<i', data, 229)[0]    # Offset 229
                
                logger.info(f"Parsed GAME_RESULT: Winner={winner_username}, Duration={game_duration}")

                return {
                    'game_id': game_id,
                    'winner_id': winner_id,
                    'winner_username': winner_username,
                    'loser_username': loser_username,
                    'total_turns': total_turns,
                    'game_duration': game_duration,
                    'winner_old_elo': winner_old_elo,
                    'winner_new_elo': winner_new_elo,
                    'loser_old_elo': loser_old_elo,
                    'loser_new_elo': loser_new_elo,
                    'winner_hits': winner_hits,
                    'winner_misses': winner_misses,
                    'loser_hits': loser_hits,
                    'loser_misses': loser_misses
                }
            except Exception as e:
                logger.critical(f"❌ FAILED TO PARSE MSG_GAME_RESULT: {e}")
                return {}
        elif msg_type == MessageType.MSG_GAME_LOGS:
            try:
                offset = 0

                game_id = data[offset:offset+65].decode().rstrip('\x00')
                offset += 65

                player1_id = data[offset:offset+64].decode().rstrip('\x00')
                offset += 64
                player1_username = data[offset:offset+32].decode().rstrip('\x00')
                offset += 32

                player2_id = data[offset:offset+64].decode().rstrip('\x00')
                offset += 64
                player2_username = data[offset:offset+32].decode().rstrip('\x00')
                offset += 32

                chunk_index = struct.unpack_from('<i', data, offset)[0]
                offset += 4
                total_chunks = struct.unpack_from('<i', data, offset)[0]
                offset += 4
                log_count = struct.unpack_from('<i', data, offset)[0]
                offset += 4

                # ===== LOGS =====
                logs = []
                LOG_SIZE = 54
                MAX_LOGS = 50

                for _ in range(log_count):
                    log = {
                        "player_username": data[offset:offset+32].decode().rstrip('\x00'),
                        "row": struct.unpack_from('<i', data, offset+32)[0],
                        "col": struct.unpack_from('<i', data, offset+36)[0],
                        "is_hit": bool(data[offset+40]),
                        "is_sunk": bool(data[offset+41]),
                        "sunk_ship_type": struct.unpack_from('<i', data, offset+42)[0],
                        "turn_number": struct.unpack_from('<i', data, offset+46)[0],
                        "timestamp": struct.unpack_from('<I', data, offset+50)[0],
                    }
                    logs.append(log)
                    offset += LOG_SIZE

                # skip unused logs
                offset = (
                    65 + 64 + 32 + 64 + 32 + 12 +
                    MAX_LOGS * LOG_SIZE
                )

                # ===== PLAYER 1 SHIPS =====
                player1_ship_count = struct.unpack_from('<i', data, offset)[0]
                offset += 4

                player1_ships = []
                for _ in range(player1_ship_count):
                    player1_ships.append({
                        "type": struct.unpack_from('<i', data, offset)[0],
                        "start_row": struct.unpack_from('<i', data, offset+4)[0],
                        "start_col": struct.unpack_from('<i', data, offset+8)[0],
                        "is_horizontal": bool(data[offset+12]),
                    })
                    offset += 16

                # ===== PLAYER 2 SHIPS =====
                player2_ship_count = struct.unpack_from('<i', data, offset)[0]
                offset += 4

                player2_ships = []
                for _ in range(player2_ship_count):
                    player2_ships.append({
                        "type": struct.unpack_from('<i', data, offset)[0],
                        "start_row": struct.unpack_from('<i', data, offset+4)[0],
                        "start_col": struct.unpack_from('<i', data, offset+8)[0],
                        "is_horizontal": bool(data[offset+12]),
                    })
                    offset += 16

                return {
                    "game_id": game_id,

                    "player1_id": player1_id,
                    "player1_username": player1_username,
                    "player2_id": player2_id,
                    "player2_username": player2_username,

                    "chunk_index": chunk_index,
                    "total_chunks": total_chunks,
                    "logs": logs,

                    "player1_ships": player1_ships,
                    "player2_ships": player2_ships,
                }

            except Exception as e:
                logger.error(f"❌ Error parsing MSG_GAME_LOGS: {e}")
                return {}
