import { useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import { useScanFlow } from "../model/useScanFlow";
import { distributions } from "../model/distributions";

export function ScanScreen() {
  const navigate = useNavigate();
  const [search, setSearch] = useState("");
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

  const filteredDistributions = distributions.filter((d) =>
    d.label.toLowerCase().includes(search.toLowerCase())
  );

  return (
    <>
      {screen === "profile" && (
        <button
          type="button"
          className="group inline-flex items-center gap-3 rounded-xl border border-[#A78BFA]/45 bg-[#8B5CF6]/10 px-5 py-3 text-base font-semibold text-[#DDD6FE] shadow-[0_10px_28px_rgba(76,29,149,.18)] transition-all hover:-translate-y-0.5 hover:border-[#C4B5FD] hover:bg-[#8B5CF6]/20 hover:text-white focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-[#C4B5FD] mt-8 ml-6"
          onClick={goToSystemScreen}
        >
          <span className="text-xl leading-none transition-transform group-hover:-translate-x-1" aria-hidden="true">←</span>
          Modifier le système
        </button>
      )}
      <div className="max-w-5xl mx-auto px-6 py-16">

        <header className="mb-16"><h1 className="text-4xl lg:text-5xl font-bold tracking-tight text-[#E2E8F0]">Choisissez votre distribution</h1></header>

        {screen === "system" && <section>
          <div className="flex items-center justify-between gap-4 mb-10 flex-wrap">
            <div className="relative">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" className="absolute left-3 top-1/2 -translate-y-1/2 text-[#64748B] pointer-events-none">
                <circle cx="11" cy="11" r="8" /><line x1="21" y1="21" x2="16.65" y2="16.65" />
              </svg>
              <input
                type="text"
                placeholder="Rechercher une distribution..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                className="pl-9 pr-3.5 py-2 rounded-lg text-sm w-64 bg-[rgba(19,23,30,0.62)] border border-white/10 text-[#E2E8F0] placeholder:text-[#64748B] focus:outline-none focus:border-[#A78BFA]/60 transition-colors"
              />
            </div>
          </div>

          <div className="lg:grid lg:grid-cols-[minmax(0,1fr)_280px] lg:gap-12 items-start">
            <div>
              {filteredDistributions.length === 0 ? (
                <p className="text-sm text-[#64748B] py-8 text-center">Aucune distribution ne correspond à « {search} ».</p>
              ) : (
                <div className="divide-y divide-white/[0.06]">
                  {filteredDistributions.map(item => {
                    const isDistroSelected = distributionId === item.id;
                    const hasVersions = item.versions.length > 0;
                    const versionsList = hasVersions ? item.versions : ['Version non précisée'];
                    return (
                      <section
                        key={item.id}
                        aria-labelledby={'distribution-' + item.id}
                        className="py-7 first:pt-0 last:pb-0"
                      >
                        <h3
                          id={'distribution-' + item.id}
                          className="text-[1.65rem] sm:text-[1.9rem] font-bold tracking-tight leading-none mb-3 transition-colors duration-200"
                          style={{
                            fontFamily: "'Outfit', sans-serif",
                            color: isDistroSelected ? item.accent : '#E2E8F0',
                          }}
                        >
                          {item.label}
                        </h3>

                        <div
                          className="h-px w-full mb-4 transition-colors duration-200"
                          style={{ backgroundColor: isDistroSelected ? `${item.accent}55` : 'rgba(255,255,255,0.10)' }}
                        />

                        <div className="flex flex-wrap gap-2">
                          {versionsList.map(itemVersion => {
                            const actualValue = hasVersions ? itemVersion : '';
                            const isSelected = distributionId === item.id && version === actualValue;
                            return (
                              <button
                                key={itemVersion}
                                type="button"
                                className="min-h-[42px] px-4 py-2.5 rounded-md text-xs font-semibold transition-colors"
                                style={{
                                  backgroundColor: isSelected ? '#8B5CF6' : 'transparent',
                                  color: isSelected ? '#fff' : '#94A3B8',
                                  border: isSelected ? '1px solid #A78BFA' : '1px solid rgba(255,255,255,0.13)',
                                }}
                                onClick={() => toggleVersion(item.id, actualValue)}
                              >
                                {itemVersion}
                              </button>
                            );
                          })}
                        </div>
                      </section>
                    );
                  })}
                </div>
              )}
            </div>

            <aside
              className="relative overflow-hidden mt-10 lg:mt-0 lg:sticky lg:top-20 rounded-2xl border p-6"
              style={{
                background: canContinue ? 'linear-gradient(135deg, #1b1730, #14161f)' : '#13171E',
                borderColor: canContinue ? 'rgba(167,139,250,.35)' : 'rgba(255,255,255,0.08)',
                boxShadow: canContinue ? '0 18px 45px rgba(76,29,149,.28)' : undefined,
              }}
            >
              {canContinue ? <>
                <div className="absolute top-0 inset-x-0 h-px bg-gradient-to-r from-transparent via-[#C4B5FD] to-transparent" />
                <div className="flex items-center gap-3 mb-5">
                  <svg width="42" height="36" viewBox="0 0 68 56" fill="none" aria-hidden="true"><path d="M4 14c0-3.3 2.7-6 6-6h16l6 6h26c3.3 0 6 2.7 6 6v24c0 3.3-2.7 6-6 6H10c-3.3 0-6-2.7-6-6V14Z" fill="#8B5CF6" fillOpacity="0.18" stroke="#A78BFA" strokeWidth="1.5" /><path d="M4 22h60" stroke="#A78BFA" strokeWidth="1.5" /></svg>
                  <div><p className="text-xs uppercase tracking-widest text-[#A78BFA]" style={{ fontFamily: "'JetBrains Mono', monospace" }}>Benchmark prêt</p><p className="text-sm font-semibold text-[#F1F5F9] mt-1">Configuration sélectionnée</p></div>
                </div>
                <dl className="space-y-3 mb-6"><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(255,255,255,.035)', borderColor: 'rgba(255,255,255,.08)' }}><dt className="text-[10px] uppercase tracking-wider text-[#64748B]">Distribution</dt><dd className="text-sm font-semibold text-[#E2E8F0] mt-1">{selectedDistribution?.label}</dd></div><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(139,92,246,.08)', borderColor: 'rgba(167,139,250,.16)' }}><dt className="text-[10px] uppercase tracking-wider text-[#64748B]">Version</dt><dd className="text-sm font-semibold text-[#C4B5FD] mt-1">{version || '—'}</dd></div></dl>
                <div className="space-y-2"><button type="button" className="w-full py-2.5 rounded-lg text-sm font-medium text-[#94A3B8] hover:text-[#E2E8F0] hover:bg-white/5 transition-colors" style={{ border: '1px solid rgba(255,255,255,0.12)' }} onClick={resetSystemChoice}>Modifier le choix</button><button type="button" className="w-full py-2.5 rounded-lg text-sm font-semibold text-white shadow-lg transition-all hover:-translate-y-0.5" style={{ background: 'linear-gradient(135deg, #8b5cf6, #7c3aed)', boxShadow: '0 8px 18px rgba(109,40,217,.32)' }} onClick={goToProfileScreen}>Choisir un profil →</button></div>
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
                {profiles.map(profile => {
                  const isSelected = selectedProfile === profile.id;
                  return (
                    <label key={profile.id} className="flex items-start gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50" style={{ backgroundColor: isSelected ? 'rgba(139,92,246,0.13)' : 'rgba(19,23,30,0.62)', borderColor: isSelected ? 'rgba(167,139,250,0.65)' : 'rgba(255,255,255,0.10)', boxShadow: isSelected ? '0 10px 28px rgba(76,29,149,.18)' : 'none' }}>
                      <input type="radio" name="profile" value={profile.id} checked={isSelected} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6] mt-0.5" />
                      <span className="flex-1 min-w-0">
                        <span className="block text-sm font-semibold text-[#E2E8F0]">{profile.title}</span>
                        <span className="block text-xs text-[#64748B] mt-1 leading-relaxed">
                          {isSelected && profile.description ? profile.description : "Profil de conformité prêt à personnaliser"}
                        </span>
                      </span>
                      <span className="text-[#A78BFA] text-sm mt-0.5" aria-hidden="true">{isSelected ? '✓' : '→'}</span>
                    </label>
                  );
                })}
              </div>

              {tailoringProfiles.length > 0 && (
                <>
                  <p className="text-xs uppercase tracking-widest text-[#A78BFA] mb-3" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
                    Vos profils personnalisés
                  </p>
                  <div className="space-y-2 mb-8">
                    {tailoringProfiles.map(profile => {
                      const isSelected = selectedProfile === profile.id;
                      return (
                        <label key={profile.id} className="flex items-start gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50" style={{ backgroundColor: isSelected ? 'rgba(139,92,246,0.13)' : 'rgba(19,23,30,0.62)', borderColor: isSelected ? 'rgba(167,139,250,0.65)' : 'rgba(255,255,255,0.10)', boxShadow: isSelected ? '0 10px 28px rgba(76,29,149,.18)' : 'none' }}>
                          <input type="radio" name="profile" value={profile.id} checked={isSelected} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6] mt-0.5" />
                          <span className="flex-1 min-w-0">
                            <span className="block text-sm font-semibold text-[#E2E8F0]">{profile.title}</span>
                            {isSelected && profile.description && (
                              <span className="block text-xs text-[#94A3B8] mt-1.5 leading-relaxed">{profile.description}</span>
                            )}
                            {profile.extends && (
                              <span className="block text-xs text-[#64748B] mt-1">Étend {profile.extends}</span>
                            )}
                          </span>
                          <span className="text-[#A78BFA] text-sm mt-0.5" aria-hidden="true">{isSelected ? '✓' : '→'}</span>
                        </label>
                      );
                    })}
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
    </>
  );
}