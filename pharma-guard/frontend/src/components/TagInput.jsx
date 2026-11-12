import { useId, useState } from 'react'

// Lets the user type several medicine names. Press Enter or comma to add a name,
// Backspace on an empty box to remove the last one. `suggestions` fills a drop-down list.
export default function TagInput({ label, values, onChange, suggestions = [], placeholder, max = 10, hint }) {
  const [draft, setDraft] = useState('')
  const listId = useId()

  function addDraft(text) {
    const parts = text
      .split(/[,;\n]/)
      .map((p) => p.trim())
      .filter(Boolean)
    if (parts.length === 0) return
    const next = [...values]
    for (const part of parts) {
      if (next.length >= max) break
      if (!next.some((v) => v.toLowerCase() === part.toLowerCase())) next.push(part)
    }
    onChange(next)
    setDraft('')
  }

  function onKeyDown(e) {
    if (e.key === 'Enter' || e.key === ',') {
      e.preventDefault()
      addDraft(draft)
    } else if (e.key === 'Backspace' && draft === '' && values.length > 0) {
      onChange(values.slice(0, -1))
    }
  }

  return (
    <div>
      <label className="field-label">{label}</label>
      <div className="flex flex-wrap items-center gap-2 rounded-lg border border-slate-300 bg-white px-2 py-2 shadow-sm focus-within:border-teal-500 focus-within:ring-2 focus-within:ring-teal-500/30">
        {values.map((v) => (
          <span key={v} className="inline-flex items-center gap-1 rounded-full bg-teal-50 py-1 pl-3 pr-1 text-sm font-medium text-teal-800">
            {v}
            <button
              type="button"
              aria-label={`Remove ${v}`}
              className="flex h-5 w-5 items-center justify-center rounded-full text-teal-700 hover:bg-teal-200"
              onClick={() => onChange(values.filter((x) => x !== v))}
            >
              ×
            </button>
          </span>
        ))}
        <input
          list={listId}
          value={draft}
          onChange={(e) => setDraft(e.target.value)}
          onKeyDown={onKeyDown}
          onBlur={() => addDraft(draft)}
          placeholder={values.length === 0 ? placeholder : 'Add another...'}
          className="min-w-[10rem] flex-1 border-0 bg-transparent px-1 py-1 text-sm focus:outline-none focus:ring-0"
        />
        <datalist id={listId}>
          {suggestions.map((s) => (
            <option key={s} value={s} />
          ))}
        </datalist>
      </div>
      <p className="mt-1 text-xs text-slate-500">{hint || 'Press Enter or comma after each name.'}</p>
    </div>
  )
}
