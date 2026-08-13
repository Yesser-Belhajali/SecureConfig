import { useEffect } from "react";
import type { Rule } from "../model/types";
import { groupReferencesByHref } from "../model/referenceLabels";

interface RuleDetailPanelProps {
  rule: Rule | null;
  onClose: () => void;
}

export default function RuleDetailPanel({ rule, onClose }: RuleDetailPanelProps) {
  useEffect(() => {
    if (!rule) return;
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [rule, onClose]);

  const isOpen = rule !== null;

  return (
    <aside
      className={`rule-panel ${isOpen ? "is-open" : ""}`}
      aria-hidden={!isOpen}
      aria-label="Détails de la règle"
    >
      {rule && (
        <>
          <div className="rule-panel-header">
            <h3>{rule.title}</h3>
            <button className="rule-panel-close" onClick={onClose} aria-label="Fermer">
              ✕
            </button>
          </div>

          <dl className="rule-panel-body">
            <dt>ID</dt>
            <dd className="rule-id">{rule.id}</dd>

            <dt>Sévérité</dt>
            <dd>
              <span className={`rule-severity ${SEVERITY_CLASS_FALLBACK}`}>{rule.severity}</span>
            </dd>

            <dt>Description</dt>
            <dd>{rule.description || "—"}</dd>

            <dt>Justification</dt>
            <dd>{rule.rationale || "—"}</dd>

            {rule.question && (
              <>
                <dt>Question</dt>
                <dd>{rule.question}</dd>
              </>
            )}

            {rule.fixes.length > 0 && (
              <>
                <dt>Remédiation</dt>
                <dd>
                  <ul className="rule-fix-list">
                    {rule.fixes.map((fix, i) => (
                      <li key={i} className="rule-fix-item">
                        <div className="rule-fix-system">{fixSystemLabel(fix.system)}</div>
                        <pre className="rule-fix-content">{fix.content}</pre>
                      </li>
                    ))}
                  </ul>
                </dd>
              </>
            )}

            {rule.references.length > 0 && (
              <>
                <dt>Références</dt>
                <dd>
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
                </dd>
              </>
            )}
          </dl>
        </>
      )}
    </aside>
  );
}

const SEVERITY_CLASS_FALLBACK = "";

function fixSystemLabel(system: string): string {
  if (system.includes("ansible")) return "Ansible";
  if (system.includes("puppet")) return "Puppet";
  if (system.includes("anaconda")) return "Anaconda (kickstart)";
  if (system.includes("bash") || system.includes(":sh")) return "Script shell";
  return system || "Script";
}