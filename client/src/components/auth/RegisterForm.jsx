import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { useAuth } from "@/hooks/useAuth";
import { useForm } from "@/hooks/useForm";
import { validateRegisterForm } from "@/utils/validation";

export default function RegisterForm() {
  const navigate = useNavigate();
  const { register } = useAuth();
  const [apiError, setApiError] = useState("");

  const {
    values,
    errors,
    touched,
    isSubmitting,
    handleChange,
    handleBlur,
    handleSubmit,
  } = useForm(
    {
      username: "",
      email: "",
      password: "",
      confirmPassword: "",
    },
    validateRegisterForm
  );

  const onSubmit = async (formValues) => {
    try {
      setApiError("");
      await register(
        formValues.username,
        formValues.email,
        formValues.password
      );
      navigate("/login");
    } catch (error) {
      setApiError(error.message || "Registration failed. Please try again.");
    }
  };

  return (
    <form
      onSubmit={handleSubmit(onSubmit)}
      className="bg-[#1a2332] border border-[#00d9ff] border-opacity-30 rounded-lg p-8 space-y-6"
    >
      {apiError && (
        <div className="p-3 bg-[#ff4757] bg-opacity-20 border border-[#ff4757] rounded text-[#ff4757] text-sm">
          {apiError}
        </div>
      )}

      <div>
        <label className="block text-sm font-medium text-[#00d9ff] mb-2">
          Username
        </label>
        <input
          type="text"
          name="username"
          value={values.username}
          onChange={handleChange}
          onBlur={handleBlur}
          className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] placeholder-gray-600 focus:outline-none focus:ring-2 focus:ring-[#00d9ff] focus:border-transparent"
          placeholder="username"
          disabled={isSubmitting}
        />
        {touched.username && errors.username && (
          <p className="mt-1 text-sm text-[#ff4757]">{errors.username}</p>
        )}
      </div>

      <div>
        <label className="block text-sm font-medium text-[#00d9ff] mb-2">
          Email
        </label>
        <input
          type="email"
          name="email"
          value={values.email}
          onChange={handleChange}
          onBlur={handleBlur}
          className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] placeholder-gray-600 focus:outline-none focus:ring-2 focus:ring-[#00d9ff] focus:border-transparent"
          placeholder="your@email.com"
          disabled={isSubmitting}
        />
        {touched.email && errors.email && (
          <p className="mt-1 text-sm text-[#ff4757]">{errors.email}</p>
        )}
      </div>

      <div>
        <label className="block text-sm font-medium text-[#00d9ff] mb-2">
          Password
        </label>
        <input
          type="password"
          name="password"
          value={values.password}
          onChange={handleChange}
          onBlur={handleBlur}
          className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] placeholder-gray-600 focus:outline-none focus:ring-2 focus:ring-[#00d9ff] focus:border-transparent"
          placeholder="••••••••"
          disabled={isSubmitting}
        />
        {touched.password && errors.password && (
          <p className="mt-1 text-sm text-[#ff4757]">{errors.password}</p>
        )}
      </div>

      <div>
        <label className="block text-sm font-medium text-[#00d9ff] mb-2">
          Confirm Password
        </label>
        <input
          type="password"
          name="confirmPassword"
          value={values.confirmPassword}
          onChange={handleChange}
          onBlur={handleBlur}
          className="w-full px-4 py-2 bg-[#0f1419] border border-[#00d9ff] border-opacity-30 rounded text-[#e8f0ff] placeholder-gray-600 focus:outline-none focus:ring-2 focus:ring-[#00d9ff] focus:border-transparent"
          placeholder="••••••••"
          disabled={isSubmitting}
        />
        {touched.confirmPassword && errors.confirmPassword && (
          <p className="mt-1 text-sm text-[#ff4757]">
            {errors.confirmPassword}
          </p>
        )}
      </div>

      <button
        type="submit"
        disabled={isSubmitting}
        className="w-full py-3 bg-gradient-to-r from-[#00d9ff] to-[#9d4edd] text-[#0f1419] font-bold rounded hover:shadow-lg hover:shadow-[#00d9ff] transition transform hover:scale-105 disabled:opacity-50 disabled:cursor-not-allowed"
      >
        {isSubmitting ? "Creating account..." : "Create Account"}
      </button>
    </form>
  );
}
