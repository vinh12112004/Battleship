export default function FooterSection() {
  return (
    <footer className="border-t border-[#00d9ff] border-opacity-20 py-12 px-6 mt-20">
      <div className="max-w-6xl mx-auto">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-8 mb-8">
          <div>
            <h3 className="text-[#00d9ff] font-bold mb-4">Product</h3>
            <ul className="space-y-2 text-gray-400 text-sm">
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Features
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Pricing
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Security
                </a>
              </li>
            </ul>
          </div>
          <div>
            <h3 className="text-[#00d9ff] font-bold mb-4">Company</h3>
            <ul className="space-y-2 text-gray-400 text-sm">
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  About
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Blog
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Careers
                </a>
              </li>
            </ul>
          </div>
          <div>
            <h3 className="text-[#00d9ff] font-bold mb-4">Legal</h3>
            <ul className="space-y-2 text-gray-400 text-sm">
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Privacy
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Terms
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Contact
                </a>
              </li>
            </ul>
          </div>
          <div>
            <h3 className="text-[#00d9ff] font-bold mb-4">Social</h3>
            <ul className="space-y-2 text-gray-400 text-sm">
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Twitter
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  Discord
                </a>
              </li>
              <li>
                <a href="#" className="hover:text-[#00d9ff] transition">
                  GitHub
                </a>
              </li>
            </ul>
          </div>
        </div>

        <div className="border-t border-[#00d9ff] border-opacity-20 pt-8 flex justify-between items-center">
          <p className="text-gray-500 text-sm">
            &copy; 2025 Battleship. All rights reserved.
          </p>
          <div className="flex gap-4">
            <button className="text-gray-400 hover:text-[#00d9ff] transition">
              Light
            </button>
            <button className="text-[#00d9ff]">Dark</button>
          </div>
        </div>
      </div>
    </footer>
  );
}
