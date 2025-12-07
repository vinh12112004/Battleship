import { useState, useRef, useEffect } from "react";
import { wsService, MSG_TYPES } from "../../services/wsService";

export default function GameChat({ gameId }) {
    const [messages, setMessages] = useState([]);
    const [input, setInput] = useState("");
    const messagesEndRef = useRef(null);

    useEffect(() => {
        // ✅ Lắng nghe tin nhắn chat từ server
        const handleChatMessage = (payload) => {
            console.log("[GameChat] Received chat message:", payload);

            setMessages((prev) => [
                ...prev,
                {
                    userName: payload.username,
                    text: payload.text,
                    timestamp: Date.now(),
                },
            ]);
        };

        // Đăng ký handler
        wsService.onMessage(MSG_TYPES.CHAT_MESSAGE, handleChatMessage);

        // Cleanup khi component unmount
        return () => {
            wsService.offMessage(MSG_TYPES.CHAT_MESSAGE, handleChatMessage);
        };
    }, []);

    // Auto scroll to bottom khi có tin nhắn mới
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    }, [messages]);

    const handleSend = (e) => {
        e.preventDefault();

        if (!input.trim()) return;

        if (!gameId) {
            console.error("[GameChat] No game ID provided");
            return;
        }

        try {
            console.log("[GameChat] Sending message:", {
                type: MSG_TYPES.CHAT,
                gameId,
                message: input.trim(),
            });

            //  Gửi tin nhắn qua WebSocket
            wsService.sendMessage(MSG_TYPES.CHAT, {
                game_id: gameId,
                message: input.trim(),
            });

            setInput("");
        } catch (error) {
            console.error("[GameChat] Failed to send message:", error);
        }
    };

    return (
        <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-4 flex flex-col h-80">
            <h3 className="text-lg font-bold text-[#00d9ff] mb-3">Game Chat</h3>

            {/* Messages Container */}
            <div className="flex-1 overflow-y-auto space-y-2 mb-3 scrollbar-thin scrollbar-thumb-[#00d9ff] scrollbar-track-[#0f1419]">
                {messages.length === 0 ? (
                    <div className="text-gray-500 text-sm italic text-center mt-4">
                        No messages yet. Start chatting!
                    </div>
                ) : (
                    messages.map((msg, index) => (
                        <div
                            key={index}
                            className={`text-sm flex ${
                                msg.isOwn ? "justify-end" : "justify-start"
                            }`}
                        >
                            <div
                                className={`inline-block max-w-[80%] px-3 py-2 rounded-lg break-words ${
                                    msg.isOwn
                                        ? "bg-[#00d9ff] bg-opacity-20 text-[#e8f0ff]"
                                        : "bg-[#0f1419] text-gray-300"
                                }`}
                            >
                                <span
                                    className={`font-semibold ${
                                        msg.isOwn
                                            ? "text-[#00ffd9]"
                                            : "text-[#00d9ff]"
                                    }`}
                                >
                                    {msg.userName}:{" "}
                                </span>
                                <span className="break-words">{msg.text}</span>
                            </div>
                        </div>
                    ))
                )}
                <div ref={messagesEndRef} />
            </div>

            {/* Input Form */}
            <form onSubmit={handleSend} className="flex gap-2 min-w-0">
                <input
                    type="text"
                    value={input}
                    onChange={(e) => setInput(e.target.value)}
                    placeholder="Type a message..."
                    maxLength={127} // Giới hạn 127 ký tự (C struct limit)
                    className="flex-1 min-w-0 px-3 py-2 bg-[#0f1419] border border-[#00d9ff] 
                               border-opacity-30 rounded text-[#e8f0ff] text-sm 
                               focus:outline-none focus:ring-2 focus:ring-[#00d9ff]
                               placeholder-gray-500"
                />
                <button
                    type="submit"
                    disabled={!input.trim()}
                    className="px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded 
                               font-semibold hover:bg-[#00ffd9] transition shrink-0
                               disabled:opacity-50 disabled:cursor-not-allowed"
                >
                    Send
                </button>
            </form>
        </div>
    );
}
