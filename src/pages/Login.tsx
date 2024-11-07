import { useState } from "react";
import { getToken } from "firebase/messaging";
import { messaging } from "../utils/firebaseClient.ts";
import { useNavigate } from "react-router-dom";
import Dialog from "../components/Dialog.tsx";
import SignUp from "../components/Signup.tsx";
import { TextField, IconButton, InputAdornment, Button } from "@mui/material";
import { Visibility, VisibilityOff } from "@mui/icons-material";
import { useUser } from "../hooks/use-user.ts";

async function registerFCMToken() {
  try {
    // Register the service worker with the correct path
    const registration = await navigator.serviceWorker.register('/firebase-messaging-sw.js');

    // Wait for the service worker to be ready
    await navigator.serviceWorker.ready;

    // Retrieve the token with the service worker registration and VAPID key
    const token = await getToken(messaging, {
      serviceWorkerRegistration: registration,
      vapidKey: import.meta.env["VITE_FIREBASE_VAPID_KEY"], // Insert your VAPID key here
    });

    console.log("Token received:", token);
    return token;
  } catch (error) {
    console.error(
      "Service worker registration or token retrieval failed:",
      error,
    );
    return null;
  }
}

export default function Login() {
  const [showPassword, setShowPassword] = useState(false);
  const [email, setEmail] = useState("");
  const [showSignUpModal, setShowSignUpModal] = useState(false);
  const [password, setPassword] = useState("");
  const navigate = useNavigate();
  const { updateProfile } = useUser();

  async function handleLogin(event) {
    event.preventDefault();
    try {
      const response = await fetch("http://localhost:3001/auth/login", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        credentials: "include",
        body: JSON.stringify({ email, password }),
      });

      const data = await response.json();
      if (!response.ok) {
        throw new Error(data.error || "Login failed");
      }

      const fcmToken = await registerFCMToken();

      // Update the profile with the FCM token and user metadata
      await updateProfile({
        user_id: data.user.id,
        first_name: data.user.user_metadata.first_name,
        last_name: data.user.user_metadata.last_name,
        speciality: data.user.user_metadata.speciality,
        permissions: data.user.user_metadata.permissions,
        fcm_token: fcmToken,
      });

      navigate("/dashboard", { replace: true });
    } catch (error) {
      console.error("Login error:", error.message);
      alert(`Login failed: ${error.message}`);
    }
  }

  const textFieldSx = {
    "& label.Mui-focused": {
      color: "white",
    },
    "& .MuiInput-underline:after": {
      borderBottomColor: "white",
    },
    "& .MuiOutlinedInput-root": {
      "&.Mui-focused fieldset": {
        borderColor: "white",
      },
    },
  };

  return (
    <div className="relative flex h-screen gap-8">
      <div className="flex flex-col justify-center w-full max-w-md px-8 mx-auto lg:w-1/2">
        <div className="flex justify-center">
          <img
            src="/assets/logo.png"
            alt="Logo"
            className="object-contain"
            style={{ width: "300px", height: "300px" }}
          />
        </div>

        <form className="space-y-6" onSubmit={handleLogin}>
          <TextField
            id="email"
            name="email"
            type="email"
            required
            fullWidth
            variant="outlined"
            label="Email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            sx={textFieldSx}
          />
          <TextField
            id="password"
            name="password"
            type={showPassword ? "text" : "password"}
            required
            fullWidth
            variant="outlined"
            label="Password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            sx={textFieldSx}
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    onClick={() => setShowPassword(!showPassword)}
                    edge="end"
                    aria-label={
                      showPassword ? "Hide password" : "Show password"
                    }
                  >
                    {showPassword ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              ),
            }}
          />
          <Button
            type="submit"
            fullWidth
            variant="contained"
            onClick={(e) => handleLogin(e)}
            sx={{
              mt: 3,
              mb: 2,
              textTransform: "none",
              fontSize: "1rem",
              backgroundColor: "blueAccent.main",
              "&:hover": {
                backgroundColor: "#1e3a8a",
              },
            }}
          >
            Sign In
          </Button>
        </form>
        <p className="mt-4 text-center text-sm text-white">
          Don't have an account?{" "}
          <Button color="secondary" onClick={() => setShowSignUpModal(true)}>
            Sign up
          </Button>
        </p>
        <Dialog
          open={showSignUpModal}
          dialogStateChange={(open) => setShowSignUpModal(open)}
          contents={
            <>
              <SignUp onClose={() => setShowSignUpModal(false)} />
            </>
          }
        />
      </div>

      <div className="w-full lg:w-1/2">
        <img
          className="object-cover w-full h-full"
          src="/assets/exoflex_team.jpg"
          alt="Team background"
        />
      </div>
    </div>
  );
}
