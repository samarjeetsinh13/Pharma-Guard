import { useState } from 'react'
import { api } from '../api.js'
import { Badge, Banner, Card, Disclaimer, ErrorBox, ExampleButtons, Spinner, bannerTone } from '../components/ui.jsx'

const EXAMPLES = [
  { label: 'Genuine batch', medicine: 'Paracetamol', batch: 'PCM-2501-A12', manufacturer: 'Sunrise Pharma Pvt Ltd' },
  { label: 'Wrong manufacturer', medicine: 'Paracetamol', batch: 'PCM-2501-A12', manufacturer: 'QuickCure Labs' },
  { label: 'Recalled batch', medicine: 'Amoxicillin', batch: 'AMX-2506-E33', manufacturer: 'Vedant Remedies' },
  { label: 'Expired batch', medicine: 'Paracetamol', batch: 'PCM-2309-B07', manufacturer: 'Sunrise Pharma Pvt Ltd' },
  { label: 'Unknown batch', medicine: 'Ibuprofen', batch: 'IBU-9999-Z99', manufacturer: 'Medora Laboratories' },
]

// Module 3: medicine name + batch number + manufacturer, compared with the demo registry.
export default function CounterfeitPanel({ medicineNames }) {
  const [form, setForm] = useState({ medicine: '', batch: '', manufacturer: '' })
  const [result, setResult] = useState(null)
  const [error, setError] = useState('')
  const [busy, setBusy] = useState(false)

  const set = (key) => (e) => setForm({ ...form, [key]: e.target.value })

  async function submit(e) {
    e?.preventDefault()
    setError('')
    setResult(null)
    if (!form.medicine.trim() || !form.batch.trim() || !form.manufacturer.trim())
      return setError('Please fill in the medicine name, batch number and manufacturer.')
    setBusy(true)
    try {
      setResult(await api.counterfeit(form.medicine.trim(), form.batch.trim(), form.manufacturer.trim()))
    } catch (err) {
      setError(err.message)
    } finally {
      setBusy(false)
    }
  }

  const rec = result?.registryRecord
  // The registry card already shows the dates, so hide those two repeated notes.
  const extraNotes = (result?.notes || []).filter((n) => !/^(Manufactured|Expires)/.test(n))

  return (
    <div className="space-y-5">
      <Card
        title="Counterfeit Detection"
        subtitle="Enter the details printed on the pack. They are compared with the local demo registry (fictional manufacturers)."
      >
        <form onSubmit={submit} className="space-y-4">
          <div className="grid gap-4 sm:grid-cols-3">
            <div>
              <label className="field-label" htmlFor="cf-medicine">
                Medicine name
              </label>
              <input id="cf-medicine" className="field-input" list="cf-medicines" value={form.medicine} onChange={set('medicine')} placeholder="e.g. Paracetamol" />
              <datalist id="cf-medicines">
                {medicineNames.map((n) => (
                  <option key={n} value={n} />
                ))}
              </datalist>
            </div>
            <div>
              <label className="field-label" htmlFor="cf-batch">
                Batch number
              </label>
              <input id="cf-batch" className="field-input font-mono" value={form.batch} onChange={set('batch')} placeholder="e.g. PCM-2501-A12" />
            </div>
            <div>
              <label className="field-label" htmlFor="cf-maker">
                Manufacturer
              </label>
              <input id="cf-maker" className="field-input" value={form.manufacturer} onChange={set('manufacturer')} placeholder="e.g. Sunrise Pharma Pvt Ltd" />
            </div>
          </div>
          <div className="flex flex-wrap items-center gap-4">
            <button className="btn-primary" disabled={busy}>
              Verify pack
            </button>
            <ExampleButtons
              examples={EXAMPLES}
              onPick={(ex) => {
                setForm({ medicine: ex.medicine, batch: ex.batch, manufacturer: ex.manufacturer })
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

          {result.checks?.length > 0 && (
            <div className="mt-4">
              <div className="mb-3 flex items-center gap-3">
                <div className="h-2 flex-1 overflow-hidden rounded-full bg-slate-200">
                  <div
                    className={`h-full ${result.matchPercent === 100 ? 'bg-emerald-500' : result.matchPercent >= 50 ? 'bg-amber-500' : 'bg-red-500'}`}
                    style={{ width: `${result.matchPercent}%` }}
                  />
                </div>
                <span className="text-sm font-semibold text-slate-700">{result.matchPercent}% match</span>
              </div>
              <table className="w-full text-left text-sm">
                <thead className="border-b border-slate-200 text-xs uppercase tracking-wide text-slate-500">
                  <tr>
                    <th className="py-2 pr-3">Field</th>
                    <th className="py-2 pr-3">You entered</th>
                    <th className="py-2 pr-3">Registry says</th>
                    <th className="py-2">Result</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-100">
                  {result.checks.map((c) => (
                    <tr key={c.field}>
                      <td className="py-2 pr-3 font-medium text-slate-800">{c.field}</td>
                      <td className="py-2 pr-3 font-mono text-xs">{c.provided}</td>
                      <td className="py-2 pr-3 font-mono text-xs">{c.expected}</td>
                      <td className="py-2">
                        <Badge value={c.match ? 'VERIFIED' : 'MISMATCH'} label={c.match ? 'Match' : 'Differs'} />
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}

          {rec && (
            <dl className="mt-4 grid gap-2 rounded-xl bg-slate-50 p-4 text-sm sm:grid-cols-2">
              {[
                ['Registered medicine', rec.medicine],
                ['Manufacturer', rec.manufacturer],
                ['Manufactured', rec.mfgDate],
                ['Expires', rec.expiryDate],
                ['Registry status', rec.status],
              ].map(([k, v]) => (
                <div key={k} className="flex justify-between gap-3">
                  <dt className="text-slate-500">{k}</dt>
                  <dd className="font-medium text-slate-800">{v}</dd>
                </div>
              ))}
            </dl>
          )}

          {extraNotes.length > 0 && (
            <ul className="mt-3 list-disc space-y-1 pl-5 text-sm text-slate-600">
              {extraNotes.map((n, i) => (
                <li key={i}>{n}</li>
              ))}
            </ul>
          )}
          <Disclaimer>{result.disclaimer}</Disclaimer>
        </Card>
      )}
    </div>
  )
}
