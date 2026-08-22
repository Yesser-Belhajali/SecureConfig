// features/scan/ui/RuleDetailPanel.tsx
import { useEffect, useState } from "react";
import type { Rule, RuleResult } from "../model/types";
import { groupReferencesByHref } from "../model/referenceLabels";

interface RuleDetailPanelProps {
  rule: Rule | RuleResult | null;
  onClose: () => void;
}

function isRuleResult(rule: Rule | RuleResult): rule is RuleResult {
  return "status" in rule;
}

function statusClass(status: string): "status-pass" | "status-fail" | "status-other" {
  if (status === "PASS") return "status-pass";
  if (status === "FAIL") return "status-fail";
  return "status-other";
}

function fixSystemLabel(system: string): string {
  if (system.includes("ansible")) return "Ansible";
  if (system.includes("puppet")) return "Puppet";
  if (system.includes("anaconda")) return "Anaconda (kickstart)";
  if (system.includes("bash") || system.includes(":sh")) return "Script shell";
  return system || "Script";
}

export default function RuleDetailPanel({ rule, onClose }: RuleDetailPanelProps) {
  const [openFixes, setOpenFixes] = useState<Set<number>>(new Set());

  // Réinitialise les remédiations dépliées à chaque changement de règle
  useEffect(() => {
    setOpenFixes(new Set());
  }, [rule]);

  useEffect(() => {
    if (!rule) return;
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [rule, onClose]);

  if (!rule) return null;

  const result = isRuleResult(rule) ? rule : null;

  function toggleFix(i: number) {
    setOpenFixes((prev) => {
      const next = new Set(prev);
      if (next.has(i)) next.delete(i);
      else next.add(i);
      return next;
    });
  }

  return (
    <div className="scan-result-backdrop" onClick={onClose}>
      <div
        className="scan-result-modal"
        role="dialog"
        aria-modal="true"
        aria-labelledby="rule-modal-title"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="scan-result-modal-header">
          <h3 id="rule-modal-title">{rule.title}</h3>
          <button className="scan-result-modal-close" onClick={onClose} aria-label="Fermer">
            ✕
          </button>
        </div>

        <div className="scan-result-modal-body">
          <table className="scan-result-table">
            <tbody>
              <tr>
                <td>Rule ID</td>
                <td className="rule-id">{rule.id}</td>
              </tr>
              {result && (
                <tr>
                  <td>Résultat</td>
                  <td>
                    <span className={`scan-result-status-bar ${statusClass(result.status)}`}>
                      {result.status}
                    </span>
                  </td>
                </tr>
              )}
              <tr>
                <td>Sévérité</td>
                <td>{rule.severity}</td>
              </tr>
            </tbody>
          </table>

          <h4 className="scan-result-section-title">Description</h4>
          <p>{rule.description || "—"}</p>

          <h4 className="scan-result-section-title">Justification</h4>
          <p>{rule.rationale || "—"}</p>

          {rule.question && (
            <>
              <h4 className="scan-result-section-title">Question</h4>
              <p>{rule.question}</p>
            </>
          )}

          {rule.warnings.length > 0 && (
            <>
              <h4 className="scan-result-section-title">Avertissements</h4>
              <ul className="rule-warning-list">
                {rule.warnings.map((w, i) => (
                  <li key={i} className="rule-warning-item">
                    <span className="rule-warning-category">{w.category}</span>
                    <span className="rule-warning-text">{w.text}</span>
                  </li>
                ))}
              </ul>
            </>
          )}

          {rule.platforms.length > 0 && (
            <>
              <h4 className="scan-result-section-title">Plateformes concernées</h4>
              <ul className="rule-platform-list">
                {rule.platforms.map((p, i) => (
                  <li key={i} className="rule-platform-item">{p}</li>
                ))}
              </ul>
            </>
          )}

          <h4 className="scan-result-section-title">Remédiation</h4>
          {rule.fixes.length === 0 ? (
            <p className="scan-result-no-fix">Aucun script de remédiation disponible.</p>
          ) : (
            <ul className="rule-fix-list">
              {rule.fixes.map((fix, i) => {
                const isOpen = openFixes.has(i);
                return (
                  <li key={i} className="rule-fix-item">
                    <button
                      type="button"
                      className="rule-fix-toggle"
                      onClick={() => toggleFix(i)}
                      aria-expanded={isOpen}
                    >
                      <span className="rule-fix-system">{fixSystemLabel(fix.system)}</span>
                      <span className="rule-fix-chevron">{isOpen ? "▾" : "▸"}</span>
                    </button>
                    {isOpen && <pre className="rule-fix-content">{fix.content}</pre>}
                  </li>
                );
              })}
            </ul>
          )}

          {rule.checks.length > 0 && (
            <>
              <h4 className="scan-result-section-title">Vérifications techniques</h4>
              <ul className="rule-check-list">
                {rule.checks.map((check, i) => (
                  <li key={i} className="rule-check-item">
                    <div className="rule-check-system">{check.system}</div>
                    {check.selector && (
                      <div className="rule-check-selector">Sélecteur : {check.selector}</div>
                    )}
                    {check.content && (
                      <pre className="rule-check-content">{check.content}</pre>
                    )}
                  </li>
                ))}
              </ul>
            </>
          )}

          {rule.references.length > 0 && (
            <>
              <h4 className="scan-result-section-title">Références</h4>
              <table className="rule-reference-table">
                <tbody>
                  {groupReferencesByHref(rule.references).map((group, i) => (
                    <tr key={i}>
                      <td className="rule-reference-label">
                        {group.href ? (
                          <a href={group.href} target="_blank" rel="noreferrer">{group.label}</a>
                        ) : (
                          group.label
                        )}
                      </td>
                      <td className="rule-reference-values">{group.values.join(", ") || "—"}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </>
          )}
        </div>
      </div>
    </div>
  );
}