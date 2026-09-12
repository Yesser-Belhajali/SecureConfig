// features/scan/ui/RemediationRunScreen.tsx
import { useEffect, useMemo, useRef, useState } from "react";
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

  const [selectedRuleId, setSelectedRuleId] = useState<string | null>(null);
  const [selectedForRemediation, setSelectedForRemediation] = useState<Set<string>>(
    () => new Set(failedRules.map((r) => r.id))
  );
  const [launched, setLaunched] = useState(false);
  const [launchedIds, setLaunchedIds] = useState<string[]>([]);
  const [confirmOpen, setConfirmOpen] = useState(false);

  const prevErrorRef = useRef<string | null>(null);
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

  // découpage basé sur le marqueur explicite remediation_start renvoyé par
  // le backend, pas sur une position devinée
  const remediationStarted = remediationStartIndex !== null;
  const verificationPhase = remediationStarted ? results.slice(0, remediationStartIndex!) : results;
  const remediationPhase = remediationStarted ? results.slice(remediationStartIndex!) : [];

  const fixedCount = remediationPhase.filter((r) => r.status === "FIXED").length;
  const remediationFailedCount = remediationPhase.length - fixedCount;

  // taux de correction simple, pas une moyenne pondérée par weight : la
  // remédiation porte sur un ensemble fixe de règles déjà en échec — 40
  // corrigées sur 50 lancées = 80%, qui augmente au fil des events FIXED.
  // totalRules vient du backend (event "total") -> plus fiable que
  // launchedIds.length si jamais une règle envoyée n'était pas traitée
  const effectiveTotal = totalRules ?? launchedIds.length ?? 1;
  const liveFixRate = remediationStarted && effectiveTotal > 0 ? (fixedCount / effectiveTotal) * 100 : 0;
  const displayScore = status === "done" && score !== null ? score : liveFixRate;

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

  const selectedResult = selectedRuleId
    ? (remediationPhase.find((r) => r.id === selectedRuleId) ??
       verificationPhase.find((r) => r.id === selectedRuleId) ??
       failedRules.find((r) => r.id === selectedRuleId) ??
       null)
    : null;

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
        ? "Remédiation arrêtée"
        : !remediationStarted
          ? "Scan en cours"
          : status === "running"
            ? "Remédiation en cours"
            : "Remédiation terminée";

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
        <div className="scan-header-row scan-header-row--plain">
          <h1>Remédiation</h1>

          {launched && status === "running" && (
            <button type="button" className="scan-stop-button" onClick={cancel}>
              Stopper la remédiation
            </button>
          )}
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
                        onClick={() => setSelectedRuleId((c) => (c === r.id ? null : r.id))}
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
            {results.length > 0 && (
              <div className="scan-summary-bar">
                <ScanScoreCircle score={displayScore} size={128} strokeWidth={10} />
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

            <section className="scan-controls" aria-label="Filtres de vérification">
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
                      isActive={r.id === selectedRuleId}
                      onClick={() => setSelectedRuleId((c) => (c === r.id ? null : r.id))}
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

                <section className="scan-controls" aria-label="Filtres de remédiation">
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
                          isActive={r.id === selectedRuleId}
                          onClick={() => setSelectedRuleId((c) => (c === r.id ? null : r.id))}
                        />
                      ))
                    )}
                  </div>
                </section>
              </>
            )}
          </>
        )}

        <RuleDetailPanel rule={selectedResult} onClose={() => setSelectedRuleId(null)} />
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