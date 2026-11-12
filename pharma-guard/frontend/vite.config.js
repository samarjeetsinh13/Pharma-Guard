import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// During development the browser talks to Vite (port 5173) and Vite forwards every
// /api/... request to the C++ backend, so no CORS setup is needed.
// If your backend runs on another port, start it with:  BACKEND_PORT=9000 npm run dev
const backendPort = process.env.BACKEND_PORT || 8080

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': `http://127.0.0.1:${backendPort}`,
    },
  },
})
