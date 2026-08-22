const ExternalLinkIcon = () => (
  <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
    <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6" />
    <polyline points="15 3 21 3 21 9" />
    <line x1="10" y1="14" x2="21" y2="3" />
  </svg>
)

const timeline = [
  {
    date: 'Point de départ',
    title: 'Automatiser l\'audit de conformité',
    desc: 'Constat initial : réaliser un audit de conformité manuellement est fastidieux, error-prone, et impossible à répéter à l\'échelle. L\'objectif — construire un outil capable de l\'automatiser fidèlement.',
  },
  {
    date: 'Première approche',
    title: 'Extraction LLM depuis PDF',
    desc: 'Tentative d\'extraction de règles depuis des PDFs de benchmarks via des modèles de langage. Rapidement abandonnée : les résultats étaient inconsistants, non standardisés, et non fiables pour un contexte de sécurité.',
  },
  {
    date: 'Pivot stratégique',
    title: 'Adopter OpenSCAP et ComplianceAsCode',
    desc: 'Découverte du standard SCAP et du projet ComplianceAsCode/content comme fondation standardisée, maintenue, et reconnue par la communauté sécurité. Décision de construire autour de libopenscap.',
  },
  {
    date: 'Architecture interne',
    title: 'Plongée dans OpenSCAP',
    desc: 'Étude approfondie du modèle interne d\'OpenSCAP : sessions d\'évaluation, policy model, mécanisme de tailoring XCCDF, structure des datastreams SCAP et des profils OVAL.',
  },
  {
    date: 'Moteur C',
    title: 'Développement du backend d\'évaluation',
    desc: 'Construction d\'un moteur d\'évaluation en C s\'appuyant directement sur libopenscap — la même bibliothèque utilisée par les outils oscap et autotailor — pour des résultats conformes à la spécification.',
  },
  {
    date: 'Interface web',
    title: 'Construction de la plateforme',
    desc: 'Développement de l\'interface web React pour rendre le moteur accessible : sélection de profils, personnalisation via tailoring, suivi en temps réel, visualisation des résultats.',
  },
]

const scapComponents = [
  { id: 'XCCDF', name: 'Extensible Configuration Checklist Description Format', desc: 'Format XML pour définir des checklists de conformité et les règles de sécurité.' },
  { id: 'OVAL', name: 'Open Vulnerability and Assessment Language', desc: 'Langage pour exprimer des vérifications techniques de l\'état d\'un système.' },
  { id: 'CPE', name: 'Common Platform Enumeration', desc: 'Schéma standardisé pour identifier les plateformes et logiciels cibles.' },
  { id: 'CVE', name: 'Common Vulnerabilities and Exposures', desc: 'Identifiants uniques pour les vulnérabilités de sécurité connues.' },
  { id: 'CVSS', name: 'Common Vulnerability Scoring System', desc: 'Score numérique standardisé pour la sévérité des vulnérabilités.' },
]

export function About() {
  return (
    <div className="max-w-4xl mx-auto px-6 py-16">

      {/* Page header */}
      <div className="mb-16">
        <p
          className="text-xs font-semibold uppercase tracking-widest mb-3"
          style={{ color: '#8B5CF6', fontFamily: "'JetBrains Mono', monospace" }}
        >
          À propos
        </p>
        <h1 className="text-4xl font-bold text-[#1E1B29] leading-tight mb-4">
          Comprendre SCAP<br />et ce projet
        </h1>
        <p className="text-[#8B8794] text-lg max-w-2xl">
          Contexte technique, outils utilisés, et historique du développement.
        </p>
      </div>

      {/* Section 1 — SCAP */}
      <section className="mb-16">
        <div
          className="rounded-xl p-8 border mb-6"
          style={{ backgroundColor: '#FAF9FD', borderColor: 'rgba(15,23,42,0.07)' }}
        >
          <h2 className="text-xl font-bold text-[#1E1B29] mb-3">Qu'est-ce que SCAP ?</h2>
          <p className="text-[#6B7280] leading-relaxed mb-6">
            <strong className="text-[#C4B5FD]">SCAP (Security Content Automation Protocol)</strong> est un ensemble de standards
            développé par le NIST pour automatiser la vérification de la conformité de sécurité d'un système.
            Plutôt que de parcourir manuellement une checklist de plusieurs centaines de règles, SCAP
            permet de l'évaluer de façon automatique, reproductible, et interopérable.
          </p>

          <div className="grid sm:grid-cols-2 gap-3">
            {scapComponents.map((c) => (
              <div
                key={c.id}
                className="rounded-lg p-4 border"
                style={{ backgroundColor: '#FFFFFF', borderColor: 'rgba(15,23,42,0.06)' }}
              >
                <div className="flex items-start gap-3">
                  <span
                    className="text-xs font-bold px-2 py-1 rounded mt-0.5 flex-shrink-0"
                    style={{
                      backgroundColor: 'rgba(139,92,246,0.15)',
                      color: '#A78BFA',
                      fontFamily: "'JetBrains Mono', monospace",
                      border: '1px solid rgba(139,92,246,0.2)',
                    }}
                  >
                    {c.id}
                  </span>
                  <div>
                    <p className="text-xs font-medium text-[#3D3555] mb-1">{c.name}</p>
                    <p className="text-xs text-[#8B8794] leading-relaxed">{c.desc}</p>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* Section 2 — OpenSCAP */}
      <section className="mb-16">
        <div
          className="rounded-xl p-8 border"
          style={{ backgroundColor: '#FAF9FD', borderColor: 'rgba(15,23,42,0.07)' }}
        >
          <h2 className="text-xl font-bold text-[#1E1B29] mb-3">OpenSCAP</h2>
          <p className="text-[#6B7280] leading-relaxed mb-4">
            <strong className="text-[#C4B5FD]">OpenSCAP</strong> est l'implémentation open source de référence du standard SCAP,
            développée par <strong className="text-[#C4B5FD]">Red Hat</strong>. C'est le moteur utilisé dans RHEL, Fedora, et de nombreux
            outils de conformité enterprise.
          </p>
          <p className="text-[#6B7280] leading-relaxed mb-6">
            Ce projet s'appuie directement sur <code
              className="px-1.5 py-0.5 rounded text-[#A78BFA] text-sm"
              style={{ backgroundColor: 'rgba(139,92,246,0.12)', fontFamily: "'JetBrains Mono', monospace" }}
            >libopenscap</code> — la bibliothèque C exposant l'API de bas niveau utilisée par
            des outils comme <code style={{ fontFamily: "'JetBrains Mono', monospace" }} className="text-[#A78BFA] text-sm">oscap</code> et <code style={{ fontFamily: "'JetBrains Mono', monospace" }} className="text-[#A78BFA] text-sm">autotailor</code>.
            Cela garantit des résultats conformes à la spécification SCAP sans couche d'abstraction intermédiaire.
          </p>
          <a
            href="https://github.com/openscap/openscap"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 text-sm font-medium transition-colors"
            style={{ color: '#8B5CF6' }}
            onMouseEnter={e => (e.currentTarget.style.color = '#A78BFA')}
            onMouseLeave={e => (e.currentTarget.style.color = '#8B5CF6')}
          >
            openscap/openscap sur GitHub <ExternalLinkIcon />
          </a>
        </div>
      </section>

      {/* Section 3 — ComplianceAsCode */}
      <section className="mb-20">
        <div
          className="rounded-xl p-8 border"
          style={{ backgroundColor: '#FAF9FD', borderColor: 'rgba(15,23,42,0.07)' }}
        >
          <h2 className="text-xl font-bold text-[#1E1B29] mb-3">ComplianceAsCode / content</h2>
          <p className="text-[#6B7280] leading-relaxed mb-4">
            <strong className="text-[#C4B5FD]">ComplianceAsCode/content</strong> est la source des profils de sécurité utilisés
            par cette plateforme. Ce projet communautaire produit le <strong className="text-[#C4B5FD]">SCAP Security Guide (SSG)</strong> — une
            collection de datastreams SCAP couvrant les profils CIS, STIG DoD, PCI-DSS, et bien d'autres, pour
            de nombreuses distributions Linux.
          </p>

          <div
            className="rounded-lg p-5 mb-6 border"
            style={{ backgroundColor: '#FFFFFF', borderColor: 'rgba(15,23,42,0.06)' }}
          >
            <p className="text-xs font-semibold text-[#8B8794] uppercase tracking-wider mb-3" style={{ fontFamily: "'JetBrains Mono', monospace" }}>
              Contenu upstream vs tailoring
            </p>
            <div className="grid sm:grid-cols-2 gap-4">
              <div>
                <p className="text-xs font-medium text-[#6B7280] mb-1">Profil upstream</p>
                <p className="text-xs text-[#8B8794] leading-relaxed">
                  Les profils officiels compilés par ComplianceAsCode, non modifiés. Référence de conformité reconnue.
                </p>
              </div>
              <div>
                <p className="text-xs font-medium text-[#A78BFA] mb-1">Personnalisation via tailoring</p>
                <p className="text-xs text-[#8B8794] leading-relaxed">
                  Mécanisme XCCDF permettant d'activer/désactiver des règles ou modifier leurs paramètres sans toucher au contenu source.
                </p>
              </div>
            </div>
          </div>

          <a
            href="https://github.com/ComplianceAsCode/content"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 text-sm font-medium transition-colors"
            style={{ color: '#8B5CF6' }}
            onMouseEnter={e => (e.currentTarget.style.color = '#A78BFA')}
            onMouseLeave={e => (e.currentTarget.style.color = '#8B5CF6')}
          >
            ComplianceAsCode/content sur GitHub <ExternalLinkIcon />
          </a>
        </div>
      </section>

      {/* Section 4 — Timeline */}
      <section>
        <p
          className="text-xs font-semibold uppercase tracking-widest mb-3"
          style={{ color: '#8B8794', fontFamily: "'JetBrains Mono', monospace" }}
        >
          Historique
        </p>
        <h2 className="text-2xl font-bold text-[#1E1B29] mb-10">Genèse du projet</h2>

        <div className="relative">
          {/* Vertical line */}
          <div
            className="absolute left-[19px] top-0 bottom-0 w-px"
            style={{ backgroundColor: 'rgba(139,92,246,0.2)' }}
          />

          <div className="space-y-0">
            {timeline.map((step, i) => (
              <div key={i} className="relative flex gap-6 pb-10 last:pb-0">
                {/* Dot */}
                <div className="relative flex-shrink-0 mt-1">
                  <div
                    className="w-10 h-10 rounded-full flex items-center justify-center z-10 relative border"
                    style={{
                      backgroundColor: '#FFFFFF',
                      borderColor: 'rgba(139,92,246,0.35)',
                    }}
                  >
                    <span
                      className="text-xs font-bold"
                      style={{ color: '#8B5CF6', fontFamily: "'JetBrains Mono', monospace" }}
                    >
                      {String(i + 1).padStart(2, '0')}
                    </span>
                  </div>
                </div>

                {/* Content */}
                <div
                  className="flex-1 rounded-xl p-5 border -mt-1"
                  style={{ backgroundColor: '#FAF9FD', borderColor: 'rgba(15,23,42,0.07)' }}
                >
                  <p
                    className="text-xs mb-1"
                    style={{ color: '#8B5CF6', fontFamily: "'JetBrains Mono', monospace" }}
                  >
                    {step.date}
                  </p>
                  <h3 className="font-semibold text-[#1E1B29] mb-2">{step.title}</h3>
                  <p className="text-sm text-[#8B8794] leading-relaxed">{step.desc}</p>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>
    </div>
  )
}
