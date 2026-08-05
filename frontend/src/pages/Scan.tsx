import { useState, useEffect } from 'react'
import { useNavigate } from 'react-router-dom'
import { getProfiles } from '../features/scan/model/api'
import type { Profile } from '../features/scan/model/types'

const distributions = [
  { id: 'ubuntu', label: 'Ubuntu', versions: ['22.04', '24.04', '26.04'], accent: '#E95420' },
  { id: 'debian', label: 'Debian', versions: ['11', '12', '13'], accent: '#D70A53' },
  { id: 'rhel', label: 'Red Hat Enterprise Linux (RHEL)', versions: ['8', '9', '10'], accent: '#EE0000' },
  { id: 'sle', label: 'SUSE Linux Enterprise (SLE)', versions: ['12', '15', '16'], accent: '#30BA78' },
  { id: 'suse-micro', label: 'SUSE Linux Micro', versions: ['5', '6'], accent: '#30BA78' },
  { id: 'oracle-linux', label: 'Oracle Linux (OL)', versions: ['7', '8', '9', '10'], accent: '#F80000' },
  { id: 'amazon-linux', label: 'Amazon Linux', versions: ['2', '3', '2023'], accent: '#FF9900' },
  { id: 'anolis', label: 'Anolis OS (Alibaba Cloud)', versions: ['8', '23'], accent: '#FF6A00' },
  { id: 'kylin', label: 'Kylin Server', versions: ['6 (Sec)', '10'], accent: '#4068D4' },
  { id: 'almalinux', label: 'AlmaLinux', versions: ['9'], accent: '#18A0E8' },
  { id: 'fedora', label: 'Fedora', versions: [], accent: '#51A2DA' },
  { id: 'opensuse', label: 'openSUSE', versions: [], accent: '#73BA25' },
  { id: 'openeuler', label: 'openEuler', versions: ['2203'], accent: '#1677FF' },
  { id: 'tencentos', label: 'TencentOS', versions: ['4'], accent: '#006EFF' },
  { id: 'rhcos', label: 'Red Hat Enterprise Linux CoreOS (RHCOS)', versions: ['4'], accent: '#EE0000' },
]

type Screen = 'system' | 'profile'

function DistributionVisual({ name, accent }: { name: string; accent: string }) {
  const initials = name.replace(/[^A-Z]/g, '').slice(0, 2) || name.slice(0, 2).toUpperCase()
  return (
    <svg width="58" height="58" viewBox="0 0 58 58" role="img" aria-label={'Illustration ' + name}>
      <rect x="2" y="2" width="54" height="54" rx="16" fill={accent} fillOpacity="0.16" stroke={accent} strokeOpacity="0.45" />
      <path d="M17 35.5V23.2c0-2.32 1.88-4.2 4.2-4.2h15.6c2.32 0 4.2 1.88 4.2 4.2v12.3c0 2.32-1.88 4.2-4.2 4.2H21.2c-2.32 0-4.2-1.88-4.2-4.2Z" fill={accent} fillOpacity="0.25" stroke={accent} strokeWidth="1.5" />
      <path d="M23 39.7v3.5m12-3.5v3.5M19 43.2h20" stroke={accent} strokeWidth="1.5" strokeLinecap="round" />
      <text x="29" y="32" textAnchor="middle" fill={accent} fontSize="11" fontWeight="700" fontFamily="Inter, sans-serif">{initials}</text>
    </svg>
  )
}

// convention backend : ../data/ssg-<id>-ds.xml
// confirmé : ubuntu+24.04 -> ubuntu2404, rhel+9 -> rhel9, fedora (sans version) -> fedora
function buildBenchmarkId(distributionId: string, version: string): string {
  if (!version) return distributionId
  return `${distributionId}${version.replace(/[.\s()]/g, '')}`
}

export function Scan() {
  const navigate = useNavigate()
  const [screen, setScreen] = useState<Screen>('system')
  const [distributionId, setDistributionId] = useState('')
  const [version, setVersion] = useState('')
  const [selectedProfile, setSelectedProfile] = useState('')

  const [profiles, setProfiles] = useState<Profile[]>([])
  const [loadingProfiles, setLoadingProfiles] = useState(false)
  const [profilesError, setProfilesError] = useState<string | null>(null)

  const selectedDistribution = distributions.find(item => item.id === distributionId)
  const canContinue = Boolean(
    selectedDistribution && (selectedDistribution.versions.length === 0 || version)
  )
  const benchmarkId = selectedDistribution ? buildBenchmarkId(distributionId, version) : ''

  // charge les vrais profils dès qu'on arrive sur l'écran "profile"
  useEffect(() => {
    if (screen !== 'profile' || !benchmarkId) return

    let cancelled = false
    setLoadingProfiles(true)
    setProfilesError(null)
    setSelectedProfile('')

    getProfiles(benchmarkId)
      .then((data) => {
        if (!cancelled) setProfiles(data)
      })
      .catch((err) => {
        if (!cancelled) setProfilesError(err.message)
      })
      .finally(() => {
        if (!cancelled) setLoadingProfiles(false)
      })

    return () => { cancelled = true }
  }, [screen, benchmarkId])

  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <header className="mb-16"><h1 className="text-4xl lg:text-5xl font-bold tracking-tight text-[#E2E8F0]">Choisissez votre distribution</h1></header>

      {screen === 'system' && <section>
        <h2 className="text-xl font-semibold text-[#E2E8F0] mb-10">Distributions prises en charge</h2>

        <div className="lg:grid lg:grid-cols-[minmax(0,1fr)_280px] lg:gap-12 items-start">
          <div className="space-y-10">
            {distributions.map(item => <section key={item.id} aria-labelledby={'distribution-' + item.id}>
              <div className="flex items-center gap-4"><DistributionVisual name={item.label} accent={item.accent} /><h3 id={'distribution-' + item.id} className="text-xl font-semibold tracking-tight" style={{ color: distributionId === item.id ? '#C4B5FD' : '#E2E8F0' }}>{item.label}</h3></div>
              <div className="h-px mt-4 mb-4" style={{ backgroundColor: 'rgba(255,255,255,0.10)' }} />
              <div className="flex flex-wrap gap-2">
                {item.versions.length > 0 ? item.versions.map(itemVersion => {
                  const isSelected = distributionId === item.id && version === itemVersion
                  return <button key={itemVersion} type="button" className="px-3.5 py-2 rounded-md text-sm font-medium transition-colors" style={{ backgroundColor: isSelected ? '#8B5CF6' : 'transparent', color: isSelected ? '#fff' : '#94A3B8', border: isSelected ? '1px solid #A78BFA' : '1px solid rgba(255,255,255,0.13)' }} onClick={() => {
                    if (isSelected) {
                      setDistributionId('')
                      setVersion('')
                    } else {
                      setDistributionId(item.id)
                      setVersion(itemVersion)
                    }
                  }}>{itemVersion}</button>
                }) : (() => {
                  const isSelected = distributionId === item.id
                  return <button type="button" className="px-3.5 py-2 rounded-md text-sm font-medium transition-colors" style={{ backgroundColor: isSelected ? '#8B5CF6' : 'transparent', color: isSelected ? '#fff' : '#94A3B8', border: isSelected ? '1px solid #A78BFA' : '1px solid rgba(255,255,255,0.13)' }} onClick={() => {
                    if (isSelected) {
                      setDistributionId('')
                      setVersion('')
                    } else {
                      setDistributionId(item.id)
                      setVersion('') // pas de version distincte pour cette distro
                    }
                  }}>Version non précisée</button>
                })()}
              </div>
            </section>)}
          </div>

          <aside className="mt-10 lg:mt-0 lg:sticky lg:top-20 rounded-xl border p-5" style={{ backgroundColor: '#13171E', borderColor: canContinue ? 'rgba(139,92,246,0.45)' : 'rgba(255,255,255,0.08)' }}>
            {canContinue ? <>
              <div className="flex items-center gap-3 mb-5">
                <svg width="42" height="36" viewBox="0 0 68 56" fill="none" aria-hidden="true"><path d="M4 14c0-3.3 2.7-6 6-6h16l6 6h26c3.3 0 6 2.7 6 6v24c0 3.3-2.7 6-6 6H10c-3.3 0-6-2.7-6-6V14Z" fill="#8B5CF6" fillOpacity="0.18" stroke="#A78BFA" strokeWidth="1.5" /><path d="M4 22h60" stroke="#A78BFA" strokeWidth="1.5" /></svg>
                <p className="text-xs uppercase tracking-widest text-[#A78BFA]" style={{ fontFamily: "'JetBrains Mono', monospace" }}>Dossier de scan</p>
              </div>
              <dl className="space-y-4 mb-6"><div><dt className="text-xs text-[#64748B]">Distribution</dt><dd className="text-sm font-semibold text-[#E2E8F0] mt-1">{selectedDistribution?.label}</dd></div><div><dt className="text-xs text-[#64748B]">Version</dt><dd className="text-sm font-semibold text-[#A78BFA] mt-1">{version || '—'}</dd></div></dl>
              <div className="space-y-2"><button type="button" className="w-full py-2.5 rounded-md text-sm font-medium text-[#94A3B8]" style={{ border: '1px solid rgba(255,255,255,0.12)' }} onClick={() => { setDistributionId(''); setVersion('') }}>Annuler</button><button type="button" className="w-full py-2.5 rounded-md text-sm font-semibold text-white" style={{ backgroundColor: '#8B5CF6' }} onClick={() => setScreen('profile')}>Continuer →</button></div>
            </> : <div><p className="text-sm font-medium text-[#E2E8F0]">Aucun système sélectionné</p><p className="text-xs text-[#64748B] leading-relaxed mt-2">Choisissez une version dans la liste pour créer votre dossier de scan.</p></div>}
          </aside>
        </div>
      </section>}

      {screen === 'profile' && <section>
        <button type="button" className="text-sm text-[#A78BFA] mb-8" onClick={() => setScreen('system')}>← Modifier le système</button>
        <h2 className="text-2xl font-bold text-[#E2E8F0] mb-2">Sélectionner un profil</h2>
        <p className="text-sm text-[#64748B] mb-6">Profil appliqué à {selectedDistribution?.label} {version}.</p>

        {loadingProfiles && <p className="text-sm text-[#64748B] mb-6">Chargement des profils...</p>}
        {profilesError && <p className="text-sm text-red-400 mb-6">Erreur : {profilesError}</p>}

        {!loadingProfiles && !profilesError && (
          <div className="space-y-2 mb-8">
            {profiles.map(profile => (
              <label key={profile.id} className="flex items-center gap-4 rounded-lg px-4 py-4 cursor-pointer border" style={{ backgroundColor: selectedProfile === profile.id ? 'rgba(139,92,246,0.1)' : 'transparent', borderColor: selectedProfile === profile.id ? 'rgba(139,92,246,0.4)' : 'rgba(255,255,255,0.10)' }}>
                <input type="radio" name="profile" value={profile.id} checked={selectedProfile === profile.id} onChange={() => setSelectedProfile(profile.id)} className="accent-[#8B5CF6]" />
                <span className="text-sm font-medium text-[#E2E8F0]">{profile.title}</span>
              </label>
            ))}
          </div>
        )}

        <button
          type="button"
          className="w-full py-3 rounded-lg font-medium text-sm"
          style={{ backgroundColor: selectedProfile ? '#8B5CF6' : '#1A1F29', color: selectedProfile ? '#fff' : '#475569' }}
          disabled={!selectedProfile}
          onClick={() => navigate(`/benchmarks/${benchmarkId}/profiles/${selectedProfile}`)}
        >
          Voir et personnaliser les règles →
        </button>
      </section>}

      <p className="mt-10 text-xs text-[#475569] text-center">Aucune donnée ne quitte votre machine.</p>
    </div>
  )
}