interface ScanResultRowProps {
  title: string;
  status: string;
}

function statusClass(status: string): "status-pass" | "status-fail" | "status-other" {
  if (status === "PASS") return "status-pass";
  if (status === "FAIL") return "status-fail";
  return "status-other";
}

export default function ScanResultRow({ title, status }: ScanResultRowProps) {
  return (
    <div className={`rule-row scan-result-row ${statusClass(status)}`}>
      <div className="rule-row-main" style={{ cursor: "default" }}>
        <span className={`rule-severity-dot scan-result-dot ${statusClass(status)}`} aria-hidden="true" />
        <span className="rule-title scan-result-title">{title}</span>
        <span className={`rule-severity scan-result-status ${statusClass(status)}`}>{status}</span>
      </div>
    </div>
  );
}