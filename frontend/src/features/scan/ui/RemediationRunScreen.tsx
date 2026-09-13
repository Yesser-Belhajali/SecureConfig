// features/scan/ui/RemediationRunScreen.tsx
import { useEffect, useMemo, useRef, useState } from "react";
import { cancelActiveOperationOnUnload } from "../model/api";
import { useLocation, useNavigate } from "react-router-dom";
import { useRemediationStream } from "../model/useRemediationStream";
import ScanResultRow from "./ScanResultRow";
import RuleDetailPanel from "./RuleDetailPanel";
import ScanScoreCircle from "./ScanScoreCircle";
import { useToasts, ToastContainer } from "../../../components/Toast";
import type { RuleResult } from "../model/types";
import "./rule-panel.css";

interface RemediationRunScreenProps {
  benchmarkId: string;
  profileId: string;
}

interface RemediationLocationState {
  failedRules?: RuleResult[];
}

const SEVERITIES = ["all", "high", "medium", "low", "unknown"];
const REMEDIATION_RESULTS = ["all", "fixed", "failed"] as const;

const badgeStyle: React.CSSProperties = {
  display: "inline-flex",
  alignItems: "center",
  gap: "0.5rem",
  padding: "0.5rem 1rem",
  borderRadius: "999px",
  fontSize: "0.85rem",
  fontWeight: 600,
};

const rescanButtonStyle: React.CSSProperties = {
  display: "inline-flex",
  alignItems: "center",
  gap: "0.5rem",
  padding: "0.55rem 1.1rem",
  borderRadius: "0.5rem",
  fontSize: "0.85rem",
  fontWeight: 600,
  color: "#fff",
  background: "linear-gradient(135deg, #8b5cf6, #7c3aed)",
  boxShadow: "0 8px 18px rgba(109,40,217,.28)",
  border: "none",
  cursor: "pointer",
};

function badgeColors(variant: "running" | "done" | "error" | "cancelled") {
  switch (variant) {
    case "running":
      return { background: "#ede9fe", color: "#6d28d9" };
    case "done":
      return { background: "#dcfce7", color: "#15803d" };
    case "error":
      return { background: "#fee2e2", color: "#b91c1c" };
    case "cancelled":
      return { background: "#f3f4f6", color: "#4b5563" };
  }
}

function matchesRemediationResultFilter(status: string, filter: (typeof REMEDIATION_RESULTS)[number]): boolean {
  if (filter === "all") return true;
  if (filter === "fixed") return status === "FIXED";
  return status !== "FIXED";
}

export function RemediationRunScreen({ benchmarkId, profileId }: RemediationRunScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const navigationState = location.state as RemediationLocationState | null;
  const failedRules = useMemo(() => navigationState?.failedRules ?? [], [navigationState]);
  const { toasts, pushToast, dismissToast } = useToasts();

  const {
    start,
    cancel,
    results,
    remediationStartIndex,
    totalRules,
    score,
    errorMessage,
    status,
  } = useRemediationStream(benchmarkId, profileId);

  const [severityFilter, setSeverityFilter] = useState("all");
  const [search, setSearch] = useState("");
  const [remediationSearch, setRemediationSearch] = useState("");
  const [remediationSeverityFilter, setRemediationSeverityFilter] = useState("all");
  const [remediationResultFilter, setRemediationResultFilter] = useState<(typeof REMEDIATION_RESULTS)[number]>("all");

  const [selectedResult, setSelectedResult] = useState<RuleResult | null>(null);
  const [selectedForRemediation, setSelectedForRemediation] = useState<Set<string>>(
    () => new Set(failedRules.map((r) => r.id))
  );
  const [launched, setLaunched] = useState(false);
  const [launchedIds, setLaunchedIds] = useState<string[]>([]);
  const [confirmOpen, setConfirmOpen] = useState(false);

  const prevErrorRef = useRef<string | null>(null);


  useEffect(() => {
  if (status !== "running") return;
  const handlePageHide = () => cancelActiveOperationOnUnload();
  window.addEventListener("pagehide", handlePageHide);
  return () => window.removeEventListener("pagehide", handlePageHide);
}, [status]);


  useEffect(() => {
    if (errorMessage && errorMessage !== prevErrorRef.current) {
      prevErrorRef.current = errorMessage;
      pushToast(errorMessage);
    }
    if (!errorMessage) prevErrorRef.current = null;
  }, [errorMessage, pushToast]);

  useEffect(() => {
    if (failedRules.length === 0) {
      navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/scan`, { replace: true });
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const toggleSelection = (ruleId: string) => {
    setSelectedForRemediation((prev) => {
      const next = new Set(prev);
      if (next.has(ruleId)) next.delete(ruleId);
      else next.add(ruleId);
      return next;
    });
  };

  const normalizedSearch = search.trim().toLowerCase();

  const filteredSelection = useMemo(
    () =>
      failedRules.filter((r) => {
        const matchesSeverity = severityFilter === "all" || r.severity?.toLowerCase() === severityFilter;
        const matchesSearch = normalizedSearch.length === 0 || r.title.toLowerCase().includes(normalizedSearch);
        return matchesSeverity && matchesSearch;
      }),
    [failedRules, severityFilter, normalizedSearch]
  );

  const selectAll = () => setSelectedForRemediation(new Set(filteredSelection.map((r) => r.id)));
  const deselectAll = () => setSelectedForRemediation(new Set());
  const handleCancelSelection = () => deselectAll();

  const confirmLaunch = () => {
    const ids = Array.from(selectedForRemediation);
    if (ids.length === 0) return;
    setLaunchedIds(ids);
    setLaunched(true);
    setConfirmOpen(false);
    start(ids);
  };

  const remediationStarted = remediationStartIndex !== null;
  const verificationPhase = remediationStarted ? results.slice(0, remediationStartIndex!) : results;
  const remediationPhase = remediationStarted ? results.slice(remediationStartIndex!) : [];

  const fixedCount = remediationPhase.filter((r) => r.status === "FIXED").length;
  const remediationFailedCount = remediationPhase.length - fixedCount;

  const effectiveTotal = totalRules ?? launchedIds.length ?? 1;
  const liveFixRate = remediationStarted && effectiveTotal > 0 ? (fixedCount / effectiveTotal) * 100 : 0;
  const displayScore = status === "done" && score !== null ? score : liveFixRate;

  const verificationDone = remediationStarted || status === "done" || status === "cancelled" || status === "error";
  const remediationDone = status === "done" || status === "cancelled" || status === "error";

  const verificationProgress = verificationDone
    ? 100
    : effectiveTotal
      ? Math.min(99, Math.round((verificationPhase.length / effectiveTotal) * 100))
      : 0;

  const remediationProgress = remediationDone
    ? 100
    : effectiveTotal
      ? Math.min(99, Math.round((remediationPhase.length / effectiveTotal) * 100))
      : 0;

  const filteredVerification = useMemo(
    () =>
      verificationPhase.filter((r) => {
        const matchesSeverity = severityFilter === "all" || r.severity?.toLowerCase() === severityFilter;
        const matchesSearch = normalizedSearch.length === 0 || r.title.toLowerCase().includes(normalizedSearch);
        return matchesSeverity && matchesSearch;
      }),
    [verificationPhase, severityFilter, normalizedSearch]
  );

  const normalizedRemediationSearch = remediationSearch.trim().toLowerCase();
  const filteredRemediation = useMemo(
    () =>
      remediationPhase.filter((r) => {
        const matchesSeverity =
          remediationSeverityFilter === "all" || r.severity?.toLowerCase() === remediationSeverityFilter;
        const matchesResult = matchesRemediationResultFilter(r.status, remediationResultFilter);
        const matchesSearch =
          normalizedRemediationSearch.length === 0 || r.title.toLowerCase().includes(normalizedRemediationSearch);
        return matchesSeverity && matchesResult && matchesSearch;
      }),
    [remediationPhase, remediationSeverityFilter, remediationResultFilter, normalizedRemediationSearch]
  );

  const badgeVariant =
    status === "error"
      ? "error"
      : status === "cancelled"
        ? "cancelled"
        : status === "done" && remediationStarted
          ? "done"
          : "running";
  const badgeLabel =
    status === "error"
      ? "Erreur"
      : status === "cancelled"
        ? remediationStarted
          ? "Remédiation arrêtée"
          : "Scan arrêté"
        : !remediationStarted
          ? "Scan en cours"
          : status === "running"
            ? "Remédiation en cours"
            : "Remédiation terminée";

  const showSummary = results.length > 0 && (status === "running" || status === "done" || status === "cancelled");
  const showStopButton = launched && status === "running";
  const showRescanButton = launched && remediationDone;

  return (
    <>
      <ToastContainer toasts={toasts} onDismiss={dismissToast} />

      <div className="scan-top-row">
        <button
          type="button"
          className="scan-back-button"
          onClick={() => navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/scan`)}
        >
          <span aria-hidden="true">←</span>
          Retour aux résultats
        </button>

        {launched && (
          <span style={{ ...badgeStyle, ...badgeColors(badgeVariant) }}>
            <span aria-hidden="true">●</span>
            {badgeLabel}
          </span>
        )}
      </div>

      <div className="scan-page">
        <h1 className="scan-page-title">Remédiation</h1>

        {/* même principe que ScanRunScreen : slot de gauche porte le titre
            "Résultats", équilibre le slot d'actions à droite, le score
            reste centré. */}
        <div className="scan-header-row scan-header-row--plain">
          <div className="scan-header-title-slot">
            <h2 className="scan-header-title">Résultats</h2>
          </div>

          <div className="scan-header-summary-slot">
            {showSummary && (
              <div className="scan-header-summary">
                <ScanScoreCircle score={displayScore} size={110} strokeWidth={9} />
                <div className="scan-summary-stats">
                  <div className="scan-summary-stat is-total">
                    <span className="scan-summary-stat-value">{effectiveTotal}</span>
                    <span className="scan-summary-stat-label">Total</span>
                  </div>
                  <div className="scan-summary-stat is-pass">
                    <span className="scan-summary-stat-value">{fixedCount}</span>
                    <span className="scan-summary-stat-label">Corrigées</span>
                  </div>
                  <div className="scan-summary-stat is-fail">
                    <span className="scan-summary-stat-value">{remediationFailedCount}</span>
                    <span className="scan-summary-stat-label">Échouées</span>
                  </div>
                </div>
              </div>
            )}
          </div>

          <div className="scan-header-actions-slot">
            {showStopButton && (
              <button type="button" className="scan-stop-button" onClick={cancel}>
                {remediationStarted ? "Stopper la remédiation" : "Stopper le scan"}
              </button>
            )}
            {showRescanButton && (
              <button
                type="button"
                style={rescanButtonStyle}
                onClick={() => navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/scan`)}
              >
                <span aria-hidden="true">▶</span>
                Re-scanner
              </button>
            )}
          </div>
        </div>

        {!launched ? (
          <>
            <section className="scan-controls" aria-label="Filtres et actions de sélection">
              <div className="rules-search">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                  <circle cx="11" cy="11" r="8" />
                  <line x1="21" y1="21" x2="16.65" y2="16.65" />
                </svg>
                <input
                  type="text"
                  placeholder="Rechercher une règle par nom..."
                  value={search}
                  onChange={(e) => setSearch(e.target.value)}
                  aria-label="Rechercher une règle par nom"
                />
              </div>

              <div className="rules-bulk-actions">
                <button type="button" onClick={selectAll} disabled={filteredSelection.length === 0}>
                  Tout sélectionner
                </button>
                <button type="button" onClick={deselectAll} disabled={selectedForRemediation.size === 0}>
                  Tout désélectionner
                </button>
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
            </section>

            <section className="scan-remediation-panel" aria-label="Sélection des règles à remédier">
              <div className="scan-remediation-header">
                <h2>Sélectionner les règles à corriger</h2>
              </div>

              {filteredSelection.length === 0 ? (
                <p className="rules-empty">Aucune règle ne correspond à ces filtres.</p>
              ) : (
                <div className="rule-list">
                  {filteredSelection.map((r) => (
                    <div key={r.id} className="rule-row">
                      <input
                        className="rule-row-check"
                        type="checkbox"
                        checked={selectedForRemediation.has(r.id)}
                        onChange={() => toggleSelection(r.id)}
                        aria-label={`Sélectionner ${r.title} pour remédiation`}
                      />
                      <button
                        className="rule-row-main"
                        onClick={() => setSelectedResult((current) => (current === r ? null : r))}
                      >
                        <span className="rule-severity-dot severity-high" aria-hidden="true" />
                        <span className="rule-title">{r.title}</span>
                        {r.severity && (
                          <span className={`rule-severity severity-${r.severity.toLowerCase()}`}>{r.severity}</span>
                        )}
                      </button>
                    </div>
                  ))}
                </div>
              )}

              <div className="rules-savebar-trigger" style={{ marginTop: "1.25rem" }}>
                <span className="rules-savebar-count">
                  {selectedForRemediation.size} règle{selectedForRemediation.size > 1 ? "s" : ""} sélectionnée
                  {selectedForRemediation.size > 1 ? "s" : ""}
                </span>

                <div style={{ display: "flex", gap: "0.5rem" }}>
                  <button type="button" className="scan-remediation-cancel" onClick={handleCancelSelection}>
                    Annuler
                  </button>
                  <button
                    type="button"
                    className="scan-remediation-launch"
                    onClick={() => setConfirmOpen(true)}
                    disabled={selectedForRemediation.size === 0}
                  >
                    Corriger la sélection
                  </button>
                </div>
              </div>
            </section>
          </>
        ) : (
          <>
            <section className="scan-controls" aria-label="Progression et filtres de vérification">
              <div className="scan-progress-track" aria-hidden="true">
                <span style={{ width: `${verificationProgress}%` }} />
              </div>

              {status === "running" && !remediationStarted && (
                <div className="scan-in-progress">
                  <span className="scan-in-progress-spinner" aria-hidden="true" />
                  {verificationPhase.length > 0 ? (
                    <span>
                      Vérification en cours : <strong>{verificationPhase[verificationPhase.length - 1].title}</strong>
                    </span>
                  ) : (
                    <span>Démarrage de la vérification...</span>
                  )}
                  <span className="scan-progress-count">
                    ({verificationPhase.length} / {effectiveTotal})
                  </span>
                </div>
              )}

              {verificationDone && (
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
                      aria-label="Rechercher un résultat de vérification par nom"
                    />
                  </div>

                  <div className="rules-filters" role="group" aria-label="Filtrer la vérification par sévérité">
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

            <section className="scan-remediation-panel" aria-label="Résultats de vérification">
              <div className="scan-remediation-header">
                <h2>Vérification</h2>
              </div>
              {filteredVerification.length === 0 ? (
                <p className="scan-empty">En attente des résultats...</p>
              ) : (
                <div className="scan-result-list">
                  {filteredVerification.map((r) => (
                    <ScanResultRow
                      key={r.id}
                      title={r.title}
                      status={r.status}
                      severity={r.severity}
                      isActive={r === selectedResult}
                      onClick={() => setSelectedResult((current) => (current === r ? null : r))}
                    />
                  ))}
                </div>
              )}
            </section>

            {remediationStarted && (
              <>
                <div className="scan-remediation-separator" role="separator" aria-label="Début de la remédiation">
                  <span>Remédiation</span>
                </div>

                <section className="scan-controls" aria-label="Progression et filtres de remédiation">
                  <div className="scan-progress-track" aria-hidden="true">
                    <span style={{ width: `${remediationProgress}%` }} />
                  </div>

                  {status === "running" && (
                    <div className="scan-in-progress">
                      <span className="scan-in-progress-spinner" aria-hidden="true" />
                      {remediationPhase.length > 0 ? (
                        <span>
                          Correction en cours : <strong>{remediationPhase[remediationPhase.length - 1].title}</strong>
                        </span>
                      ) : (
                        <span>Démarrage de la correction...</span>
                      )}
                      <span className="scan-progress-count">
                        ({remediationPhase.length} / {effectiveTotal})
                      </span>
                    </div>
                  )}

                  {remediationDone && (
                    <>
                      <div className="rules-search">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                          <circle cx="11" cy="11" r="8" />
                          <line x1="21" y1="21" x2="16.65" y2="16.65" />
                        </svg>
                        <input
                          type="text"
                          placeholder="Rechercher un résultat de remédiation par nom..."
                          value={remediationSearch}
                          onChange={(e) => setRemediationSearch(e.target.value)}
                          aria-label="Rechercher un résultat de remédiation par nom"
                        />
                      </div>

                      <div className="rules-filters" role="group" aria-label="Filtrer la remédiation par résultat">
                        <span>Résultat</span>
                        {REMEDIATION_RESULTS.map((r) => (
                          <button
                            key={r}
                            type="button"
                            className={remediationResultFilter === r ? "is-selected" : ""}
                            onClick={() => setRemediationResultFilter(r)}
                          >
                            {r === "all" ? "Tous" : r === "fixed" ? "Corrigé" : "Échoué"}
                          </button>
                        ))}
                      </div>

                      <div className="rules-filters" role="group" aria-label="Filtrer la remédiation par sévérité">
                        <span>Sévérité</span>
                        {SEVERITIES.map((sev) => (
                          <button
                            key={sev}
                            type="button"
                            className={remediationSeverityFilter === sev ? "is-selected" : ""}
                            onClick={() => setRemediationSeverityFilter(sev)}
                          >
                            {sev === "all" ? "Toutes" : sev}
                          </button>
                        ))}
                      </div>
                    </>
                  )}
                </section>

                <section className="scan-remediation-panel" aria-label="Résultats de la remédiation">
                  <div className="scan-result-list">
                    {filteredRemediation.length === 0 ? (
                      <p className="scan-empty">Aucun résultat ne correspond à ces filtres.</p>
                    ) : (
                      filteredRemediation.map((r) => (
                        <ScanResultRow
                          key={r.id}
                          title={r.title}
                          status={r.status}
                          severity={r.severity}
                          isActive={r === selectedResult}
                          onClick={() => setSelectedResult((current) => (current === r ? null : r))}
                        />
                      ))
                    )}
                  </div>
                </section>
              </>
            )}
          </>
        )}

        <RuleDetailPanel rule={selectedResult} onClose={() => setSelectedResult(null)} />
      </div>

      {confirmOpen && (
        <div
          role="dialog"
          aria-modal="true"
          aria-label="Confirmer la remédiation"
          style={{
            position: "fixed",
            inset: 0,
            background: "rgba(0,0,0,0.4)",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            zIndex: 100,
          }}
          onClick={() => setConfirmOpen(false)}
        >
          <div
            style={{
              background: "#fff",
              borderRadius: "0.75rem",
              padding: "1.5rem",
              maxWidth: "26rem",
              boxShadow: "0 20px 40px rgba(0,0,0,0.2)",
            }}
            onClick={(e) => e.stopPropagation()}
          >
            <h3 style={{ marginTop: 0 }}>Confirmer la remédiation</h3>
            <p>
              Vous êtes sur le point de corriger {selectedForRemediation.size} règle
              {selectedForRemediation.size > 1 ? "s" : ""}. Cette action modifie la configuration du système. Continuer ?
            </p>
            <div style={{ display: "flex", justifyContent: "flex-end", gap: "0.75rem", marginTop: "1.25rem" }}>
              <button type="button" className="scan-remediation-cancel" onClick={() => setConfirmOpen(false)}>
                Annuler
              </button>
              <button type="button" className="scan-remediation-launch" onClick={confirmLaunch}>
                Confirmer
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}