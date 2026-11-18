import { useState } from 'react'
import { api } from '../api.js'
import TagInput from '../components/TagInput.jsx'
import { Badge, Banner, Card, ErrorBox, ExampleButtons, Spinner, bannerTone } from '../components/ui.jsx'

const EXAMPLES = [
  { label: 'Blood thinner mix', medicines: ['Warfarin', 'Aspirin', 'Paracetamol'] },
  { label: '4 medicines (6 pairs)', medicines: ['Warfarin', 'Aspirin', 'Paracetamol', 'Atenolol'] },
  { label: 'No interaction', medicines: ['Paracetamol', 'Cetirizine'] },
]

// Module 2: two or more medicines. The C++ backend checks the pairs on worker threads.
export default function InteractionPanel({ medicineNames }) {
  const [medicines, setMedicines] = useState([])
  const [result, setResult] = useState(null)
  const [error, setError] = useState('')
  const [busy, setBusy] = useState(false)

  async function submit(e) {
    e?.preventDefault()
    setError('')
    setResult(null)
    if (medicines.length < 2) return setError('Please add at least two medicines.')
    setBusy(true)
    try {
      setResult(await api.interactions(medicines))
    } catch (err) {
      setError(err.message)
    } finally {
      setBusy(false)
    }
  }

  return (
    <div className="space-y-5">
      <Card
        title="Medicine Interaction"
        subtitle="Add two or more medicines. The backend checks each pair for known interactions."
      >
        <form onSubmit={submit} className="space-y-4">
          <TagInput
            label="Medicines (two or more)"
            values={medicines}
            onChange={setMedicines}
            suggestions={medicineNames}
            placeholder="e.g. Warfarin, Aspirin"
          />
          <div className="flex flex-wrap items-center gap-4">
            <button className="btn-primary" disabled={busy}>
              Check interactions
            </button>
            <ExampleButtons
              examples={EXAMPLES}
              onPick={(ex) => {
                setMedicines(ex.medicines)
                setResult(null)
                setError('')
              }}
            />
          </div>
        </form>
      </Card>

      {busy && <Spinner text="Checking medicine pairs..." />}
      <ErrorBox message={error} />

      {result && (
        <>
          <Card>
            <Banner tone={bannerTone(result.status)} title={result.status.replaceAll('_', ' ')}>
              {result.headline}
            </Banner>

            {result.interactions?.length > 0 && (
              <ul className="mt-4 space-y-3">
                {result.interactions.map((it, i) => (
                  <li key={i} className="rounded-xl border border-slate-200 p-4">
                    <div className="flex flex-wrap items-center gap-2">
                      <span className="font-semibold text-slate-900">
                        {it.medicineA} + {it.medicineB}
                      </span>
                      <Badge value={it.severity} />
                    </div>
                    <p className="mt-2 text-sm text-slate-700">{it.effect}</p>
                    <p className="mt-1 text-sm text-slate-500">
                      <span className="font-medium text-slate-600">Advice:</span> {it.advice}
                    </p>
                  </li>
                ))}
              </ul>
            )}

            {result.cleanPairs?.length > 0 && (
              <p className="mt-4 text-sm text-slate-600">
                <span className="font-medium">No known interaction in the demo database:</span> {result.cleanPairs.join(', ')}
              </p>
            )}
            {result.unknownMedicines?.length > 0 && (
              <p className="mt-2 text-sm text-amber-700">Not recognised (skipped): {result.unknownMedicines.join(', ')}</p>
            )}
            {result.notes?.length > 0 && (
              <ul className="mt-3 list-disc space-y-1 pl-5 text-sm text-slate-600">
                {result.notes.map((n, i) => (
                  <li key={i}>{n}</li>
                ))}
              </ul>
            )}
          </Card>
        </>
      )}
    </div>
  )
}
