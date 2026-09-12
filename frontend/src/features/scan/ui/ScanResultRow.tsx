// features/scan/ui/ScanResultRow.tsx
interface ScanResultRowProps {
  title: string;
  status: string;
  severity?: string;
  isActive: boolean;
  onClick: () => void;
}

function statusClass(status: string): "status-pass" | "status-fail" | "status-fixed" | "status-other" {
  if (status === "PASS") return "status-pass";
  if (status === "FAIL") return "status-fail";
  if (status === "FIXED") return "status-fixed";
  return "status-other";
}

const SEVERITY_CLASS: Record<string, string> = {
  high: "severity-high",
  medium: "severity-medium",
  low: "severity-low",
  unknown: "severity-unknown",
};

export default function ScanResultRow({ title, status, severity, isActive, onClick }: ScanResultRowProps) {
  const severityClass = severity ? (SEVERITY_CLASS[severity.toLowerCase()] ?? "severity-unknown") : "severity-unknown";

  return (
    <div
      className={`rule-row scan-result-row ${statusClass(status)} ${isActive ? "is-active" : ""}`}
      onClick={onClick}
      role="button"
      tabIndex={0}
      onKeyDown={(e) => {
        if (e.key === "Enter" || e.key === " ") {
          e.preventDefault();
          onClick();
        }
      }}
    >
      <div className="rule-row-main">
        <span className={`rule-severity-dot scan-result-dot ${statusClass(status)}`} aria-hidden="true" />
        <span className="rule-title scan-result-title">{title}</span>
        <span className="scan-result-badges">
          <span className={`rule-severity scan-result-severity-slot ${severityClass}`}>
            {severity ?? "—"}
          </span>
          <span className={`rule-severity scan-result-status scan-result-status-slot ${statusClass(status)}`}>
            {status}
          </span>
        </span>
      </div>
    </div>
  );
}