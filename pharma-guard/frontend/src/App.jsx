import { useCallback, useEffect, useMemo, useState } from 'react'
import { api } from './api.js'
import SuitabilityPanel from './panels/SuitabilityPanel.jsx'
import InteractionPanel from './panels/InteractionPanel.jsx'
import CounterfeitPanel from './panels/CounterfeitPanel.jsx'
import DatabasePanel from './panels/DatabasePanel.jsx'

const TABS = [
  { id: 'suitability', label: 'Suitability', hint: 'Module 1' },
  { id: 'interaction', label: 'Interaction', hint: 'Module 2' },
  { id: 'counterfeit', label: 'Counterfeit', hint: 'Module 3' },
  { id: 'database', label: 'Database', hint: 'Data files' },
]

export default function App() {
  const [tab, setTab] = useState('suitability')
  const [db, setDb] = useState(null)
  const [online, setOnline] = useState(null) // null = checking, true / false
  const [offlineMessage, setOfflineMessage] = useState('')
  const [reloading, setReloading] = useState(false)
  const [dbError, setDbError] = useState('')

  const loadDatabase = useCallback(async () => {
    try {
      setDb(await api.database())
      setOnline(true)
      setOfflineMessage('')
    } catch (err) {
      setOnline(false)
      setOfflineMessage(err.message)
    }
  }, [])

  useEffect(() => {
    loadDatabase()
  }, [loadDatabase])

  async function reload() {
    setReloading(true)
    setDbError('')
    try {
      await api.reload()
      await loadDatabase()
    } catch (err) {
      setDbError(err.message)
    } finally {
      setReloading(false)
    }
  }

  // Names offered in the drop-down suggestions (generic names and brand names).
  const medicineNames = useMemo(
    () => (db ? db.medicines.flatMap((m) => [m.name, ...m.aliases.map((a) => a.replace(/\b\w/g, (c) => c.toUpperCase()))]) : []),
    [db],
  )
  const illnessNames = useMemo(() => (db ? db.illnesses.map((i) => i.name) : []), [db])

  return (
    <div className="min-h-screen">
      <header className="border-b border-slate-200 bg-white">
        <div className="mx-auto flex max-w-5xl flex-wrap items-center gap-4 px-4 py-4">
          <div className="flex items-center gap-3">
            <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-teal-700 text-xl text-white" aria-hidden>
              ✚
            </div>
            <div>
              <h1 className="text-xl font-bold leading-tight text-slate-900">Pharma Guard</h1>
              <p className="text-xs italic text-slate-500">&ldquo;Care begins with thoughtful choices.&rdquo;</p>
            </div>
          </div>
          <div className="ml-auto flex items-center gap-2 text-xs font-medium">
            <span
              className={`h-2.5 w-2.5 rounded-full ${online === null ? 'bg-slate-300' : online ? 'bg-emerald-500' : 'bg-red-500'}`}
              aria-hidden
            />
            <span className="text-slate-600">
              {online === null ? 'Connecting...' : online ? 'Backend connected' : 'Backend offline'}
            </span>
          </div>
        </div>
        <nav className="mx-auto flex max-w-5xl gap-1 overflow-x-auto px-4" aria-label="Modules">
          {TABS.map((t) => (
            <button
              key={t.id}
              onClick={() => setTab(t.id)}
              aria-current={tab === t.id ? 'page' : undefined}
              className={`whitespace-nowrap border-b-2 px-4 py-3 text-sm font-semibold transition ${
                tab === t.id ? 'border-teal-700 text-teal-800' : 'border-transparent text-slate-500 hover:text-slate-800'
              }`}
            >
              {t.label}
              <span className="ml-2 hidden text-xs font-normal text-slate-400 sm:inline">{t.hint}</span>
            </button>
          ))}
        </nav>
      </header>

      <main className="mx-auto max-w-5xl space-y-5 px-4 py-6">
        {online === false && (
          <div role="alert" className="rounded-xl border border-red-200 bg-red-50 px-4 py-3 text-sm text-red-800">
            <p className="font-semibold">The C++ backend is not reachable.</p>
            <p className="mt-1">{offlineMessage}</p>
            <button className="btn-ghost mt-3" onClick={loadDatabase}>
              Try again
            </button>
          </div>
        )}

        {/* All panels stay mounted so typed text and results survive tab switches. */}
        <div hidden={tab !== 'suitability'}>
          <SuitabilityPanel illnessNames={illnessNames} medicineNames={medicineNames} />
        </div>
        <div hidden={tab !== 'interaction'}>
          <InteractionPanel medicineNames={medicineNames} />
        </div>
        <div hidden={tab !== 'counterfeit'}>
          <CounterfeitPanel medicineNames={medicineNames} />
        </div>
        <div hidden={tab !== 'database'}>
          <DatabasePanel db={db} onReload={reload} reloading={reloading} error={dbError} />
        </div>
      </main>

      <footer className="mx-auto max-w-5xl px-4 pb-8 text-center text-xs text-slate-400">
        Pharma Guard - Demo data only; not for medical decisions.
      </footer>
    </div>
  )
}
