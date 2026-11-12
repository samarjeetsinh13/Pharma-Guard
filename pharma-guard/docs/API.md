# HTTP / JSON API

Base URL: `http://127.0.0.1:8080`. Every answer is JSON: `{"ok":true,"result":{...}}` on success or
`{"ok":false,"error":"..."}` with HTTP 400/404/405/500 on failure. Limits: at most 10 medicines per request,
at most 80 characters per value.

| Method | Path | Body | Purpose |
|--------|------|------|---------|
| GET | `/api/health` | - | Server status and database counts |
| GET | `/api/database` | - | All five tables as JSON |
| POST | `/api/reload` | - | Re-read the `data/*.txt` files |
| POST | `/api/suitability` | `{"illness":"Fever","medicines":["Paracetamol","Aspirin"]}` | Module 1 (exactly one illness) |
| POST | `/api/interactions` | `{"medicines":["Warfarin","Aspirin","Paracetamol"]}` | Module 2 (worker threads) |
| POST | `/api/counterfeit` | `{"medicine":"Paracetamol","batch":"PCM-2501-A12","manufacturer":"Sunrise Pharma Pvt Ltd"}` | Module 3 |
| GET | `/api/concepts` | - | Live output of the C++ concept demos |

```bash
curl -X POST -d '{"illness":"Fever","medicines":["Paracetamol","Amoxicillin"]}' http://127.0.0.1:8080/api/suitability
curl -X POST -d '{"medicines":["Warfarin","Aspirin"]}' http://127.0.0.1:8080/api/interactions
curl -X POST -d '{"medicine":"Paracetamol","batch":"PCM-2501-A12","manufacturer":"Sunrise Pharma Pvt Ltd"}' http://127.0.0.1:8080/api/counterfeit
```
(On Windows Command Prompt use `curl -X POST -d "{\"medicines\":[\"Warfarin\",\"Aspirin\"]}" ...`, or use the web app.)

Result fields (main ones)

* **suitability** - `status`, `headline`, `illness`, `illnessFound`, `counts`, `items[]` (`medicine`, `drugClass`, `verdict`, `reason`)
* **interaction** - `status`, `overallSeverity`, `interactions[]` (`medicineA`, `medicineB`, `severity`, `effect`, `advice`, `foundBy`),
  `cleanPairs[]`, `unknownMedicines[]`, `thread` (`workers`, `pairsChecked`, `mutexLocks`, `elapsedMs`, `trace[]`)
* **counterfeit** - `status` (`VERIFIED|MISMATCH|RECALLED|EXPIRED|NOT_FOUND`), `matchPercent`, `checks[]`, `registryRecord`

The server listens on `127.0.0.1` only. Run `scripts/smoke-test.sh` to exercise every route.
