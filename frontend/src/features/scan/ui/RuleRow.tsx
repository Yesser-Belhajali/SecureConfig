import type { Rule } from "../model/types";

interface RuleRowProps {
  rule: Rule;
  checked: boolean;
  active: boolean;
  readOnly?: boolean;
  onToggle: () => void;
  onSelect: () => void;
}

const SEVERITY_CLASS: Record<string, string> = {
  high: "severity-high",
  medium: "severity-medium",
  low: "severity-low",
  unknown: "severity-unknown",
};

export default function RuleRow({ rule, checked, active, readOnly = false, onToggle, onSelect }: RuleRowProps) {
  const severityClass = SEVERITY_CLASS[rule.severity?.toLowerCase()] ?? "severity-unknown";

  return (
    <div className={`rule-row ${active ? "is-active" : ""}`}>
      {!readOnly && (
        <input
          className="rule-row-check"
          type="checkbox"
          checked={checked}
          onChange={(e) => {
            e.stopPropagation();
            onToggle();
          }}
          onClick={(e) => e.stopPropagation()}
          aria-label={`Inclure la règle ${rule.title}`}
        />
      )}

      <button className="rule-row-main" onClick={onSelect}>
        <span className={`rule-severity-dot ${severityClass}`} aria-hidden="true" />
        <span className="rule-title">{rule.title}</span>
        <span className={`rule-severity ${severityClass}`}>{rule.severity}</span>
      </button>
    </div>
  );
}