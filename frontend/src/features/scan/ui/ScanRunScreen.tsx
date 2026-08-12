// features/scan/ui/ScanRunScreen.tsx
import { useMemo, useState } from "react";
import { useScanStream } from "../model/useScanStream";
import ScanResultRow from "./ScanResultRow";
import ScanScoreCircle from "./ScanScoreCircle";
import "./rule-panel.css";

interface ScanRunScreenProps {
  benchmarkId: string;
  profileId: string;
}

const SEVERITIES = ["all", "high", "medium", "low", "unknown"];
const STATUSES = ["all", "pass", "fail", "other"] as const;

function matchesStatusFilter(
  status: string,
  filter: (typeof STATUSES)[number]
): boolean {
  if (filter === "all") return true;
  if (filter === "pass") return status === "PASS";
  if (filter === "fail") return status === "FAIL";
  return status !== "PASS" && status !== "FAIL";
}

export function ScanRunScreen({
  benchmarkId,
  profileId,
}: ScanRunScreenProps) {
  const {
    inProgressTitle,
    results,
    totalRules,
    score,
    status,
  } = useScanStream(benchmarkId, profileId, true);

  const [severityFilter, setSeverityFilter] = useState("all");
  const [statusFilter, setStatusFilter] =
    useState<(typeof STATUSES)[number]>("all");
  const [search, setSearch] = useState("");

  const passCount = results.filter((r) => r.status === "PASS").length;
  const failCount = results.filter((r) => r.status === "FAIL").length;
  const evaluatedCount = results.length;

  // Score en direct pendant le scan :
  // - commence à 0
  // - augmente uniquement lorsqu'une règle est PASS
  // - utilise le nombre total de règles comme dénominateur
  // - le score final du backend prend le relais lorsque le scan est terminé
  const liveScore = totalRules
    ? Math.round((passCount / totalRules) * 100)
    : 0;

  const displayScore =
    status === "done" && score !== null ? score : liveScore;

  // La barre ne peut atteindre 100% que lorsque le backend
  // confirme officiellement que le scan est terminé.
  const scanProgress =
    status === "done"
      ? 100
      : totalRules
        ? Math.min(
            99,
            Math.round((evaluatedCount / totalRules) * 100)
          )
        : 0;

  const normalizedSearch = search.trim().toLowerCase();

  const visibleResults = useMemo(() => {
    return results.filter((r) => {
      const matchesSeverity =
        severityFilter === "all" ||
        r.severity?.toLowerCase() === severityFilter;

      const matchesStatus = matchesStatusFilter(
        r.status,
        statusFilter
      );

      const matchesSearch =
        normalizedSearch.length === 0 ||
        r.title.toLowerCase().includes(normalizedSearch);

      return (
        matchesSeverity &&
        matchesStatus &&
        matchesSearch
      );
    });
  }, [
    results,
    severityFilter,
    statusFilter,
    normalizedSearch,
  ]);

  return (
    <div className="scan-page">
      <div className="scan-header-row">
        <h1>Résultats</h1>

        {status === "running" && (
          <span className="scan-status-badge is-running">
            <span
              className="scan-status-dot"
              aria-hidden="true"
            />
            Scan en cours
          </span>
        )}

        {status === "done" && (
          <span className="scan-status-badge is-done">
            <span
              className="scan-status-dot"
              aria-hidden="true"
            />
            Scan terminé
          </span>
        )}

        {status === "error" && (
          <span className="scan-status-badge is-error">
            <span
              className="scan-status-dot"
              aria-hidden="true"
            />
            Erreur
          </span>
        )}
      </div>

      {(status === "running" || status === "done") &&
        evaluatedCount > 0 && (
          <div className="scan-score-top">
            <div className="scan-score-side is-pass">
              <span className="scan-score-side-value">
                {passCount}
              </span>
              <span className="scan-score-side-label">
                Réussies
              </span>
            </div>

            <div className="scan-score-center">
              <ScanScoreCircle score={displayScore} />
              <p className="scan-score-top-label">
                Score de conformité
              </p>
            </div>

            <div className="scan-score-side is-fail">
              <span className="scan-score-side-value">
                {failCount}
              </span>
              <span className="scan-score-side-label">
                Échouées
              </span>
            </div>
          </div>
        )}

      <section
        className="scan-controls"
        aria-label="Progression et filtres du scan"
      >
        <div
          className="scan-progress-track"
          aria-hidden="true"
        >
          <span
            style={{ width: `${scanProgress}%` }}
          />
        </div>

        {status === "running" && (
          <div className="scan-in-progress">
            <span
              className="scan-in-progress-spinner"
              aria-hidden="true"
            />

            {inProgressTitle ? (
              <span>
                Test en cours :{" "}
                <strong>{inProgressTitle}</strong>
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

        {/* Recherche et filtres disponibles uniquement
            lorsque le scan est officiellement terminé. */}
        {status === "done" && (
          <>
            <div className="rules-search">
              <svg
                width="14"
                height="14"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2.5"
                strokeLinecap="round"
                strokeLinejoin="round"
                aria-hidden="true"
              >
                <circle
                  cx="11"
                  cy="11"
                  r="8"
                />
                <line
                  x1="21"
                  y1="21"
                  x2="16.65"
                  y2="16.65"
                />
              </svg>

              <input
                type="text"
                placeholder="Rechercher un résultat par nom..."
                value={search}
                onChange={(e) =>
                  setSearch(e.target.value)
                }
                aria-label="Rechercher un résultat par nom"
              />
            </div>

            <div
              className="rules-filters"
              role="group"
              aria-label="Filtrer par statut"
            >
              <span>Statut</span>

              {STATUSES.map((s) => (
                <button
                  key={s}
                  type="button"
                  className={
                    statusFilter === s
                      ? "is-selected"
                      : ""
                  }
                  onClick={() =>
                    setStatusFilter(s)
                  }
                >
                  {s === "all"
                    ? "Tous"
                    : s === "pass"
                      ? "Réussi"
                      : s === "fail"
                        ? "Échoué"
                        : "Autre"}
                </button>
              ))}
            </div>

            <div
              className="rules-filters"
              role="group"
              aria-label="Filtrer par sévérité"
            >
              <span>Sévérité</span>

              {SEVERITIES.map((sev) => (
                <button
                  key={sev}
                  type="button"
                  className={
                    severityFilter === sev
                      ? "is-selected"
                      : ""
                  }
                  onClick={() =>
                    setSeverityFilter(sev)
                  }
                >
                  {sev === "all"
                    ? "Toutes"
                    : sev}
                </button>
              ))}
            </div>
          </>
        )}
      </section>

      {visibleResults.length === 0 ? (
        <p className="scan-empty">
          {evaluatedCount === 0 &&
          status === "running"
            ? "En attente des premiers résultats..."
            : "Aucun résultat ne correspond à ces filtres."}
        </p>
      ) : (
        <div className="scan-result-list">
          {visibleResults.map((r) => (
            <ScanResultRow
              key={r.rule_id}
              title={r.title}
              status={r.status}
            />
          ))}
        </div>
      )}

      {status === "error" && (
        <p className="scan-error-card">
          Une erreur est survenue pendant le scan.
        </p>
      )}
    </div>
  );
}