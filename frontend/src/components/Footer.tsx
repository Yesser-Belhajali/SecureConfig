import { NavLink } from 'react-router-dom'

export function Footer() {
  return (
    <footer
      className="border-t mt-24"
      style={{ borderColor: 'rgba(255,255,255,0.07)' }}
    >
      <div className="max-w-6xl mx-auto px-6 py-12">
        <div className="grid grid-cols-1 md:grid-cols-3 gap-10">
          {/* Brand */}
          <div>
            <p className="font-semibold text-[#E2E8F0] mb-3">
              Secure<span className="text-[#8B5CF6]">Config</span>
            </p>
            <p className="text-sm text-[#64748B] leading-relaxed max-w-xs">
              Plateforme d’audit de conformité et de sécurité.
            </p>
          </div>

          {/* Ressources */}
          <div>
            <p className="text-xs font-semibold uppercase tracking-widest text-[#64748B] mb-4">Ressources</p>
            <ul className="space-y-2.5">
              <li>
                <NavLink to="/about" className="text-sm text-[#94A3B8] hover:text-[#A78BFA] transition-colors">
                  À propos
                </NavLink>
              </li>
              <li>
                <a
                  href="https://www.open-scap.org/resources/documentation/"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-sm text-[#94A3B8] hover:text-[#A78BFA] transition-colors"
                >
                  Documentation OpenSCAP
                </a>
              </li>
              <li>
                <a
                  href="https://github.com/openscap/openscap"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-sm text-[#94A3B8] hover:text-[#A78BFA] transition-colors"
                >
                  openscap/openscap ↗
                </a>
              </li>
              <li>
                <a
                  href="https://github.com/ComplianceAsCode/content"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-sm text-[#94A3B8] hover:text-[#A78BFA] transition-colors"
                >
                  ComplianceAsCode/content ↗
                </a>
              </li>
            </ul>
          </div>

          {/* Contact */}
          <div>
            <p className="text-xs font-semibold uppercase tracking-widest text-[#64748B] mb-4">Contact</p>
            <p className="text-sm text-[#94A3B8] mb-3">
              Une question, un bug, une suggestion ?
            </p>
            <a
              href="mailto:medyesserbha@gmail.com"
              className="text-sm text-[#A78BFA] hover:text-[#8B5CF6] transition-colors"
            >
              medyesserbha@gmail.com
            </a>
          </div>
        </div>

        <div
          className="mt-10 pt-6 border-t flex flex-col sm:flex-row items-center justify-between gap-3"
          style={{ borderColor: 'rgba(255,255,255,0.06)' }}
        >
          <p className="text-xs text-[#475569]">
            Projet personnel &mdash; non affilié à Red Hat, CIS ou DISA.
          </p>
          <p
            className="text-xs"
            style={{ color: '#475569', fontFamily: "'JetBrains Mono', monospace" }}
          >
            Conformité et sécurité, simplement
          </p>
        </div>
      </div>
    </footer>
  )
}
