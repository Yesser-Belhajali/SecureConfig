interface ScanScoreCircleProps {
  score: number; // 0-100
}

function scoreColor(score: number): string {
  if (score < 50) return "#f87171"; // rouge
  if (score < 75) return "#facc15"; // jaune
  return "#4ade80"; // vert
}

export default function ScanScoreCircle({ score }: ScanScoreCircleProps) {
  const clamped = Math.max(0, Math.min(100, score));
  const radius = 46;
  const circumference = 2 * Math.PI * radius;
  const offset = circumference * (1 - clamped / 100);
  const color = scoreColor(clamped);

  return (
    <div className="scan-score-circle" role="img" aria-label={`Conformité : ${clamped.toFixed(1)}%`}>
      <svg width="120" height="120" viewBox="0 0 120 120">
        <circle
          className="scan-score-circle-track"
          cx="60"
          cy="60"
          r={radius}
          fill="none"
          strokeWidth="10"
        />
        <circle
          cx="60"
          cy="60"
          r={radius}
          fill="none"
          strokeWidth="10"
          strokeLinecap="round"
          transform="rotate(-90 60 60)"
          style={{
            stroke: color,
            strokeDasharray: circumference,
            strokeDashoffset: offset,
            transition: "stroke-dashoffset 0.6s ease, stroke 0.3s ease",
          }}
        />
      </svg>
      <span className="scan-score-circle-value" style={{ color }}>
        {clamped.toFixed(0)}%
      </span>
    </div>
  );
}