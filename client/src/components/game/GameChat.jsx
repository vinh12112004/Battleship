import { useState, useRef, useEffect } from "react";

export default function GameChat({ gameId, messages, onSendMessage }) {
    // --- Thêm tin nhắn mẫu để test ---
    const initialTestMessages = [
        { senderId: "Player1", text: "Hello, ready chơi chưa?" },
        { senderId: "Player2", text: "Ok vào luôn!" },
    ];

    // --- State lưu tin nhắn cục bộ ---
    const [localMessages, setLocalMessages] = useState(initialTestMessages);

    const [input, setInput] = useState("");
    const messagesEndRef = useRef(null);

    // Mỗi khi props.messages thay đổi thì merge vào localMessages
    useEffect(() => {
        if (messages && messages.length > 0) {
            setLocalMessages((prev) => [...prev, ...messages]);
        }
    }, [messages]);

    const scrollToBottom = () => {
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    };

    useEffect(() => {
        scrollToBottom();
    }, [localMessages]);

    const handleSend = (e) => {
        e.preventDefault();
        if (input.trim()) {
            // Gửi ra ngoài
            onSendMessage(gameId, input.trim());

            // Gửi local để thấy ngay message
            setLocalMessages((prev) => [
                ...prev,
                { sender: "You", text: input.trim() },
            ]);

            setInput("");
        }
    };

    return (
        <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-4 flex flex-col h-80">
            <h3 className="text-lg font-bold text-[#00d9ff] mb-3">Chat</h3>

            {/* Messages */}
            <div className="flex-1 overflow-y-auto space-y-2 mb-3">
                {localMessages.map((msg, index) => (
                    <div key={index} className="text-sm">
                        <span className="text-[#00d9ff] font-semibold">
                            {msg.sender}:{" "}
                        </span>
                        <span className="text-gray-300">{msg.text}</span>
                    </div>
                ))}
                <div ref={messagesEndRef} />
            </div>

            {/* Input */}
            <form onSubmit={handleSend} className="flex gap-2 min-w-0">
                <input
                    type="text"
                    value={input}
                    onChange={(e) => setInput(e.target.value)}
                    placeholder="Type a message..."
                    className="flex-1 min-w-0 px-3 py-2 bg-[#0f1419] border border-[#00d9ff] 
               border-opacity-30 rounded text-[#e8f0ff] text-sm 
               focus:outline-none focus:ring-2 focus:ring-[#00d9ff]"
                />
                <button
                    type="submit"
                    className="px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded 
               font-semibold hover:bg-[#00ffd9] transition shrink-0"
                >
                    Send
                </button>
            </form>
        </div>
    );
}
