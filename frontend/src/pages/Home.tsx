import { NavLink } from 'react-router-dom'

const CheckIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="20 6 9 17 4 12" />
  </svg>
)

const ActivityIcon = () => (
  <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="22 12 18 12 15 21 9 3 6 12 2 12" />
  </svg>
)

const LayersIcon = () => (
  <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
    <polygon points="12 2 2 7 12 12 22 7 12 2" />
    <polyline points="2 17 12 22 22 17" />
    <polyline points="2 12 12 17 22 12" />
  </svg>
)

const SlidersIcon = () => (
  <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
    <line x1="4" y1="21" x2="4" y2="14" /><line x1="4" y1="10" x2="4" y2="3" />
    <line x1="12" y1="21" x2="12" y2="12" /><line x1="12" y1="8" x2="12" y2="3" />
    <line x1="20" y1="21" x2="20" y2="16" /><line x1="20" y1="12" x2="20" y2="3" />
    <line x1="1" y1="14" x2="7" y2="14" /><line x1="9" y1="8" x2="15" y2="8" /><line x1="17" y1="16" x2="23" y2="16" />
  </svg>
)

const stats = [
  { label: 'Distributions Linux supportées', value: '17', mono: true },
  { label: 'Règles disponibles', value: '4 200+', mono: true },
  { label: 'Référentiels pris en charge', value: 'CIS · DISA STIG · etc.', mono: false },
  { label: 'Durée moyenne d’un scan', value: '≈ 10 s', mono: true },
]

const features = [
  { icon: <SlidersIcon />, title: 'Profils personnalisables', description: 'Partez d’un profil existant et adaptez les règles à votre contexte, ou créez votre propre sélection.' },
  { icon: <ActivityIcon />, title: 'Suivi en temps réel', description: 'Visualisez l’avancement de l’audit règle par règle et consultez les résultats dès leur évaluation.' },
  { icon: <LayersIcon />, title: 'Multi-benchmarks', description: 'Évaluez votre système avec les référentiels de conformité les plus adaptés à vos exigences.' },
]

export function Home() {
  return (
    <div>
      <section className="relative max-w-5xl mx-auto px-6 pt-24 pb-20 lg:pt-32 lg:pb-28 text-center">
        <div className="absolute inset-0 pointer-events-none" style={{ backgroundImage: 'linear-gradient(rgba(139,92,246,0.04) 1px, transparent 1px), linear-gradient(90deg, rgba(139,92,246,0.04) 1px, transparent 1px)', backgroundSize: '40px 40px' }} />
        <div className="relative flex flex-col items-center">
          <h1 className="text-4xl lg:text-6xl font-bold leading-tight tracking-tight text-[#1E1B29] mb-6">
            Auditez la <span className="text-[#8B5CF6]">conformité</span><br />
            et la <span className="text-[#A78BFA]">sécurité</span> de vos machines
          </h1>
          <p className="text-[#6B7280] text-lg leading-relaxed mb-9 max-w-2xl">
            SecureConfig est une plateforme conçue pour auditer les systèmes Linux avec les référentiels adaptés à vos exigences de conformité et de sécurité.
          </p>
          <div className="flex flex-wrap justify-center gap-3">
            <NavLink to="/scan" className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-semibold text-sm transition-all duration-150 text-white" style={{ backgroundColor: '#8B5CF6' }} onMouseEnter={e => (e.currentTarget.style.backgroundColor = '#7C3AED')} onMouseLeave={e => (e.currentTarget.style.backgroundColor = '#8B5CF6')}>
              Lancer un scan
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round"><line x1="5" y1="12" x2="19" y2="12" /><polyline points="12 5 19 12 12 19" /></svg>
            </NavLink>
            <NavLink to="/about" className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-medium text-sm text-[#6B7280] hover:text-[#1E1B29]" style={{ border: '1px solid rgba(15,23,42,0.1)' }}>
              En savoir plus
            </NavLink>
          </div>
        </div>
      </section>

      <section className="border-y" style={{ borderColor: 'rgba(15,23,42,0.06)', backgroundColor: '#FAF9FD' }}>
        <div className="max-w-5xl mx-auto px-6 py-10">
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6 text-center">
            {stats.map(s => <div key={s.label} className="flex flex-col gap-1"><span className="text-2xl font-bold text-[#1E1B29]" style={s.mono ? { fontFamily: "'JetBrains Mono', monospace" } : {}}>{s.value}</span><span className="text-xs text-[#8B8794]">{s.label}</span></div>)}
          </div>
        </div>
      </section>

      <section className="max-w-6xl mx-auto px-6 py-20">
        <div className="mb-12 text-center"><p className="text-xs font-semibold uppercase tracking-widest text-[#8B8794] mb-3" style={{ fontFamily: "'JetBrains Mono', monospace" }}>Fonctionnalités</p><h2 className="text-2xl font-bold text-[#1E1B29]">Un audit clair et maîtrisé</h2></div>
        <div className="grid md:grid-cols-3 gap-5">
          {features.map(f => <div key={f.title} className="rounded-xl p-6 border" style={{ backgroundColor: '#FAF9FD', borderColor: 'rgba(15,23,42,0.07)' }}><div className="w-10 h-10 rounded-lg flex items-center justify-center mb-4" style={{ backgroundColor: 'rgba(139,92,246,0.12)', color: '#8B5CF6', border: '1px solid rgba(139,92,246,0.2)' }}>{f.icon}</div><h3 className="font-semibold text-[#1E1B29] mb-2">{f.title}</h3><p className="text-sm text-[#8B8794] leading-relaxed">{f.description}</p></div>)}
        </div>
      </section>

      <section className="max-w-6xl mx-auto px-6 pb-20">
        <div className="rounded-2xl p-10 lg:p-14 flex flex-col lg:flex-row items-start lg:items-center justify-between gap-6" style={{ backgroundColor: '#FAF9FD', border: '1px solid rgba(139,92,246,0.18)' }}>
          <div><h2 className="text-2xl font-bold text-[#1E1B29] mb-2">Prêt à auditer votre configuration ?</h2><p className="text-[#8B8794] text-sm max-w-md">Choisissez votre système, sélectionnez un profil et lancez l’évaluation.</p></div>
          <NavLink to="/scan" className="flex-shrink-0 inline-flex items-center gap-2 px-7 py-3.5 rounded-lg font-semibold text-sm text-white" style={{ backgroundColor: '#8B5CF6' }}><CheckIcon />Démarrer l'audit</NavLink>
        </div>
      </section>
    </div>
  )
}
