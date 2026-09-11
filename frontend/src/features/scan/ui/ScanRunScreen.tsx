// features/scan/ui/ScanRunScreen.tsx
import { useMemo, useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import { useScanStream } from "../model/useScanStream";
import ScanResultRow from "./ScanResultRow";
import RuleDetailPanel from "./RuleDetailPanel";
import ScanScoreCircle from "./ScanScoreCircle";
import "./rule-panel.css";

interface ScanRunScreenProps {
  benchmarkId: string;
  profileId: string;
}

interface ScanRunLocationState {
  distributionId?: string;
  version?: string;
}

const SEVERITIES = ["all", "high", "medium", "low", "unknown"];
const STATUSES = ["all", "pass", "fail", "other"] as const;

const badgeStyle: React.CSSProperties = {
  display: "inline-flex",
  alignItems: "center",
  gap: "0.5rem",
  padding: "0.5rem 1rem",
  borderRadius: "999px",
  fontSize: "0.85rem",
  fontWeight: 600,
};

function badgeColors(variant: "running" | "done" | "error") {
  switch (variant) {
    case "running":
      return { background: "#ede9fe", color: "#6d28d9" };
    case "done":
      return { background: "#dcfce7", color: "#15803d" };
    case "error":
      return { background: "#fee2e2", color: "#b91c1c" };
  }
}

function matchesStatusFilter(status: string, filter: (typeof STATUSES)[number]): boolean {
  if (filter === "all") return true;
  if (filter === "pass") return status === "PASS";
  if (filter === "fail") return status === "FAIL";
  return status !== "PASS" && status !== "FAIL";
}

export function ScanRunScreen({ benchmarkId, profileId }: ScanRunScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const navigationState = location.state as ScanRunLocationState | null;

  const { inProgressTitle, results, totalRules, score, liveScore, status } = useScanStream(
    benchmarkId,
    profileId,
    true
  );

  const [severityFilter, setSeverityFilter] = useState("all");
  const [statusFilter, setStatusFilter] = useState<(typeof STATUSES)[number]>("all");
  const [search, setSearch] = useState("");
  const [selectedRuleId, setSelectedRuleId] = useState<string | null>(null);

  const passCount = results.filter((r) => r.status === "PASS").length;
  const failCount = results.filter((r) => r.status === "FAIL").length;
  const evaluatedCount = results.length;

  // pondère le score en direct par la proportion du profil déjà couverte -
  // sinon 8/10 règles évaluées affiche "80%" alors que 638 autres règles
  // du profil n'ont même pas encore été vérifiées
  const progressFraction = totalRules ? evaluatedCount / totalRules : 0;
  const displayScore =
    status === "done" && score !== null
      ? score
      : liveScore !== null
        ? liveScore * progressFraction
        : 0;

  const scanProgress =
    status === "done"
      ? 100
      : totalRules
        ? Math.min(99, Math.round((evaluatedCount / totalRules) * 100))
        : 0;

  const normalizedSearch = search.trim().toLowerCase();

  const visibleResults = useMemo(() => {
    return results.filter((r) => {
      const matchesSeverity = severityFilter === "all" || r.severity?.toLowerCase() === severityFilter;
      const matchesStatus = matchesStatusFilter(r.status, statusFilter);
      const matchesSearch = normalizedSearch.length === 0 || r.title.toLowerCase().includes(normalizedSearch);
      return matchesSeverity && matchesStatus && matchesSearch;
    });
  }, [results, severityFilter, statusFilter, normalizedSearch]);

  const selectedResult = selectedRuleId ? (results.find((r) => r.id === selectedRuleId) ?? null) : null;

  const goToRemediation = () => {
    const failedRules = results.filter((r) => r.status === "FAIL");
    if (failedRules.length === 0) return;
    navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/remediate`, {
      state: { failedRules },
    });
  };

  const badgeVariant = status === "error" ? "error" : status === "done" ? "done" : "running";
  const badgeLabel = status === "error" ? "Erreur" : status === "done" ? "Scan terminé" : "Scan en cours";

  return (
    <>
      <div className="scan-top-row">
        <button
          type="button"
          className="scan-back-button"
          onClick={() =>
            navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/view`, {
              state: {
                distributionId: navigationState?.distributionId,
                version: navigationState?.version,
              },
            })
          }
        >
          <span aria-hidden="true">←</span>
          Retour aux règles
        </button>

        {status !== "idle" && (
          <span style={{ ...badgeStyle, ...badgeColors(badgeVariant) }}>
            <span aria-hidden="true">●</span>
            {badgeLabel}
          </span>
        )}
      </div>

      <div className="scan-page">
        <div className="scan-header-row scan-header-row--plain">
          <h1>Résultats</h1>

          {status === "done" && failCount > 0 && (
            <button type="button" className="scan-remediate-trigger" onClick={goToRemediation}>
              Remédier
            </button>
          )}
        </div>

        {(status === "running" || status === "done") && evaluatedCount > 0 && (
          <div className="scan-score-top">
            <div className="scan-score-side is-pass">
              <span className="scan-score-side-value">{passCount}</span>
              <span className="scan-score-side-label">Réussies</span>
            </div>

            <div className="scan-score-center">
              <ScanScoreCircle score={displayScore} />
              <p className="scan-score-top-label">Score de conformité</p>
            </div>

            <div className="scan-score-side is-fail">
              <span className="scan-score-side-value">{failCount}</span>
              <span className="scan-score-side-label">Échouées</span>
            </div>
          </div>
        )}

        <section className="scan-controls" aria-label="Progression et filtres du scan">
          <div className="scan-progress-track" aria-hidden="true">
            <span style={{ width: `${scanProgress}%` }} />
          </div>

          {status === "running" && (
            <div className="scan-in-progress">
              <span className="scan-in-progress-spinner" aria-hidden="true" />
              {inProgressTitle ? (
                <span>
                  Test en cours : <strong>{inProgressTitle}</strong>
                </span>
              ) : (
                <span>Démarrage du scan...</span>
              )}
              {totalRules !== null && (
                <span className="scan-progress-count">
                  ({evaluatedCount} / {totalRules})
                </span>
              )}
            </div>
          )}

          {status === "done" && (
            <>
              <div className="rules-search">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                  <circle cx="11" cy="11" r="8" />
                  <line x1="21" y1="21" x2="16.65" y2="16.65" />
                </svg>
                <input
                  type="text"
                  placeholder="Rechercher un résultat par nom..."
                  value={search}
                  onChange={(e) => setSearch(e.target.value)}
                  aria-label="Rechercher un résultat par nom"
                />
              </div>

              <div className="rules-filters" role="group" aria-label="Filtrer par statut">
                <span>Statut</span>
                {STATUSES.map((s) => (
                  <button key={s} type="button" className={statusFilter === s ? "is-selected" : ""} onClick={() => setStatusFilter(s)}>
                    {s === "all" ? "Tous" : s === "pass" ? "Réussi" : s === "fail" ? "Échoué" : "Autre"}
                  </button>
                ))}
              </div>

              <div className="rules-filters" role="group" aria-label="Filtrer par sévérité">
                <span>Sévérité</span>
                {SEVERITIES.map((sev) => (
                  <button key={sev} type="button" className={severityFilter === sev ? "is-selected" : ""} onClick={() => setSeverityFilter(sev)}>
                    {sev === "all" ? "Toutes" : sev}
                  </button>
                ))}
              </div>
            </>
          )}
        </section>

        <section className="scan-remediation-panel" aria-label="Résultats du scan">
          <div className="scan-remediation-header">
            <h2>Résultats du scan</h2>
          </div>

          {visibleResults.length === 0 ? (
            <p className="scan-empty">
              {evaluatedCount === 0 && status === "running" ? "En attente des premiers résultats..." : "Aucun résultat ne correspond à ces filtres."}
            </p>
          ) : (
            <div className="scan-result-list">
              {visibleResults.map((r) => (
                <ScanResultRow
                  key={r.id}
                  title={r.title}
                  status={r.status}
                  severity={r.severity}
                  isActive={r.id === selectedRuleId}
                  onClick={() => setSelectedRuleId((current) => (current === r.id ? null : r.id))}
                />
              ))}
            </div>
          )}
        </section>

        <RuleDetailPanel rule={selectedResult} onClose={() => setSelectedRuleId(null)} />

        {status === "error" && <p className="scan-error-card">Une erreur est survenue pendant le scan.</p>}
      </div>
    </>
  );
}