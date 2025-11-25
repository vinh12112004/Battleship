import { Link } from "react-router-dom";
import NavBar from "../components/common/Navbar";
import RegisterForm from "../components/auth/RegisterForm";

export default function RegisterPage() {
  return (
    <div className="min-h-screen bg-[#0f1419]">
      <NavBar />
      <div className="flex items-center justify-center min-h-[calc(100vh-80px)] py-12 px-4">
        <div className="w-full max-w-md">
          <div className="text-center mb-8">
            <h1 className="text-3xl font-bold text-[#00d9ff] mb-2">
              Join the Battle
            </h1>
            <p className="text-gray-400">
              Create your account to start playing
            </p>
          </div>
          <RegisterForm />
          <p className="text-center text-gray-400 mt-6">
            Already have an account?{" "}
            <Link
              to="/login"
              className="text-[#00d9ff] hover:text-[#00ffd9] transition"
            >
              Sign in
            </Link>
          </p>
        </div>
      </div>
    </div>
  );
}
