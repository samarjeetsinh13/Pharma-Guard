# The manual database

The project has **no SQL engine**. The database is five plain-text files in `data/`, written by hand and loaded
by `Database.cpp` when the server starts (or when you press **Reload from files**). One record per line, columns
separated by `|`. Blank lines and lines starting with `#` are ignored. Spaces around `|` do not matter.

| File | Columns | Used by |
|------|---------|---------|
| `medicines.txt` | `id \| name \| generic name \| drug class \| typical use \| aliases` | all modules |
| `illnesses.txt` | `id \| name \| category \| description \| aliases` | suitability |
| `suitability.txt` | `illness \| medicine \| verdict \| reason` | suitability |
| `interactions.txt` | `medicine A \| medicine B \| severity \| what happens \| advice` | interaction |
| `registry.txt` | `medicine \| batch number \| manufacturer \| manufacture date \| expiry date \| status` | counterfeit |

## Rules for each column

* **aliases** - comma-separated alternative names (brand names, short forms). Matching ignores case, punctuation
  and typed doses (`Paracetamol 500mg tablet` matches `Paracetamol`).
* **id** - unique (`M001`, `I001`, ...). Rule files refer to medicines and illnesses by **name or alias**, not by id.
* **verdict** (suitability) - `SUITABLE`, `CAUTION` or `NOT_SUITABLE`. A pair that is not listed is reported as `UNKNOWN`.
* **severity** (interactions) - `MINOR`, `MODERATE` or `MAJOR`. Order of A and B does not matter. An unlisted pair is
  reported as "no known interaction in the demo database", **not** as safe.
* **dates** (registry) - `YYYY-MM-DD`. **status** - `GENUINE` or `RECALLED`. Batch numbers are compared ignoring case,
  spaces and dashes (`pcm 2501 a12` = `PCM-2501-A12`). Manufacturer names are fictional.

## How the counterfeit check decides

1. Find the registry row by **batch number**. No row → `NOT_FOUND` (cannot verify).
2. Compare the typed **medicine name** (generic or brand) and **manufacturer** with that row.
   Any difference → `MISMATCH` (possible counterfeit).
3. Everything matches, but status is `RECALLED` → `RECALLED`; expiry date before today → `EXPIRED`;
   otherwise → `VERIFIED`.

## Adding your own records

1. Open the file in any text editor and add a line in the same format. Example (new batch):
   `Paracetamol | PCM-2601-C44 | Sunrise Pharma Pvt Ltd | 2026-01-10 | 2028-12-31 | GENUINE`
2. Save. In a terminal run `backend/build/pharma_guard --check` - it reports the exact file and line of any mistake.
3. In the web app open **Database → Reload from files** (no recompiling and no restart needed).

A new medicine needs a new line in `medicines.txt` **first**, because the other files refer to it by name.

> All content is for teaching. Do not use it for real medical decisions.
