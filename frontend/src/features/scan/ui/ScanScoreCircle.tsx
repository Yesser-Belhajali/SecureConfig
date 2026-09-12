interface ScanScoreCircleProps {
  score: number; // 0-100
  size?: number;
  strokeWidth?: number;
}

function scoreColor(score: number): string {
  if (score < 50) return "#f87171";
  if (score < 75) return "#facc15";
  return "#4ade80";
}

export default function ScanScoreCircle({ score, size = 120, strokeWidth = 10 }: ScanScoreCircleProps) {
  const clamped = Math.max(0, Math.min(100, score));
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  const offset = circumference * (1 - clamped / 100);
  const color = scoreColor(clamped);
  const center = size / 2;

  return (
    <div className="scan-score-circle" role="img" aria-label={`Conformité : ${clamped.toFixed(1)}%`}>
      <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`}>
        <circle
          className="scan-score-circle-track"
          cx={center}
          cy={center}
          r={radius}
          fill="none"
          strokeWidth={strokeWidth}
        />
        <circle
          cx={center}
          cy={center}
          r={radius}
          fill="none"
          strokeWidth={strokeWidth}
          strokeLinecap="round"
          transform={`rotate(-90 ${center} ${center})`}
          style={{
            stroke: color,
            strokeDasharray: circumference,
            strokeDashoffset: offset,
            transition: "stroke-dashoffset 0.6s ease, stroke 0.3s ease",
          }}
        />
      </svg>
      <span
        className="scan-score-circle-value"
        style={{ color, fontSize: size < 100 ? "1rem" : undefined }}
      >
        {clamped.toFixed(0)}%
      </span>
    </div>
  );
}