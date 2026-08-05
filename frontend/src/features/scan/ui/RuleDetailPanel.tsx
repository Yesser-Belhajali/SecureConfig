import { useEffect } from "react";
import type { Rule } from "../model/types";

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
    <>
      <div
        className={`rule-panel-backdrop ${isOpen ? "is-open" : ""}`}
        onClick={onClose}
        aria-hidden={!isOpen}
      />

      <aside className={`rule-panel ${isOpen ? "is-open" : ""}`} aria-hidden={!isOpen}>
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
              <dd>{rule.severity}</dd>

              <dt>Description</dt>
              <dd>{rule.description || "—"}</dd>

              <dt>Justification</dt>
              <dd>{rule.rationale || "—"}</dd>
            </dl>
          </>
        )}
      </aside>
    </>
  );
}