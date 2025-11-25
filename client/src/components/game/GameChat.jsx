import { useState, useRef, useEffect } from "react";

export default function GameChat({ gameId, messages, onSendMessage }) {
  const [input, setInput] = useState("");
  const messagesEndRef = useRef(null);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSend = (e) => {
    e.preventDefault();
    if (input.trim()) {
      onSendMessage(gameId, input.trim());
      setInput("");
    }
  };

  return (
    <div className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-4 flex flex-col h-80">
      <h3 className="text-lg font-bold text-[#00d9ff] mb-3">Chat</h3>

      {/* Messages */}
      <div className="flex-1 overflow-y-auto space-y-2 mb-3">
        {messages.map((msg, index) => (
          <div key={index} className="text-sm">
            <span className="text-[#00d9ff] font-semibold">{msg.sender}: </span>
            <span className="text-gray-300">{msg.text}</span>
          </div>
        ))}
        <div ref={messagesEndRef} />
      </div>

      {/* Input */}
      <form onSubmit={handleSend} className="flex gap-2">
        <input
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="Type a message..."
          className="flex-1 px-3 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] text-sm focus:outline-none focus:ring-2 focus:ring-[#00d9ff]"
        />
        <button
          type="submit"
          className="px-4 py-2 bg-[#00d9ff] text-[#0f1419] rounded font-semibold hover:bg-[#00ffd9] transition"
        >
          Send
        </button>
      </form>
    </div>
  );
}
