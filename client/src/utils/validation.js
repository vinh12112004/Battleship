export const validators = {
  required: (value) => {
    return value && value.trim() !== "" ? "" : "This field is required";
  },

  email: (value) => {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(value) ? "" : "Invalid email address";
  },

  minLength: (min) => (value) => {
    return value && value.length >= min
      ? ""
      : `Must be at least ${min} characters`;
  },

  maxLength: (max) => (value) => {
    return value && value.length <= max
      ? ""
      : `Must be at most ${max} characters`;
  },

  password: (value) => {
    if (!value || value.length < 6) {
      return "Password must be at least 6 characters";
    }
    if (!/[a-zA-Z]/.test(value)) {
      return "Password must contain at least one letter";
    }
    if (!/[0-9]/.test(value)) {
      return "Password must contain at least one number";
    }
    return "";
  },

  username: (value) => {
    if (!value || value.length < 3) {
      return "Username must be at least 3 characters";
    }
    if (value.length > 20) {
      return "Username must be at most 20 characters";
    }
    if (!/^[a-zA-Z0-9_]+$/.test(value)) {
      return "Username can only contain letters, numbers, and underscores";
    }
    return "";
  },
};

export function validateLoginForm(values) {
  const errors = {};

  const usernameError =
    validators.required(values.username) ||
    validators.username(values.username);
  if (usernameError) errors.username = usernameError;

  const passwordError = validators.required(values.password);
  if (passwordError) errors.password = passwordError;

  return errors;
}

export function validateRegisterForm(values) {
  const errors = {};

  const usernameError =
    validators.required(values.username) ||
    validators.username(values.username);
  if (usernameError) errors.username = usernameError;

  const emailError =
    validators.required(values.email) || validators.email(values.email);
  if (emailError) errors.email = emailError;

  const passwordError =
    validators.required(values.password) ||
    validators.password(values.password);
  if (passwordError) errors.password = passwordError;

  if (values.confirmPassword !== values.password) {
    errors.confirmPassword = "Passwords do not match";
  }

  return errors;
}
