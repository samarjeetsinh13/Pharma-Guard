import { useState } from 'react'
import { api } from '../api.js'
import TagInput from '../components/TagInput.jsx'
import { Badge, Banner, Card, ErrorBox, ExampleButtons, Spinner, bannerTone } from '../components/ui.jsx'

const EXAMPLES = [
  { label: 'Fever + 3 medicines', illness: 'Fever', medicines: ['Paracetamol', 'Aspirin', 'Amoxicillin'] },
  { label: 'Hypertension', illness: 'Hypertension', medicines: ['Amlodipine', 'Ibuprofen'] },
  { label: 'UTI (alias)', illness: 'UTI', medicines: ['Cipro', 'Dolo 650'] },
]

// Module 1: exactly ONE illness + one or more medicines.
export default function SuitabilityPanel({ illnessNames, medicineNames }) {
  const [illness, setIllness] = useState('')
  const [medicines, setMedicines] = useState([])
  const [result, setResult] = useState(null)
  const [error, setError] = useState('')
  const [busy, setBusy] = useState(false)

  async function submit(e) {
    e?.preventDefault()
    setError('')
    setResult(null)
    if (!illness.trim()) return setError('Please enter one illness.')
    if (medicines.length === 0) return setError('Please add at least one medicine.')
    setBusy(true)
    try {
      setResult(await api.suitability(illness.trim(), medicines))
    } catch (err) {
      setError(err.message)
    } finally {
      setBusy(false)
    }
  }

  return (
    <div className="space-y-5">
      <Card
        title="Medicine Suitability"
        subtitle="Choose exactly one illness and one or more medicines. Each medicine is checked against the local demo knowledge base."
      >
        <form onSubmit={submit} className="space-y-4">
          <div>
            <label className="field-label" htmlFor="illness">
              Illness (exactly one)
            </label>
            <input
              id="illness"
              className="field-input"
              list="illness-list"
              value={illness}
              onChange={(e) => setIllness(e.target.value)}
              placeholder="e.g. Fever, Hypertension, UTI"
            />
            <datalist id="illness-list">
              {illnessNames.map((n) => (
                <option key={n} value={n} />
              ))}
            </datalist>
          </div>
          <TagInput
            label="Medicines (one or more)"
            values={medicines}
            onChange={setMedicines}
            suggestions={medicineNames}
            placeholder="e.g. Paracetamol, Dolo 650"
          />
          <div className="flex flex-wrap items-center gap-4">
            <button className="btn-primary" disabled={busy}>
              Check suitability
            </button>
            <ExampleButtons
              examples={EXAMPLES}
              onPick={(ex) => {
                setIllness(ex.illness)
                setMedicines(ex.medicines)
                setResult(null)
                setError('')
              }}
            />
          </div>
        </form>
      </Card>

      {busy && <Spinner />}
      <ErrorBox message={error} />

      {result && (
        <Card>
          <Banner tone={bannerTone(result.status)} title={result.status.replaceAll('_', ' ')}>
            {result.headline}
          </Banner>
          {result.illnessFound && (
            <div className="mt-4 overflow-x-auto">
              <table className="w-full text-left text-sm">
                <thead className="border-b border-slate-200 text-xs uppercase tracking-wide text-slate-500">
                  <tr>
                    <th className="py-2 pr-3">Medicine</th>
                    <th className="py-2 pr-3">Verdict</th>
                    <th className="py-2">Why</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100">
                  {result.items.map((it, i) => (
                    <tr key={i} className="align-top">
                      <td className="py-3 pr-3">
                        <div className="font-medium text-slate-900">{it.medicine || it.input}</div>
                        {it.drugClass && <div className="text-xs text-slate-500">{it.drugClass}</div>}
                        {it.medicine && it.medicine.toLowerCase() !== it.input.toLowerCase() && (
                          <div className="text-xs text-slate-400">you typed: {it.input}</div>
                        )}
                      </td>
                      <td className="py-3 pr-3">
                        <Badge value={it.verdict} />
                      </td>
                      <td className="py-3 text-slate-600">{it.reason}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
          {result.notes?.length > 0 && (
            <ul className="mt-3 list-disc space-y-1 pl-5 text-sm text-slate-600">
              {result.notes.map((n, i) => (
                <li key={i}>{n}</li>
              ))}
            </ul>
          )}
        </Card>
      )}
    </div>
  )
}
