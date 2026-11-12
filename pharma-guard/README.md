# Pharma Guard

A beginner-friendly academic project: a **C++17 backend** (no third-party libraries) and a
**React + Tailwind CSS frontend** that together provide three modules:

| # | Module | What it does |
|---|--------|--------------|
| 1 | **Medicine Suitability** | You give **exactly one illness** and **one or more medicines**. Each medicine is judged SUITABLE / CAUTION / NOT SUITABLE using a local demo knowledge base. |
| 2 | **Medicine Interaction** | You give **two or more medicines**. Every pair is checked on **separate C++ worker threads** that share one **mutex-protected** result list. The page shows a thread-lifecycle trace. |
| 3 | **Counterfeit Detection** | You give **medicine name, batch number and manufacturer**. They are compared with a local demo registry: VERIFIED / MISMATCH / RECALLED / EXPIRED / NOT FOUND. |

There is also a **Database** tab (view the hand-built text database and reload it after editing) and a
**C++ Concepts** tab (live output of the OOP demos, produced by the C++ code).

> **Educational demo only.** The database is small, hand-written and the manufacturers are fictional.
> It is not medical advice and cannot verify real medicines.

---

## 1. Project layout

```
pharma-guard/
├── start.bat / start.sh        one-click start (Windows / Linux-macOS-WSL)
├── data/                       THE MANUAL DATABASE - five plain-text tables you can edit
│   ├── medicines.txt  illnesses.txt  suitability.txt  interactions.txt  registry.txt
├── backend/                    C++17 server
│   ├── include/  src/          the source code (read docs/CONCEPTS.md first)
│   ├── Makefile  CMakeLists.txt
├── frontend/                   React 18 + Tailwind CSS 3 + Vite
│   └── src/ (App.jsx, panels/, components/, api.js)
├── scripts/                    build / run / smoke-test scripts (.bat and .sh)
└── docs/                       CONCEPTS.md  DATABASE.md  API.md
```

How the pieces talk: **Browser → React (port 5173) → `/api/...` → C++ server (port 8080) → reads `data/*.txt`**.
In development, Vite forwards every `/api` request to the C++ server, so no CORS setup is needed.

---

## 2. What you need to install

| Tool | Why | Version |
|------|-----|---------|
| A C++17 compiler (g++ or clang++) | build the backend | g++ 8 or newer |
| Node.js (includes npm) | run the React frontend | Node 18 or newer (LTS recommended) |

### Windows (step by step)

1. **Install Node.js LTS** from <https://nodejs.org> (accept the defaults).
2. **Install a C++ compiler (MSYS2 + MinGW-w64)**
   1. Download and run the installer from <https://www.msys2.org>.
   2. Open **"MSYS2 UCRT64"** from the Start menu and run:
      ```
      pacman -S --needed mingw-w64-ucrt-x86_64-gcc
      ```
   3. Add the compiler to your Windows PATH: Start → type *"environment variables"* → *Environment Variables* →
      select **Path** under *User variables* → *Edit* → *New* → `C:\msys64\ucrt64\bin` → OK.
3. **Open a NEW Command Prompt** (so the PATH change is picked up) and check:
   ```
   g++ --version
   node --version
   npm --version
   ```
4. **Start everything** - double-click **`start.bat`** in the project folder, or in Command Prompt:
   ```
   cd path\to\pharma-guard
   start.bat
   ```
   It compiles the backend, runs `npm install` the first time (needs internet), opens two server windows and
   your browser at **<http://localhost:5173>**. Close the two server windows to stop.

Manual alternative (three Command Prompt commands, useful if something goes wrong):
```
cd path\to\pharma-guard
scripts\build-backend.bat         rem compile once (re-run after changing any C++ file)
scripts\run-backend.bat           rem window 1: C++ server on port 8080
scripts\run-frontend.bat          rem window 2: React app on port 5173
```

> **Windows note:** the Windows build was cross-compiled and linked successfully with MinGW-w64 (zero warnings)
> during development, but it could not be *run* on a real Windows machine. If anything fails on Windows,
> the fallback is **WSL** (Windows Subsystem for Linux): `wsl --install`, then follow the Linux steps below inside Ubuntu.

### Linux / macOS / WSL

```bash
# Ubuntu / Debian / WSL:  one-time install
sudo apt update && sudo apt install -y build-essential nodejs npm   # Node 18+ (or use nvm / nodejs.org)
# macOS:  xcode-select --install   and   brew install node

cd pharma-guard
./start.sh                 # builds the backend, starts it, then starts the frontend
# open http://localhost:5173
```

Manual alternative:
```bash
cd pharma-guard
scripts/build-backend.sh   # or:  cd backend && make
scripts/run-backend.sh     # terminal 1  -> http://127.0.0.1:8080
scripts/run-frontend.sh    # terminal 2  -> http://localhost:5173
```

CMake works too: `cd backend && cmake -S . -B build && cmake --build build`
(run the program from the project folder so it can find `data/`).

---

## 3. Using the app

1. **Suitability** - type one illness (e.g. `Fever`, or an alias such as `UTI`), add medicines (generic or brand names, e.g.
   `Paracetamol`, `Dolo 650`), press **Check suitability**. A pair that is not in the database is shown as **UNKNOWN**
   (never as "safe").
2. **Interaction** - add at least two medicines, press **Check interactions**. Look at the *Thread activity* card.
3. **Counterfeit** - enter name, batch and manufacturer. Use the example buttons to see all five outcomes.
4. **Database** - browse the tables. Edit a `.txt` file in `data/`, save, press **Reload from files**.

Every module has **"Try an example"** buttons so you can demonstrate it without typing.

## 4. Commands that are handy for the viva / lab

```
backend/build/pharma_guard --demo     prints the C++ concept demos in the terminal (Windows: backend\build\pharma_guard.exe --demo)
backend/build/pharma_guard --check    validates the database files and exits
backend/build/pharma_guard --port 9000 --data path/to/data     custom port / data folder
scripts/smoke-test.sh                 calls every API route of a running backend (36 checks)
```
If you change the backend port, start the frontend with `BACKEND_PORT=9000 npm run dev`
(Windows: `set BACKEND_PORT=9000&& npm run dev`).

To build the frontend for static hosting: `cd frontend && npm run build` (output in `frontend/dist`).

## 5. Troubleshooting

| Problem | Fix |
|---------|-----|
| `g++` / `npm` "is not recognized" | Close and reopen the terminal after installing; re-check the PATH step above. |
| Page says **"Backend offline"** | The C++ server is not running (or uses another port). Start it, then press *Try again*. |
| `Address already in use` / port busy | Another program uses 8080. Run `pharma_guard --port 9000` and start the frontend with `BACKEND_PORT=9000`. |
| `Could not find the data folder` | Run the backend from the project folder, or pass `--data path\to\data`. |
| Database reload shows an error | A line in a `data/*.txt` file is malformed; run `pharma_guard --check` - it names the file and line. |
| `npm install` fails | It needs internet access to download React, Tailwind and Vite. |

## 6. More documentation

* [`docs/CONCEPTS.md`](docs/CONCEPTS.md) - where every C++ concept lives in the code, and the Java → C++ thread mapping
* [`docs/DATABASE.md`](docs/DATABASE.md) - how the manual database works and how to add your own records
* [`docs/API.md`](docs/API.md) - the HTTP/JSON API with `curl` examples
