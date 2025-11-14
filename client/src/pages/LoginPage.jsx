import { Link } from "react-router-dom";
import NavBar from "../components/common/NavBar";
import LoginForm from "@/components/auth/LoginForm";

export default function LoginPage() {
  return (
    <div className="min-h-screen bg-[#0f1419]">
      <NavBar />
      <div className="flex items-center justify-center min-h-[calc(100vh-80px)] py-12 px-4">
        <div className="w-full max-w-md">
          <div className="text-center mb-8">
            <h1 className="text-3xl font-bold text-[#00d9ff] mb-2">
              Battle Awaits
            </h1>
            <p className="text-gray-400">Sign in to your account</p>
          </div>
          <LoginForm />
          <p className="text-center text-gray-400 mt-6">
            Don't have an account?{" "}
            <Link
              to="/register"
              className="text-[#00d9ff] hover:text-[#00ffd9] transition"
            >
              Create one
            </Link>
          </p>
        </div>
      </div>
    </div>
  );
}
