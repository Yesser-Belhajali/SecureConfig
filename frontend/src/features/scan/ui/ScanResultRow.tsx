// features/scan/ui/ScanResultRow.tsx
interface ScanResultRowProps {
  title: string;
  status: string;
  isActive: boolean;
  onClick: () => void;
}

function statusClass(status: string): "status-pass" | "status-fail" | "status-other" {
  if (status === "PASS") return "status-pass";
  if (status === "FAIL") return "status-fail";
  return "status-other";
}

export default function ScanResultRow({ title, status, isActive, onClick }: ScanResultRowProps) {
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
        <span className={`rule-severity scan-result-status ${statusClass(status)}`}>{status}</span>
      </div>
    </div>
  );
}