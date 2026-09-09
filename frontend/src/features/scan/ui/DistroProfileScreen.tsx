import { useEffect, useRef, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import { useScanFlow } from "../model/useScanFlow";
import { distributions } from "../model/distributions";
import type { Profile } from "../model/types";
import { useToasts, ToastContainer } from "../../../components/Toast";

export function ScanScreen() {
  const navigate = useNavigate();
  const [search, setSearch] = useState("");
  const [confirmDeleteProfile, setConfirmDeleteProfile] = useState<Profile | null>(null);
  const { toasts, pushToast, dismissToast } = useToasts();
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
    deletingProfileId,
    deleteErrors,
    deleteProfile,
  } = useScanFlow();

  // détecte les nouvelles erreurs de suppression (deleteErrors est une map
  // profileId -> message) et les pousse en toast au lieu de les afficher
  // inline sous le profil concerné
  const prevDeleteErrorsRef = useRef<Record<string, string>>({});
  useEffect(() => {
    const prev = prevDeleteErrorsRef.current;
    for (const [profileId, message] of Object.entries(deleteErrors)) {
      if (message && prev[profileId] !== message) {
        pushToast(message);
      }
    }
    prevDeleteErrorsRef.current = deleteErrors;
  }, [deleteErrors, pushToast]);

  // même traitement pour l'erreur de chargement des profils
  useEffect(() => {
    if (profilesError) {
      pushToast(profilesError);
    }
  }, [profilesError, pushToast]);

  const filteredDistributions = distributions.filter((d) =>
    d.label.toLowerCase().includes(search.toLowerCase())
  );

  return (
    <>
      <ToastContainer toasts={toasts} onDismiss={dismissToast} />

      {screen === "profile" && (
        <button
          type="button"
          className="rules-back-button"
          onClick={goToSystemScreen}
        >
          <span aria-hidden="true">←</span>
          Modifier le système
        </button>
      )}
      <div className="max-w-5xl mx-auto px-6 py-16">

        <header className="mb-16"><h1 className="text-4xl lg:text-5xl font-bold tracking-tight text-[#1E1B29]">Choisissez votre distribution</h1></header>

        {screen === "system" && <section>
          <div className="flex items-center justify-between gap-4 mb-10 flex-wrap">
            <div className="relative">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" className="absolute left-3 top-1/2 -translate-y-1/2 text-[#8B8794] pointer-events-none">
                <circle cx="11" cy="11" r="8" /><line x1="21" y1="21" x2="16.65" y2="16.65" />
              </svg>
              <input
                type="text"
                placeholder="Rechercher une distribution..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                className="pl-9 pr-3.5 py-2 rounded-lg text-sm w-64 bg-[rgba(250,248,255,0.9)] border border-black/10 text-[#1E1B29] placeholder:text-[#8B8794] focus:outline-none focus:border-[#A78BFA]/60 transition-colors"
              />
            </div>
          </div>

          <div className="lg:grid lg:grid-cols-[minmax(0,1fr)_280px] lg:gap-12 items-start">
            <div>
              {filteredDistributions.length === 0 ? (
                <p className="text-sm text-[#8B8794] py-8 text-center">Aucune distribution ne correspond à « {search} ».</p>
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
                            color: isDistroSelected ? item.accent : '#1E1B29',
                          }}
                        >
                          {item.label}
                        </h3>

                        <div
                          className="h-px w-full mb-4 transition-colors duration-200"
                          style={{ backgroundColor: isDistroSelected ? `${item.accent}55` : 'rgba(15,23,42,0.10)' }}
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
                                  backgroundColor: isSelected ? item.accent : 'transparent',
                                  color: isSelected ? '#fff' : '#6B7280',
                                  border: isSelected ? `1px solid ${item.accent}` : '1px solid rgba(15,23,42,0.13)',
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
                background: canContinue ? 'linear-gradient(135deg, #f6f2fd, #f0eafb)' : '#FAF9FD',
                borderColor: canContinue ? 'rgba(167,139,250,.35)' : 'rgba(15,23,42,0.08)',
                boxShadow: canContinue ? '0 18px 45px rgba(76,29,149,.28)' : undefined,
              }}
            >
              {canContinue ? <>
                <div className="absolute top-0 inset-x-0 h-px bg-gradient-to-r from-transparent via-[#C4B5FD] to-transparent" />
                <div className="flex items-center gap-3 mb-5">
                  <svg width="42" height="36" viewBox="0 0 68 56" fill="none" aria-hidden="true"><path d="M4 14c0-3.3 2.7-6 6-6h16l6 6h26c3.3 0 6 2.7 6 6v24c0 3.3-2.7 6-6 6H10c-3.3 0-6-2.7-6-6V14Z" fill="#8B5CF6" fillOpacity="0.18" stroke="#A78BFA" strokeWidth="1.5" /><path d="M4 22h60" stroke="#A78BFA" strokeWidth="1.5" /></svg>
                  <div><p className="text-xs uppercase tracking-widest text-[#A78BFA]" style={{ fontFamily: "'JetBrains Mono', monospace" }}>Benchmark prêt</p><p className="text-sm font-semibold text-[#1E1B29] mt-1">Configuration sélectionnée</p></div>
                </div>
                <dl className="space-y-3 mb-6"><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(15,23,42,.035)', borderColor: 'rgba(15,23,42,.08)' }}><dt className="text-[10px] uppercase tracking-wider text-[#8B8794]">Distribution</dt><dd className="text-sm font-semibold text-[#1E1B29] mt-1">{selectedDistribution?.label}</dd></div><div className="rounded-lg border px-3 py-2.5" style={{ background: 'rgba(139,92,246,.08)', borderColor: 'rgba(167,139,250,.16)' }}><dt className="text-[10px] uppercase tracking-wider text-[#8B8794]">Version</dt><dd className="text-sm font-semibold text-[#C4B5FD] mt-1">{version || '—'}</dd></div></dl>
                <div className="space-y-2"><button type="button" className="w-full py-2.5 rounded-lg text-sm font-medium text-[#6B7280] hover:text-[#1E1B29] hover:bg-black/5 transition-colors" style={{ border: '1px solid rgba(15,23,42,0.12)' }} onClick={resetSystemChoice}>Modifier le choix</button><button type="button" className="w-full py-2.5 rounded-lg text-sm font-semibold text-white shadow-lg transition-all hover:-translate-y-0.5" style={{ background: 'linear-gradient(135deg, #8b5cf6, #7c3aed)', boxShadow: '0 8px 18px rgba(109,40,217,.32)' }} onClick={goToProfileScreen}>Choisir un profil →</button></div>
              </> : <div><p className="text-sm font-medium text-[#1E1B29]">Aucun système sélectionné</p><p className="text-xs text-[#8B8794] leading-relaxed mt-2">Choisissez une version dans la liste pour créer votre dossier de scan.</p></div>}
            </aside>
          </div>
        </section>}

        {screen === "profile" && <section className="pt-2">
          <h2 className="text-2xl font-bold text-[#1E1B29] mb-2">Sélectionner un profil</h2>
          <p className="text-sm text-[#8B8794] mb-6">Profil appliqué à {selectedDistribution?.label} {version}.</p>

          {loadingProfiles && <p className="text-sm text-[#8B8794] mb-6">Chargement des profils...</p>}

          {!loadingProfiles && !profilesError && (
            <>
              <div className="space-y-2 mb-6">
                {profiles.map(profile => {
                  const isSelected = selectedProfile === profile.id;
                  return (
                    <label key={profile.id} className="flex items-start gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50" style={{ backgroundColor: isSelected ? 'rgba(139,92,246,0.13)' : 'rgba(250,248,255,0.9)', borderColor: isSelected ? 'rgba(167,139,250,0.65)' : 'rgba(15,23,42,0.10)', boxShadow: isSelected ? '0 10px 28px rgba(76,29,149,.18)' : 'none' }}>
                      <input type="radio" name="profile" value={profile.id} checked={isSelected} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6] mt-0.5" />
                      <span className="flex-1 min-w-0">
                        <span className="block text-sm font-semibold text-[#1E1B29]">{profile.title}</span>
                        <span className="block text-[10px] font-mono text-[#8B8794] mt-0.5 truncate">{profile.id}</span>
                        <span className="block text-xs text-[#8B8794] mt-1 leading-relaxed">
                          {isSelected && profile.description ? profile.description : ""}
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
                        <label
                          key={profile.id}
                          className="flex items-start gap-4 rounded-xl px-5 py-4 cursor-pointer border transition-all duration-200 hover:-translate-y-0.5 hover:border-[#A78BFA]/50"
                          style={{
                            backgroundColor: isSelected ? 'rgba(139,92,246,0.13)' : 'rgba(250,248,255,0.9)',
                            borderColor: isSelected ? 'rgba(167,139,250,0.65)' : 'rgba(15,23,42,0.10)',
                            boxShadow: isSelected ? '0 10px 28px rgba(76,29,149,.18)' : 'none',
                          }}
                        >
                          <input type="radio" name="profile" value={profile.id} checked={isSelected} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6] mt-0.5" />
                          <span className="flex-1 min-w-0">
                            <span className="block text-sm font-semibold text-[#1E1B29]">{profile.title}</span>
                            <span className="block text-[10px] font-mono text-[#8B8794] mt-0.5 truncate">{profile.id}</span>
                            {isSelected && profile.description && (
                              <span className="block text-xs text-[#6B7280] mt-1.5 leading-relaxed">{profile.description}</span>
                            )}
                            {profile.extends && (
                              <span className="block text-xs text-[#8B8794] mt-1">Étend {profile.extends}</span>
                            )}
                          </span>

                          {isSelected ? (
                            <button
                              type="button"
                              aria-label={`Supprimer le profil ${profile.title}`}
                              disabled={deletingProfileId === profile.id}
                              onClick={(e) => {
                                e.preventDefault(); // n'active pas le radio au clic
                                e.stopPropagation();
                                setConfirmDeleteProfile(profile);
                              }}
                              className="flex items-center justify-center w-11 h-11 rounded-xl text-white text-lg transition-all hover:-translate-y-0.5 disabled:opacity-40 disabled:cursor-not-allowed disabled:hover:translate-y-0 shrink-0"
                              style={{
                                background: 'linear-gradient(135deg, #ef4444, #dc2626)',
                                boxShadow: '0 8px 18px rgba(220,38,38,.32)',
                                border: 'none',
                                cursor: 'pointer',
                              }}
                            >
                              {deletingProfileId === profile.id ? "…" : "🗑"}
                            </button>
                          ) : (
                            <span className="text-[#A78BFA] text-sm mt-0.5" aria-hidden="true">→</span>
                          )}
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
              style={{ backgroundColor: selectedProfile ? '#8B5CF6' : '#F3F0FC', color: selectedProfile ? '#fff' : '#9CA3AF', boxShadow: selectedProfile ? '0 10px 20px rgba(109,40,217,.28)' : undefined }}
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

      </div>

      {confirmDeleteProfile && (
        <div
          className="fixed inset-0 z-50 flex items-center justify-center p-8"
          style={{ background: 'rgba(8,10,15,0.72)', backdropFilter: 'blur(2px)' }}
          onClick={() => setConfirmDeleteProfile(null)}
        >
          <div
            role="dialog"
            aria-modal="true"
            className="w-full max-w-sm rounded-2xl border p-6"
            style={{
              background: 'linear-gradient(135deg, #f6f2fd, #f0eafb)',
              borderColor: 'rgba(167,139,250,.35)',
              boxShadow: '0 25px 60px rgba(0,0,0,.5)',
            }}
            onClick={(e) => e.stopPropagation()}
          >
            <h3 className="text-lg font-bold text-[#1E1B29] mb-2">Supprimer ce profil ?</h3>
            <p className="text-sm text-[#6B7280] leading-relaxed mb-6">
              Le profil « <span className="font-semibold text-[#1E1B29]">{confirmDeleteProfile.title}</span> » sera supprimé définitivement. Cette action est irréversible.
            </p>
            <div className="flex gap-3">
              <button
                type="button"
                className="flex-1 py-2.5 rounded-lg text-sm font-medium text-[#6B7280] hover:text-[#1E1B29] hover:bg-black/5 transition-colors"
                style={{ border: '1px solid rgba(15,23,42,0.12)' }}
                onClick={() => setConfirmDeleteProfile(null)}
              >
                Annuler
              </button>
              <button
                type="button"
                className="flex-1 py-2.5 rounded-lg text-sm font-semibold text-white transition-all hover:-translate-y-0.5"
                style={{
                  background: 'linear-gradient(135deg, #ef4444, #dc2626)',
                  boxShadow: '0 8px 18px rgba(220,38,38,.32)',
                }}
                onClick={() => {
                  deleteProfile(confirmDeleteProfile.id);
                  setConfirmDeleteProfile(null);
                }}
              >
                Supprimer
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}