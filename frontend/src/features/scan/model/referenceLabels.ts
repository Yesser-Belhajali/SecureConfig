import type { RuleReference } from "./types";

const REFERENCE_LABELS: { prefix: string; label: string }[] = [
  { prefix: "https://benchmarks.cisecurity.org/", label: "CIS" },
  { prefix: "https://www.pcisecuritystandards.org/", label: "PCI-DSS" },
  { prefix: "http://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-53", label: "NIST SP 800-53" },
  { prefix: "http://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-171", label: "NIST SP 800-171" },
  { prefix: "http://iase.disa.mil/stigs/cci/", label: "DISA CCI" },
  { prefix: "http://iase.disa.mil/stigs/srgs/", label: "DISA SRG" },
  { prefix: "http://iase.disa.mil/stigs/os/", label: "DISA STIG" },
  { prefix: "http://iase.disa.mil/stigs/app-security/", label: "DISA STIG" },
  { prefix: "http://www.ssi.gouv.fr/administration/bonnes-pratiques", label: "ANSSI" },
  { prefix: "https://www.iso.org/standard/54534.html", label: "ISO 27001" },
];

function labelForHref(href: string): string {
  const match = REFERENCE_LABELS.find((entry) => href.startsWith(entry.prefix));
  if (match) return match.label;
  try {
    return new URL(href).hostname.replace(/^www\./, "");
  } catch {
    return href; // href malformé, fallback brut
  }
}

export interface ReferenceGroup {
  label: string;
  href: string;   // vide si la référence n'avait pas de href
  values: string[];
}

// regroupe les références par href (plusieurs contrôles du même référentiel
// partagent le même href, ex: 4.3.8 et un autre id CIS) ; les références
// sans href (rares) restent affichées individuellement
export function groupReferencesByHref(references: RuleReference[]): ReferenceGroup[] {
  const byHref = new Map<string, ReferenceGroup>();
  const standalone: ReferenceGroup[] = [];

  for (const ref of references) {
    if (!ref.href) {
      standalone.push({ label: ref.text || "—", href: "", values: [] });
      continue;
    }
    const existing = byHref.get(ref.href);
    if (existing) {
      if (ref.text) existing.values.push(ref.text);
    } else {
      byHref.set(ref.href, {
        label: labelForHref(ref.href),
        href: ref.href,
        values: ref.text ? [ref.text] : [],
      });
    }
  }

  return [...byHref.values(), ...standalone];
}