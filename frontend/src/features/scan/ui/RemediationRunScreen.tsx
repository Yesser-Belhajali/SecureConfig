// features/scan/ui/RemediationRunScreen.tsx
import { useEffect, useMemo, useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import { useRemediationStream } from "../model/useRemediationStream";
import ScanResultRow from "./ScanResultRow";
import RuleDetailPanel from "./RuleDetailPanel";
import ScanScoreCircle from "./ScanScoreCircle";
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

export function RemediationRunScreen({ benchmarkId, profileId }: RemediationRunScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const navigationState = location.state as RemediationLocationState | null;
  const failedRules = useMemo(() => navigationState?.failedRules ?? [], [navigationState]);

  const { start, results, status, liveScore, score, errorMessage } = useRemediationStream(
    benchmarkId,
    profileId
  );

  const [severityFilter, setSeverityFilter] = useState("all");
  const [search, setSearch] = useState("");
  const [selectedRuleId, setSelectedRuleId] = useState<string | null>(null);
  const [selectedForRemediation, setSelectedForRemediation] = useState<Set<string>>(
    () => new Set(failedRules.map((r) => r.id))
  );
  const [launched, setLaunched] = useState(false);
  const [launchedIds, setLaunchedIds] = useState<string[]>([]);
  const [confirmOpen, setConfirmOpen] = useState(false);

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

  // "Annuler" vide la sélection sans naviguer -> naviguer vers /scan
  // remonterait ScanRunScreen avec autoStart=true et relancerait tout un
  // nouveau scan, ce qui n'est pas le comportement voulu ici
  const handleCancelSelection = () => {
    deselectAll();
  };

  const confirmLaunch = () => {
    const ids = Array.from(selectedForRemediation);
    if (ids.length === 0) return;
    setLaunchedIds(ids);
    setLaunched(true);
    setConfirmOpen(false);
    start(ids);
  };

  // le flux /remediate envoie d'abord un event par règle sélectionnée (phase
  // de re-vérification, FAIL attendu), puis un second event par règle (phase
  // de remédiation, FIXED/ERROR) -- scindé par position dans `results`
  const verificationPhase = results.slice(0, launchedIds.length);
  const remediationPhase = results.slice(launchedIds.length);
  const remediationStarted = remediationPhase.length > 0;

  const applyFilters = (list: RuleResult[]) =>
    list.filter((r) => {
      const matchesSeverity = severityFilter === "all" || r.severity?.toLowerCase() === severityFilter;
      const matchesSearch = normalizedSearch.length === 0 || r.title.toLowerCase().includes(normalizedSearch);
      return matchesSeverity && matchesSearch;
    });

  const filteredVerification = useMemo(() => applyFilters(verificationPhase), [verificationPhase, severityFilter, normalizedSearch]);
  const filteredRemediation = useMemo(() => applyFilters(remediationPhase), [remediationPhase, severityFilter, normalizedSearch]);

  const selectedResult = selectedRuleId
    ? (remediationPhase.find((r) => r.id === selectedRuleId) ??
       verificationPhase.find((r) => r.id === selectedRuleId) ??
       failedRules.find((r) => r.id === selectedRuleId) ??
       null)
    : null;

  const totalWeight = launchedIds.length > 0 ? launchedIds.length : failedRules.length || 1;
  const progressFraction = remediationStarted ? remediationPhase.length / totalWeight : 0;
  const displayScore =
    status === "done" && score !== null
      ? score
      : liveScore !== null
        ? liveScore * (remediationStarted ? progressFraction : 0)
        : 0;

  const badgeVariant = status === "error" ? "error" : status === "done" && remediationStarted ? "done" : "running";
  const badgeLabel =
    status === "error"
      ? "Erreur"
      : !remediationStarted
        ? "Scan en cours"
        : status === "running"
          ? "Remédiation en cours"
          : "Remédiation terminée";

  return (
    <>
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
              <div className="scan-score-top">
                <div className="scan-score-side is-pass">
                  <span className="scan-score-side-value">
                    {remediationPhase.filter((r) => r.status === "FIXED").length}
                  </span>
                  <span className="scan-score-side-label">Corrigées</span>
                </div>

                <div className="scan-score-center">
                  <ScanScoreCircle score={displayScore} />
                  <p className="scan-score-top-label">Score de conformité</p>
                </div>

                <div className="scan-score-side is-fail">
                  <span className="scan-score-side-value">
                    {remediationPhase.filter((r) => r.status === "ERROR").length}
                  </span>
                  <span className="scan-score-side-label">Échouées</span>
                </div>
              </div>
            )}

            <section className="scan-controls" aria-label="Filtres">
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

                <section className="scan-remediation-panel" aria-label="Résultats de la remédiation">
                  <div className="scan-result-list">
                    {filteredRemediation.map((r) => (
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
                </section>
              </>
            )}

            {status === "error" && errorMessage && <p className="scan-error-card">{errorMessage}</p>}
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