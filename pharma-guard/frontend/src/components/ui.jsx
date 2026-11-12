// Small shared building blocks used by every panel.

export function Card({ title, subtitle, children, className = '' }) {
  return (
    <section className={`rounded-2xl border border-slate-200 bg-white p-5 shadow-sm ${className}`}>
      {(title || subtitle) && (
        <header className="mb-4">
          {title && <h2 className="text-lg font-semibold text-slate-900">{title}</h2>}
          {subtitle && <p className="mt-0.5 text-sm text-slate-500">{subtitle}</p>}
        </header>
      )}
      {children}
    </section>
  )
}

// Colour for each status word the backend can send back.
const TONES = {
  green: 'bg-emerald-50 text-emerald-800 ring-emerald-600/20',
  amber: 'bg-amber-50 text-amber-800 ring-amber-600/20',
  red: 'bg-red-50 text-red-800 ring-red-600/20',
  slate: 'bg-slate-100 text-slate-700 ring-slate-500/20',
  blue: 'bg-sky-50 text-sky-800 ring-sky-600/20',
}

const STATUS_TONE = {
  SUITABLE: 'green',
  CAUTION: 'amber',
  NOT_SUITABLE: 'red',
  UNKNOWN: 'slate',
  NOT_FOUND: 'slate',
  MINOR: 'blue',
  MODERATE: 'amber',
  MAJOR: 'red',
  VERIFIED: 'green',
  MISMATCH: 'red',
  RECALLED: 'red',
  EXPIRED: 'amber',
  GENUINE: 'green',
}

export function Badge({ value, label }) {
  const tone = TONES[STATUS_TONE[value] || 'slate']
  return (
    <span className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold ring-1 ring-inset ${tone}`}>
      {label || String(value).replaceAll('_', ' ')}
    </span>
  )
}

export function ErrorBox({ message }) {
  if (!message) return null
  return (
    <div role="alert" className="rounded-lg border border-red-200 bg-red-50 px-4 py-3 text-sm text-red-800">
      {message}
    </div>
  )
}

export function Spinner({ text = 'Working...' }) {
  return (
    <div className="flex items-center gap-2 text-sm text-slate-500">
      <span className="h-4 w-4 animate-spin rounded-full border-2 border-slate-300 border-t-teal-600" />
      {text}
    </div>
  )
}

// A banner colour + icon for the headline of a result.
export function Banner({ tone = 'slate', title, children }) {
  const colours = {
    green: 'border-emerald-200 bg-emerald-50 text-emerald-900',
    amber: 'border-amber-200 bg-amber-50 text-amber-900',
    red: 'border-red-200 bg-red-50 text-red-900',
    slate: 'border-slate-200 bg-slate-50 text-slate-800',
  }
  return (
    <div className={`rounded-xl border px-4 py-3 ${colours[tone]}`}>
      {title && <p className="text-sm font-bold uppercase tracking-wide">{title}</p>}
      <p className="text-sm">{children}</p>
    </div>
  )
}

export function Disclaimer({ children }) {
  return <p className="mt-4 text-xs italic text-slate-500">{children}</p>
}

export function ExampleButtons({ examples, onPick }) {
  return (
    <div className="flex flex-wrap items-center gap-2">
      <span className="text-xs font-medium uppercase tracking-wide text-slate-400">Try an example:</span>
      {examples.map((ex) => (
        <button key={ex.label} type="button" className="btn-ghost" onClick={() => onPick(ex)}>
          {ex.label}
        </button>
      ))}
    </div>
  )
}

// Maps a result "status" word from the backend to a banner colour.
const BANNER_TONE = {
  ALL_SUITABLE: 'green',
  CAUTION: 'amber',
  REVIEW_NEEDED: 'amber',
  NOT_SUITABLE_FOUND: 'red',
  ILLNESS_NOT_FOUND: 'slate',
  NO_KNOWN_INTERACTIONS: 'green',
  INTERACTIONS_FOUND: 'amber',
  NEED_MORE_MEDICINES: 'slate',
  VERIFIED: 'green',
  MISMATCH: 'red',
  RECALLED: 'red',
  EXPIRED: 'amber',
  NOT_FOUND: 'amber',
}
export const bannerTone = (status) => BANNER_TONE[status] || 'slate'
