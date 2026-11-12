#!/usr/bin/env bash
# Smoke test: calls every API route of a RUNNING Pharma Guard backend and checks the answers.
#
#   1) start the backend in one terminal   (scripts/run-backend.sh)
#   2) run this script in another terminal  (scripts/smoke-test.sh)
#
# Optional: PORT=9000 scripts/smoke-test.sh

PORT="${PORT:-8080}"
BASE="http://127.0.0.1:${PORT}"
PASS=0
FAIL=0

# check "description" "expected text" <curl arguments...>
check() {
  local name="$1" expected="$2"
  shift 2
  local output
  output="$(curl -s --max-time 10 "$@")"
  if [[ "$output" == *"$expected"* ]]; then
    printf '  PASS  %s\n' "$name"
    PASS=$((PASS + 1))
  else
    printf '  FAIL  %s\n        expected to contain: %s\n        got: %s\n' "$name" "$expected" "${output:0:300}"
    FAIL=$((FAIL + 1))
  fi
}

post() { # post <path> <json>
  echo -X POST -H 'Content-Type: application/json' -d "$2" "${BASE}$1"
}

echo "Testing Pharma Guard at ${BASE}"

echo "General"
check "health"                         '"status":"running"'           "${BASE}/api/health"
check "database dump has medicines"    '"name":"Paracetamol"'         "${BASE}/api/database"
check "unknown route -> 404"           'No such API route'            "${BASE}/api/nothing"
check "bad JSON -> 400"                'not valid JSON'               -X POST -d '{oops' "${BASE}/api/suitability"

echo "Module 1 - Medicine Suitability"
check "suitable medicine"              '"verdict":"SUITABLE"'         -X POST -d '{"illness":"Fever","medicines":["Paracetamol"]}' "${BASE}/api/suitability"
check "caution medicine"               '"verdict":"CAUTION"'          -X POST -d '{"illness":"Fever","medicines":["Aspirin"]}' "${BASE}/api/suitability"
check "not suitable medicine"          '"verdict":"NOT_SUITABLE"'     -X POST -d '{"illness":"Fever","medicines":["Amoxicillin"]}' "${BASE}/api/suitability"
check "illness alias (UTI)"            '"illness":"Urinary Tract Infection"' -X POST -d '{"illness":"uti","medicines":["Cipro"]}' "${BASE}/api/suitability"
check "brand name + dose (Dolo 650)"   '"medicine":"Paracetamol"'     -X POST -d '{"illness":"Fever","medicines":["Dolo 650"]}' "${BASE}/api/suitability"
check "dose stripped (500mg tablet)"   '"medicine":"Paracetamol"'     -X POST -d '{"illness":"Fever","medicines":["paracetamol 500mg tablet"]}' "${BASE}/api/suitability"
check "unknown rule -> UNKNOWN"        '"verdict":"UNKNOWN"'          -X POST -d '{"illness":"Fever","medicines":["Warfarin"]}' "${BASE}/api/suitability"
check "unknown medicine -> NOT_FOUND"  '"verdict":"NOT_FOUND"'        -X POST -d '{"illness":"Fever","medicines":["Zzzcillin"]}' "${BASE}/api/suitability"
check "unknown illness"                '"illnessFound":false'         -X POST -d '{"illness":"Moon sickness","medicines":["Paracetamol"]}' "${BASE}/api/suitability"
check "two illnesses (array) rejected" 'exactly one illness'          -X POST -d '{"illness":["Fever","Headache"],"medicines":["Paracetamol"]}' "${BASE}/api/suitability"
check "two illnesses (comma) rejected" 'exactly one illness'          -X POST -d '{"illness":"Fever, Headache","medicines":["Paracetamol"]}' "${BASE}/api/suitability"
check "no medicines rejected"          'at least 1 medicine'          -X POST -d '{"illness":"Fever","medicines":[]}' "${BASE}/api/suitability"

echo "Module 2 - Medicine Interaction (worker threads)"
check "major interaction found"        '"severity":"MAJOR"'           -X POST -d '{"medicines":["Warfarin","Aspirin"]}' "${BASE}/api/interactions"
check "order does not matter"          '"severity":"MAJOR"'           -X POST -d '{"medicines":["Aspirin","Warfarin"]}' "${BASE}/api/interactions"
check "no known interaction"           'NO_KNOWN_INTERACTIONS'        -X POST -d '{"medicines":["Paracetamol","Cetirizine"]}' "${BASE}/api/interactions"
check "4 medicines, 2 workers"         '"workers":2'                  -X POST -d '{"medicines":["Warfarin","Aspirin","Paracetamol","Atenolol"]}' "${BASE}/api/interactions"
check "6 pairs checked"                '"pairsChecked":6'             -X POST -d '{"medicines":["Warfarin","Aspirin","Paracetamol","Atenolol"]}' "${BASE}/api/interactions"
check "thread lifecycle in trace"      'state TERMINATED'             -X POST -d '{"medicines":["Warfarin","Aspirin","Paracetamol"]}' "${BASE}/api/interactions"
check "mutex locks reported"           '"mutexLocks":'                -X POST -d '{"medicines":["Warfarin","Aspirin","Paracetamol"]}' "${BASE}/api/interactions"
check "duplicate ignored"              'Duplicate ignored'            -X POST -d '{"medicines":["Warfarin","warfarin","Aspirin"]}' "${BASE}/api/interactions"
check "unknown medicine skipped"       'Zzzcillin'                    -X POST -d '{"medicines":["Warfarin","Aspirin","Zzzcillin"]}' "${BASE}/api/interactions"
check "one medicine rejected"          'at least 2 medicines'         -X POST -d '{"medicines":["Warfarin"]}' "${BASE}/api/interactions"

echo "Module 3 - Counterfeit Detection"
check "verified"                       '"status":"VERIFIED"'          -X POST -d '{"medicine":"Paracetamol","batch":"PCM-2501-A12","manufacturer":"Sunrise Pharma Pvt Ltd"}' "${BASE}/api/counterfeit"
check "forgiving batch/maker format"   '"status":"VERIFIED"'          -X POST -d '{"medicine":"crocin","batch":"pcm 2501 a12","manufacturer":"sunrise pharma pvt. ltd."}' "${BASE}/api/counterfeit"
check "wrong manufacturer -> MISMATCH" '"status":"MISMATCH"'          -X POST -d '{"medicine":"Paracetamol","batch":"PCM-2501-A12","manufacturer":"Fake Labs"}' "${BASE}/api/counterfeit"
check "wrong medicine -> MISMATCH"     '"status":"MISMATCH"'          -X POST -d '{"medicine":"Ibuprofen","batch":"PCM-2501-A12","manufacturer":"Sunrise Pharma Pvt Ltd"}' "${BASE}/api/counterfeit"
check "unknown batch -> NOT_FOUND"     '"status":"NOT_FOUND"'         -X POST -d '{"medicine":"Paracetamol","batch":"XXX-0000","manufacturer":"Sunrise Pharma Pvt Ltd"}' "${BASE}/api/counterfeit"
check "recalled batch"                 '"status":"RECALLED"'          -X POST -d '{"medicine":"Amoxicillin","batch":"AMX-2506-E33","manufacturer":"Vedant Remedies"}' "${BASE}/api/counterfeit"
check "expired batch"                  '"status":"EXPIRED"'           -X POST -d '{"medicine":"Paracetamol","batch":"PCM-2309-B07","manufacturer":"Sunrise Pharma Pvt Ltd"}' "${BASE}/api/counterfeit"
check "missing field rejected"         'Please enter the manufacturer' -X POST -d '{"medicine":"Paracetamol","batch":"PCM-2501-A12","manufacturer":"  "}' "${BASE}/api/counterfeit"

echo "Extras"
check "reload database"                '"reloaded":true'              -X POST "${BASE}/api/reload"
check "C++ concept demos"              'Virtual inheritance'          "${BASE}/api/concepts"

echo
echo "Result: ${PASS} passed, ${FAIL} failed"
[[ "$FAIL" -eq 0 ]]
