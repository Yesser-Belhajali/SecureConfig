import { Link, useNavigate } from "react-router-dom";
import { useScanFlow } from "../model/useScanFlow";
import { distributions } from "../model/distributions";

function DistributionVisual({ name, accent }: { name: string; accent: string }) {
  const initials = name.replace(/[^A-Z]/g, "").slice(0, 2) || name.slice(0, 2).toUpperCase();
  return (
    <svg width="58" height="58" viewBox="0 0 58 58" role="img" aria-label={"Illustration " + name}>
      <rect x="2" y="2" width="54" height="54" rx="16" fill={accent} fillOpacity="0.16" stroke={accent} strokeOpacity="0.45" />
      <path d="M17 35.5V23.2c0-2.32 1.88-4.2 4.2-4.2h15.6c2.32 0 4.2 1.88 4.2 4.2v12.3c0 2.32-1.88 4.2-4.2 4.2H21.2c-2.32 0-4.2-1.88-4.2-4.2Z" fill={accent} fillOpacity="0.25" stroke={accent} strokeWidth="1.5" />
      <path d="M23 39.7v3.5m12-3.5v3.5M19 43.2h20" stroke={accent} strokeWidth="1.5" strokeLinecap="round" />
      <text x="29" y="32" textAnchor="middle" fill={accent} fontSize="11" fontWeight="700" fontFamily="Inter, sans-serif">{initials}</text>
    </svg>
  );
}

export function ScanScreen() {
  const navigate = useNavigate();
  const {
    screen,
    distributionId,
    version,
    selectedDistribution,
    benchmarkId,
    canContinue,
    selectedProfile,
    profiles,
    tailoringProfiles,
    loadingProfiles,
    profilesError,
    toggleVersion,
    resetSystemChoice,
    goToProfileScreen,
    goToSystemScreen,
    setSelectedProfile,
  } = useScanFlow();

  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
    {screen === "profile" && (
      <button
        type="button"
        className="group inline-flex items-center gap-3 rounded-xl border border-[#A78BFA]/45 bg-[#8B5CF6]/10 px-5 py-3 text-base font-semibold text-[#DDD6FE] shadow-[0_10px_28px_rgba(76,29,149,.18)] transition-all hover:-translate-y-0.5 hover:border-[#C4B5FD] hover:bg-[#8B5CF6]/20 hover:text-white focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-[#C4B5FD] mb-10 -ml-2"
        onClick={goToSystemScreen}
      >
        <span className="text-xl leading-none transition-transform group-hover:-translate-x-1" aria-hidden="true">←</span>
        Modifier le système
      </button>
    )}

      <header className="mb-16"><h1 className="text-4xl lg:text-5xl font-bold tracking-tight text-[#E2E8F0]">Choisissez votre distribution</h1></header>

      {screen === "system" && <section>
        <h2 className="text-xl font-semibold text-[#E2E8F0] mb-10">Distributions prises en charge</h2>

        <div className="lg:grid lg:grid-cols-[minmax(0,1fr)_280px] lg:gap-12 items-start">
          <div className="space-y-10">
            {distributions.map(item => <section key={item.id} aria-labelledby={'distribution-' + item.id}>
              <div className="flex items-center gap-4 rounded-xl border p-4 transition-all duration-200 hover:-translate-y-0.5" style={{ borderColor: distributionId === item.id ? `${item.accent}88` : 'rgba(255,255,255,0.07)', backgroundColor: distributionId === item.id ? `${item.accent}12` : 'rgba(19,23,30,0.52)' }}><DistributionVisual name={item.label} accent={item.accent} /><div className="min-w-0"><p className="text-[10px] uppercase tracking-[0.16em] text-[#64748B] mb-1">Système compatible</p><h3 id={'distribution-' + item.id} className="text-xl font-semibold tracking-tight" style={{ color: distributionId === item.id ? '#C4B5FD' : '#E2E8F0' }}>{item.label}</h3></div></div>
              <div className="h-px mt-4 mb-4" style={{ backgroundColor: 'rgba(255,255,255,0.10)' }} />
              <div className="flex flex-wrap gap-2">
                {item.versions.length > 0 ? item.versions.map(itemVersion => {
                  const isSelected = distributionId === item.id && version === itemVersion;
                  return <button key={itemVersion} type="button" className="px-3.5 py-2 rounded-md text-sm font-medium transition-colors" style={{ backgroundColor: isSelected ? '#8B5CF6' : 'transparent', color: isSelected ? '#fff' : '#94A3B8', border: isSelected ? '1px solid #A78BFA' : '1px solid rgba(255,255,255,0.13)' }} onClick={() => toggleVersion(item.id, itemVersion)}>{itemVersion}</button>;
                }) : (() => {
                  const isSelected = distributionId === item.id;
                  return <button type="button" className="px-3.5 py-2 rounded-md text-sm font-medium transition-colors" style={{ backgroundColor: isSelected ? '#8B5CF6' : 'transparent', color: isSelected ? '#fff' : '#94A3B8', border: isSelected ? '1px solid #A78BFA' : '1px solid rgba(255,255,255,0.13)' }} onClick={() => toggleVersion(item.id, '')}>Version non précisée</button>;
                })()}
              </div>
            </section>)}
          </div>

          <aside className="relative overflow-hidden mt-10 lg:mt-0 lg:sticky lg:top-20 rounded-2xl border p-6 shadow-2xl" style={{ background: canContinue ? 'linear-gradient(145deg, #1B1830 0%, #121720 72%)' : '#13171E', borderColor: canContinue ? 'rgba(167,139,250,0.58)' : 'rgba(255,255,255,0.08)', boxShadow: canContinue ? '0 18px 45px rgba(76,29,149,.18)' : undefined }}>
            {canContinue ? <>
              <div className="absolute top-0 inset-x-0 h-px bg-gradient-to-r from-transparent via-[#C4B5FD] to-transparent" />
              <div className="flex items-center gap-3 mb-5">
                <svg width="42" height="36" viewBox="0 0 68 56" fill="none" aria-hidden="true"><path d="M4 14c0-3.3 2.7-6 6-6h16l6 6h26c3.3 0 6 2.7 6 6v24c0 3.3-2.7 6-6 6H10c-3.3 0-6-2.7-6-6V14Z" fill="#8B5CF6" fillOpacity="0.18" stroke="#A78BFA" strokeWidth="1.5" /><path d="M4 22h60" stroke="#A78BFA" strokeWidth="1.5" /></svg>
                <div><p className="text-xs uppercase tracking-widest text-[#A78BFA]" style={{ fontFamily: "'JetBrains Mono', monospace" }}>Benchmark prêt</p><p className="text-sm font-semibold text-[#F1F5F9] mt-1">Configuration sélectionnée</p></div>
              </div>
              <dl className="space-y-3 mb-6"><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(255,255,255,.035)', borderColor: 'rgba(255,255,255,.08)' }}><dt className="text-[10px] uppercase tracking-wider text-[#64748B]">Distribution</dt><dd className="text-sm font-semibold text-[#E2E8F0] mt-1">{selectedDistribution?.label}</dd></div><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(139,92,246,.08)', borderColor: 'rgba(167,139,250,.16)' }}><dt className="text-[10px] uppercase tracking-wider text-[#64748B]">Version</dt><dd className="text-sm font-semibold text-[#C4B5FD] mt-1">{version || '—'}</dd></div></dl>
              <div className="space-y-2"><button type="button" className="w-full py-2.5 rounded-lg text-sm font-medium text-[#94A3B8] hover:text-[#E2E8F0] hover:bg-white/5 transition-colors" style={{ border: '1px solid rgba(255,255,255,0.12)' }} onClick={resetSystemChoice}>Modifier le choix</button><button type="button" className="w-full py-2.5 rounded-lg text-sm font-semibold text-white shadow-lg transition-all hover:-translate-y-0.5 hover:bg-[#7C3AED]" style={{ backgroundColor: '#8B5CF6', boxShadow: '0 10px 20px rgba(109,40,217,.28)' }} onClick={goToProfileScreen}>Choisir un profil →</button></div>
            </> : <div><p className="text-sm font-medium text-[#E2E8F0]">Aucun système sélectionné</p><p className="text-xs text-[#64748B] leading-relaxed mt-2">Choisissez une version dans la liste pour créer votre dossier de scan.</p></div>}
          </aside>
        </div>
      </section>}

      {screen === "profile" && <section className="pt-2">
        <h2 className="text-2xl font-bold text-[#E2E8F0] mb-2">Sélectionner un profil</h2>
        <p className="text-sm text-[#64748B] mb-6">Profil appliqué à {selectedDistribution?.label} {version}.</p>

        {loadingProfiles && <p className="text-sm text-[#64748B] mb-6">Chargement des profils...</p>}
        {profilesError && <p className="text-sm text-red-400 mb-6">Erreur : {profilesError}</p>}

        {!loadingProfiles && !profilesError && (
          <>
            <div className="space-y-2 mb-6">
              {profiles.map(profile => (
                <label key={profile.id} className="flex items-center gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50" style={{ backgroundColor: selectedProfile === profile.id ? 'rgba(139,92,246,0.13)' : 'rgba(19,23,30,0.62)', borderColor: selectedProfile === profile.id ? 'rgba(167,139,250,0.65)' : 'rgba(255,255,255,0.10)', boxShadow: selectedProfile === profile.id ? '0 10px 28px rgba(76,29,149,.18)' : 'none' }}>
                  <input type="radio" name="profile" value={profile.id} checked={selectedProfile === profile.id} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6]" />
                  <span className="flex-1 min-w-0"><span className="block text-sm font-semibold text-[#E2E8F0]">{profile.title}</span><span className="block text-xs text-[#64748B] mt-1">Profil de conformité prêt à personnaliser</span></span>
                  <span className="text-[#A78BFA] text-sm" aria-hidden="true">{selectedProfile === profile.id ? '✓' : '→'}</span>
                </label>
              ))}
            </div>

            {tailoringProfiles.length > 0 && (
              <>
                <p className="text-xs uppercase tracking-widest text-[#A78BFA] mb-3" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
                  Vos profils personnalisés
                </p>
                <div className="space-y-2 mb-8">
                  {tailoringProfiles.map(profile => (
                    <label key={profile.id} className="flex items-center gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50" style={{ backgroundColor: selectedProfile === profile.id ? 'rgba(139,92,246,0.13)' : 'rgba(19,23,30,0.62)', borderColor: selectedProfile === profile.id ? 'rgba(167,139,250,0.65)' : 'rgba(255,255,255,0.10)', boxShadow: selectedProfile === profile.id ? '0 10px 28px rgba(76,29,149,.18)' : 'none' }}>
                      <input type="radio" name="profile" value={profile.id} checked={selectedProfile === profile.id} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6]" />
                      <span className="flex-1 min-w-0">
                        <span className="block text-sm font-semibold text-[#E2E8F0]">{profile.title}</span>
                        {profile.extends && (
                          <span className="block text-xs text-[#64748B] mt-1">Étend {profile.extends}</span>
                        )}
                      </span>
                      <span className="text-[#A78BFA] text-sm" aria-hidden="true">{selectedProfile === profile.id ? '✓' : '→'}</span>
                    </label>
                  ))}
                </div>
              </>
            )}
          </>
        )}

        <div className="grid gap-3 sm:grid-cols-2">
          <button
            type="button"
            className="min-h-12 rounded-lg px-5 py-3 font-semibold text-sm shadow-lg transition-all hover:-translate-y-0.5 disabled:cursor-not-allowed disabled:shadow-none"
            style={{ backgroundColor: selectedProfile ? '#8B5CF6' : '#1A1F29', color: selectedProfile ? '#fff' : '#475569', boxShadow: selectedProfile ? '0 10px 20px rgba(109,40,217,.28)' : undefined }}
            disabled={!selectedProfile}
            onClick={() => navigate(`/benchmarks/${benchmarkId}/profiles/${selectedProfile}/view`, { state: { distributionId, version } })}
          >
            Voir profil →
          </button>

          <Link
            to={`/benchmarks/${benchmarkId}/create-profile`}
            state={{ distributionId, version }}
            className="min-h-12 rounded-lg border border-dashed border-[#A78BFA]/45 bg-[#8B5CF6]/5 px-5 py-3 text-center text-sm font-semibold text-[#C4B5FD] transition-colors hover:bg-[#8B5CF6]/12 flex items-center justify-center"
          >
            Créer votre propre profil
          </Link>
        </div>
      </section>}

      <p className="mt-12 rounded-xl border border-[#A78BFA]/25 bg-gradient-to-r from-[#8B5CF6]/12 via-[#171426] to-[#8B5CF6]/8 px-6 py-4 text-center text-sm font-medium tracking-wide text-[#C4B5FD] shadow-[0_10px_30px_rgba(76,29,149,.12)]">
        <span className="mr-2 text-base" aria-hidden="true">⌁</span>
        Aucune donnée ne quitte votre machine.
      </p>
    </div>
  );
}