// All communication with the C++ backend lives in this one file.
//
// In development, Vite forwards /api/... to the backend (see vite.config.js).
// To call a backend on another address, create frontend/.env with:
//   VITE_API_URL=http://127.0.0.1:8080
const BASE = import.meta.env.VITE_API_URL || ''

async function request(path, options) {
  let response
  try {
    response = await fetch(BASE + path, options)
  } catch {
    throw new Error(
      'Cannot reach the C++ backend. Start it first (scripts/run-backend.sh or scripts\\run-backend.bat) and check the port.',
    )
  }

  let payload
  try {
    payload = await response.json()
  } catch {
    throw new Error(`The backend sent an unexpected answer (HTTP ${response.status}).`)
  }
  if (!response.ok || payload.ok === false) {
    throw new Error(payload.error || `Request failed (HTTP ${response.status})`)
  }
  return payload.result
}

const post = (path, body) =>
  request(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body ?? {}),
  })

export const api = {
  health: () => request('/api/health'),
  database: () => request('/api/database'),
  reload: () => post('/api/reload'),
  concepts: () => request('/api/concepts'),
  suitability: (illness, medicines) => post('/api/suitability', { illness, medicines }),
  interactions: (medicines) => post('/api/interactions', { medicines }),
  counterfeit: (medicine, batch, manufacturer) => post('/api/counterfeit', { medicine, batch, manufacturer }),
}
