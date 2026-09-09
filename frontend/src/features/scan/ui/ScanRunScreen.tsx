// features/scan/ui/ScanRunScreen.tsx
import { useMemo, useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import { useScanStream } from "../model/useScanStream";
import { useRemediationStream } from "../model/useRemediationStream";
import ScanResultRow from "./ScanResultRow";
import RuleDetailPanel from "./RuleDetailPanel";
import ScanScoreCircle from "./ScanScoreCircle";
import { useToasts, ToastContainer } from "../../../components/Toast";
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

function matchesStatusFilter(
  status: string,
  filter: (typeof STATUSES)[number]
): boolean {
  if (filter === "all") return true;
  if (filter === "pass") return status === "PASS";
  if (filter === "fail") return status === "FAIL";
  return status !== "PASS" && status !== "FAIL";
}

export function ScanRunScreen({ benchmarkId, profileId }: ScanRunScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const navigationState = location.state as ScanRunLocationState | null;
  const { toasts, pushToast, dismissToast } = useToasts();

  const { inProgressTitle, results, totalRules, score, status } = useScanStream(
    benchmarkId,
    profileId,
    true
  );

  const {
    start: startRemediation,
    results: remediationResults,
    status: remediationStatus,
    errorMessage: remediationError,
  } = useRemediationStream(benchmarkId, profileId);

  const [severityFilter, setSeverityFilter] = useState("all");
  const [statusFilter, setStatusFilter] = useState<(typeof STATUSES)[number]>("all");
  const [search, setSearch] = useState("");
  const [selectedRuleId, setSelectedRuleId] = useState<string | null>(null);

  // mode remédiation : activé par le bouton "Remédier", uniquement disponible
  // une fois le scan terminé — les FAIL du scan sont la seule source possible
  // de rule_ids envoyés au backend, jamais une liste construite ailleurs
  const [remediationMode, setRemediationMode] = useState(false);
  const [selectedForRemediation, setSelectedForRemediation] = useState<Set<string>>(new Set());

  // signale toute erreur de remédiation via toast, dès qu'elle apparaît —
  // même pattern que ScanScreen/ProfileRulesScreen pour rester cohérent
  const prevRemediationErrorRef = useState<{ current: string | null }>({ current: null })[0];
  if (remediationError && remediationError !== prevRemediationErrorRef.current) {
    prevRemediationErrorRef.current = remediationError;
    pushToast(remediationError);
  }

  const passCount = results.filter((r) => r.status === "PASS").length;
  const failCount = results.filter((r) => r.status === "FAIL").length;
  const evaluatedCount = results.length;

  const liveScore = totalRules ? Math.round((passCount / totalRules) * 100) : 0;
  const displayScore = status === "done" && score !== null ? score : liveScore;

  const scanProgress =
    status === "done"
      ? 100
      : totalRules
        ? Math.min(99, Math.round((evaluatedCount / totalRules) * 100))
        : 0;

  const normalizedSearch = search.trim().toLowerCase();

  const visibleResults = useMemo(() => {
    return results.filter((r) => {
      const matchesSeverity =
        severityFilter === "all" || r.severity?.toLowerCase() === severityFilter;
      const matchesStatus = matchesStatusFilter(r.status, statusFilter);
      const matchesSearch =
        normalizedSearch.length === 0 || r.title.toLowerCase().includes(normalizedSearch);
      return matchesSeverity && matchesStatus && matchesSearch;
    });
  }, [results, severityFilter, statusFilter, normalizedSearch]);

  // en mode remédiation, on ne montre que les FAIL — l'utilisateur ne doit
  // jamais pouvoir sélectionner une règle déjà PASS, ça n'a pas de sens
  const remediableResults = useMemo(
    () => results.filter((r) => r.status === "FAIL"),
    [results]
  );

  const selectedResult = selectedRuleId
    ? (results.find((r) => r.id === selectedRuleId) ?? null)
    : null;

  const toggleRemediationSelection = (ruleId: string) => {
    setSelectedForRemediation((prev) => {
      const next = new Set(prev);
      if (next.has(ruleId)) {
        next.delete(ruleId);
      } else {
        next.add(ruleId);
      }
      return next;
    });
  };

  const handleStartRemediation = () => {
    const ids = Array.from(selectedForRemediation);
    if (ids.length === 0) return;
    startRemediation(ids);
  };

  return (
    <>
      <ToastContainer toasts={toasts} onDismiss={dismissToast} />

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

      <div className="scan-page">
        <div className="scan-header-row">
          <h1>Résultats</h1>

          {status === "running" && (
            <span className="scan-status-badge is-running">
              <span className="scan-status-dot" aria-hidden="true" />
              Scan en cours
            </span>
          )}

          {status === "done" && (
            <span className="scan-status-badge is-done">
              <span className="scan-status-dot" aria-hidden="true" />
              Scan terminé
            </span>
          )}

          {status === "error" && (
            <span className="scan-status-badge is-error">
              <span className="scan-status-dot" aria-hidden="true" />
              Erreur
            </span>
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

        {/* bouton "Remédier" : uniquement une fois le scan terminé, et
            seulement s'il y a des FAIL à corriger */}
        {status === "done" && failCount > 0 && !remediationMode && (
          <button
            type="button"
            className="scan-remediate-trigger"
            onClick={() => setRemediationMode(true)}
          >
            Remédier ({failCount} règle{failCount > 1 ? "s" : ""} en échec)
          </button>
        )}

        {remediationMode && (
          <section className="scan-remediation-panel" aria-label="Sélection des règles à remédier">
            <div className="scan-remediation-header">
              <h2>Sélectionner les règles à corriger</h2>
              <button
                type="button"
                className="scan-remediation-cancel"
                onClick={() => {
                  setRemediationMode(false);
                  setSelectedForRemediation(new Set());
                }}
                disabled={remediationStatus === "running"}
              >
                Annuler
              </button>
            </div>

            {remediableResults.length === 0 ? (
              <p className="scan-empty">Aucune règle en échec à corriger.</p>
            ) : (
              <div className="scan-result-list">
                {remediableResults.map((r) => (
                  <label key={r.id} className="scan-remediation-row">
                    <input
                      type="checkbox"
                      checked={selectedForRemediation.has(r.id)}
                      onChange={() => toggleRemediationSelection(r.id)}
                      disabled={remediationStatus === "running"}
                    />
                    <ScanResultRow
                      title={r.title}
                      status={r.status}
                      isActive={r.id === selectedRuleId}
                      onClick={() =>
                        setSelectedRuleId((current) => (current === r.id ? null : r.id))
                      }
                    />
                  </label>
                ))}
              </div>
            )}

            <button
              type="button"
              className="scan-remediation-launch"
              onClick={handleStartRemediation}
              disabled={selectedForRemediation.size === 0 || remediationStatus === "running"}
            >
              {remediationStatus === "running"
                ? "Remédiation en cours..."
                : `Corriger ${selectedForRemediation.size} règle${selectedForRemediation.size > 1 ? "s" : ""}`}
            </button>

            {/* résultats de remédiation : réutilise ScanResultRow, la forme
                JSON est identique (id, title, status, fixes, ...) — seuls
                FIXED/ERROR viennent s'ajouter aux statuts déjà gérés */}
            {remediationResults.length > 0 && (
              <div className="scan-result-list scan-remediation-results">
                <p className="scan-remediation-results-label">Résultats de la remédiation</p>
                {remediationResults.map((r, idx) => (
                  <ScanResultRow
                    key={`${r.id}-${idx}`}
                    title={r.title}
                    status={r.status}
                    isActive={false}
                    onClick={() => {}}
                  />
                ))}
              </div>
            )}

            {remediationStatus === "done" && (
              <p className="scan-remediation-done">Remédiation terminée.</p>
            )}
          </section>
        )}

        {!remediationMode && (
          <>
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
                      <button
                        key={s}
                        type="button"
                        className={statusFilter === s ? "is-selected" : ""}
                        onClick={() => setStatusFilter(s)}
                      >
                        {s === "all" ? "Tous" : s === "pass" ? "Réussi" : s === "fail" ? "Échoué" : "Autre"}
                      </button>
                    ))}
                  </div>

                  <div className="rules-filters" role="group" aria-label="Filtrer par sévérité">
                    <span>Sévérité</span>
                    {SEVERITIES.map((sev) => (
                      <button
                        key={sev}
                        type="button"
                        className={severityFilter === sev ? "is-selected" : ""}
                        onClick={() => setSeverityFilter(sev)}
                      >
                        {sev === "all" ? "Toutes" : sev}
                      </button>
                    ))}
                  </div>
                </>
              )}
            </section>

            {visibleResults.length === 0 ? (
              <p className="scan-empty">
                {evaluatedCount === 0 && status === "running"
                  ? "En attente des premiers résultats..."
                  : "Aucun résultat ne correspond à ces filtres."}
              </p>
            ) : (
              <div className="scan-result-list">
                {visibleResults.map((r) => (
                  <ScanResultRow
                    key={r.id}
                    title={r.title}
                    status={r.status}
                    isActive={r.id === selectedRuleId}
                    onClick={() =>
                      setSelectedRuleId((current) => (current === r.id ? null : r.id))
                    }
                  />
                ))}
              </div>
            )}

            <RuleDetailPanel rule={selectedResult} onClose={() => setSelectedRuleId(null)} />
          </>
        )}

        {status === "error" && <p className="scan-error-card">Une erreur est survenue pendant le scan.</p>}
      </div>
    </>
  );
}