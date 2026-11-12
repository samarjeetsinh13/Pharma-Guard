import { useMemo, useState } from 'react'
import { Badge, Card } from '../components/ui.jsx'

// Describes each table of the hand-built text database: its file and its columns.
const TABLES = {
  medicines: {
    file: 'data/medicines.txt',
    columns: [['id', 'ID'], ['name', 'Name'], ['generic', 'Generic'], ['drugClass', 'Class'], ['use', 'Use'], ['aliases', 'Aliases']],
  },
  illnesses: {
    file: 'data/illnesses.txt',
    columns: [['id', 'ID'], ['name', 'Name'], ['category', 'Category'], ['description', 'Description'], ['aliases', 'Aliases']],
  },
  suitability: {
    file: 'data/suitability.txt',
    columns: [['illness', 'Illness'], ['medicine', 'Medicine'], ['verdict', 'Verdict'], ['reason', 'Reason']],
  },
  interactions: {
    file: 'data/interactions.txt',
    columns: [['medicineA', 'Medicine A'], ['medicineB', 'Medicine B'], ['severity', 'Severity'], ['effect', 'Effect'], ['advice', 'Advice']],
  },
  registry: {
    file: 'data/registry.txt',
    columns: [['medicine', 'Medicine'], ['batch', 'Batch'], ['manufacturer', 'Manufacturer'], ['mfgDate', 'Made'], ['expiryDate', 'Expires'], ['status', 'Status']],
  },
}

const BADGE_COLUMNS = new Set(['verdict', 'severity', 'status'])

function Cell({ column, value, row }) {
  if (Array.isArray(value)) return value.join(', ')
  if (BADGE_COLUMNS.has(column)) {
    const expired = column === 'status' && row.expired
    return (
      <span className="inline-flex gap-1">
        <Badge value={value} />
        {expired && <Badge value="EXPIRED" />}
      </span>
    )
  }
  return String(value ?? '')
}

// Read-only view of the hand-built database, with a "reload" button for after you edit the .txt files.
export default function DatabasePanel({ db, onReload, reloading, error }) {
  const [tab, setTab] = useState('medicines')
  const [filter, setFilter] = useState('')
  const table = TABLES[tab]

  const rows = useMemo(() => {
    const all = db?.[tab] || []
    const q = filter.trim().toLowerCase()
    if (!q) return all
    return all.filter((r) => JSON.stringify(Object.values(r)).toLowerCase().includes(q))
  }, [db, tab, filter])

  return (
    <div className="space-y-5">
      <Card
        title="Manual Database"
        subtitle="Five plain-text tables in the data/ folder. Edit them with any text editor, save, then press Reload - no recompiling needed."
      >
        <div className="flex flex-wrap items-center gap-3">
          <button className="btn-primary" onClick={onReload} disabled={reloading}>
            {reloading ? 'Reloading...' : 'Reload from files'}
          </button>
          {db && (
            <p className="text-sm text-slate-500">
              {db.stats.medicines} medicines · {db.stats.illnesses} illnesses · {db.stats.suitabilityRules} suitability rules ·{' '}
              {db.stats.interactions} interactions · {db.stats.batches} batches
            </p>
          )}
        </div>
        {error && <p className="mt-3 text-sm text-red-700">{error}</p>}
      </Card>

      <Card>
        <div className="mb-4 flex flex-wrap items-center gap-2">
          {Object.keys(TABLES).map((key) => (
            <button
              key={key}
              onClick={() => setTab(key)}
              className={`rounded-full px-3 py-1.5 text-sm font-medium transition ${
                tab === key ? 'bg-teal-700 text-white' : 'bg-slate-100 text-slate-600 hover:bg-slate-200'
              }`}
            >
              {key} <span className="opacity-70">({db?.[key]?.length ?? 0})</span>
            </button>
          ))}
          <input
            className="field-input ml-auto max-w-xs"
            placeholder="Filter rows..."
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
            aria-label="Filter rows"
          />
        </div>
        <p className="mb-3 font-mono text-xs text-slate-400">{table.file}</p>
        <div className="max-h-[32rem] overflow-auto rounded-xl border border-slate-200">
          <table className="w-full text-left text-sm">
            <thead className="sticky top-0 bg-slate-50 text-xs uppercase tracking-wide text-slate-500">
              <tr>
                {table.columns.map(([, label]) => (
                  <th key={label} className="whitespace-nowrap px-3 py-2">
                    {label}
                  </th>
                ))}
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100">
              {rows.map((row, i) => (
                <tr key={i} className="align-top hover:bg-slate-50">
                  {table.columns.map(([key]) => (
                    <td key={key} className="px-3 py-2 text-slate-700">
                      <Cell column={key} value={row[key]} row={row} />
                    </td>
                  ))}
                </tr>
              ))}
              {rows.length === 0 && (
                <tr>
                  <td colSpan={table.columns.length} className="px-3 py-6 text-center text-slate-400">
                    No rows
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </Card>
    </div>
  )
}
