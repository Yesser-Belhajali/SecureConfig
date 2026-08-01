import { NavLink } from 'react-router-dom'

export function NotFound() {
  return (
    <div className="flex flex-col items-center justify-center min-h-[50vh] px-6 text-center">
      <p
        className="text-6xl font-bold mb-4"
        style={{ color: '#8B5CF6', fontFamily: "'JetBrains Mono', monospace" }}
      >
        404
      </p>
      <h1 className="text-xl font-semibold text-[#E2E8F0] mb-3">Page introuvable</h1>
      <p className="text-[#64748B] mb-8">Cette route n'existe pas dans le datastream.</p>
      <NavLink
        to="/"
        className="px-6 py-3 rounded-lg font-medium text-sm text-white transition-all duration-150"
        style={{ backgroundColor: '#8B5CF6' }}
        onMouseEnter={e => (e.currentTarget.style.backgroundColor = '#7C3AED')}
        onMouseLeave={e => (e.currentTarget.style.backgroundColor = '#8B5CF6')}
      >
        Retour à l'accueil
      </NavLink>
    </div>
  )
}
