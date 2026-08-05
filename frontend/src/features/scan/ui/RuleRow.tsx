import type { Rule } from "../model/types";

interface RuleRowProps {
  rule: Rule;
  checked: boolean;
  onToggle: () => void;
  onSelect: () => void;
}

const SEVERITY_CLASS: Record<string, string> = {
  high: "severity-high",
  medium: "severity-medium",
  low: "severity-low",
  unknown: "severity-unknown",
};

export default function RuleRow({ rule, checked, onToggle, onSelect }: RuleRowProps) {
  const severityClass = SEVERITY_CLASS[rule.severity?.toLowerCase()] ?? "severity-unknown";

  return (
    <div className="rule-row">
      <input
        type="checkbox"
        checked={checked}
        onChange={(e) => {
          e.stopPropagation(); // ne pas déclencher l'ouverture du panneau
          onToggle();
        }}
        onClick={(e) => e.stopPropagation()}
      />

      <button className="rule-row-main" onClick={onSelect}>
        <span className="rule-title">{rule.title}</span>
        <span className="rule-id">{rule.id}</span>
        <span className={`rule-severity ${severityClass}`}>{rule.severity}</span>
      </button>
    </div>
  );
}