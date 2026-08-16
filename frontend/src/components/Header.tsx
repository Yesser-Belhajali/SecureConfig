import { NavLink } from 'react-router-dom'
import { useEffect, useState } from 'react'

const ShieldIcon = () => (
  <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
    <polyline points="9 12 11 14 15 10" />
  </svg>
)

const MenuIcon = () => (
  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="3" y1="6" x2="21" y2="6" />
    <line x1="3" y1="12" x2="21" y2="12" />
    <line x1="3" y1="18" x2="21" y2="18" />
  </svg>
)

const CloseIcon = () => (
  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" />
    <line x1="6" y1="6" x2="18" y2="18" />
  </svg>
)

export function Header() {
  const [open, setOpen] = useState(false)
  const [isHeaderHidden, setIsHeaderHidden] = useState(false)

  useEffect(() => {
    let lastScrollY = window.scrollY
    const onScroll = () => {
      const currentScrollY = window.scrollY
      if (!open) setIsHeaderHidden(currentScrollY > 96 && currentScrollY > lastScrollY)
      lastScrollY = currentScrollY
    }
    window.addEventListener('scroll', onScroll, { passive: true })
    return () => window.removeEventListener('scroll', onScroll)
  }, [open])

  const linkClass = ({ isActive }: { isActive: boolean }) =>
    `text-sm font-medium transition-colors duration-150 ${
      isActive
        ? 'text-[#A78BFA]'
        : 'text-[#94A3B8] hover:text-[#E2E8F0]'
    }`

  return (
    <header
      className="sticky top-0 z-50 border-b transition-transform duration-300 ease-out"
      style={{
        backgroundColor: 'rgba(13,15,18,0.92)',
        borderColor: 'rgba(255,255,255,0.07)',
        backdropFilter: 'blur(12px)',
        WebkitBackdropFilter: 'blur(12px)',
        transform: isHeaderHidden ? 'translateY(-100%)' : 'translateY(0)',
      }}
    >
      <div className="max-w-6xl mx-auto px-6 h-14 flex items-center justify-between">
        <NavLink to="/" className="flex items-center gap-2.5 group" aria-label="SecureConfig — Accueil">
          <span className="text-[#8B5CF6] group-hover:text-[#A78BFA] transition-colors">
            <ShieldIcon />
          </span>
          <span className="font-semibold text-[#E2E8F0] tracking-tight">
            Secure<span className="text-[#8B5CF6]">Config</span>
          </span>
        </NavLink>

        <nav className="hidden md:flex items-center gap-8">
          <NavLink to="/" end className={linkClass}>Home</NavLink>
          <NavLink to="/scan" className={linkClass}>Scan</NavLink>
          <NavLink to="/about" className={linkClass}>About</NavLink>
          <NavLink
            to="/scan"
            className="text-sm font-medium px-4 py-1.5 rounded-md transition-all duration-150"
            style={{
              backgroundColor: '#8B5CF6',
              color: '#fff',
            }}
            onMouseEnter={e => (e.currentTarget.style.backgroundColor = '#7C3AED')}
            onMouseLeave={e => (e.currentTarget.style.backgroundColor = '#8B5CF6')}
          >
            Lancer un scan
          </NavLink>
        </nav>

        <div className="md:hidden flex items-center gap-3">
          <button className="text-[#94A3B8] hover:text-[#E2E8F0] transition-colors" onClick={() => setOpen(o => !o)} aria-label="Ouvrir le menu">{open ? <CloseIcon /> : <MenuIcon />}</button>
        </div>
      </div>

      {open && (
        <div
          className="md:hidden border-t px-6 py-4 flex flex-col gap-4"
          style={{ borderColor: 'rgba(255,255,255,0.07)', backgroundColor: '#0D0F12' }}
        >
          <NavLink to="/" end className={linkClass} onClick={() => setOpen(false)}>Home</NavLink>
          <NavLink to="/scan" className={linkClass} onClick={() => setOpen(false)}>Scan</NavLink>
          <NavLink to="/about" className={linkClass} onClick={() => setOpen(false)}>About</NavLink>
          <NavLink
            to="/scan"
            className="text-sm font-medium px-4 py-2 rounded-md text-center transition-all duration-150"
            style={{ backgroundColor: '#8B5CF6', color: '#fff' }}
            onClick={() => setOpen(false)}
          >
            Lancer un scan
          </NavLink>
        </div>
      )}
    </header>
  )
}