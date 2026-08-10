import { useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import type { SelectionMode } from "../model/useRuleSelection";
import { useRuleSelection } from "../model/useRuleSelection";
import { saveProfileSelection } from "../model/api";
import type { Rule } from "../model/types";
import RuleRow from "./RuleRow";
import RuleDetailPanel from "./RuleDetailPanel";
import "./rule-panel.css";

interface ProfileRulesScreenProps {
  benchmarkId: string;
  mode: SelectionMode;
}

export function ProfileRulesScreen({ benchmarkId, mode }: ProfileRulesScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();

  const isViewOnly = mode.kind === "view-profile";
  const isCreate = mode.kind === "create-from-scratch";
  const profileId = mode.kind !== "create-from-scratch" ? mode.profileId : undefined;

  const {
    rules,
    selectedIds,
    loading,
    error,
    toggleRule,
    selectAll,
    deselectAll,
    resetToBaseline,
    diff,
  } = useRuleSelection(benchmarkId, mode);

  const [selectedRule, setSelectedRule] = useState<Rule | null>(null);
  const [profileName, setProfileName] = useState("");
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [severityFilter, setSeverityFilter] = useState("all");

  if (loading) {
    return <div className="rules-loading" role="status" aria-label="Chargement des règles"><span /></div>;
  }
  if (error) return <p>Erreur : {error}</p>;

  const activeRulesCount = selectedIds.size;
  const progress = rules.length ? Math.round((activeRulesCount / rules.length) * 100) : 0;
  const visibleRules = rules.filter(
    (rule) => severityFilter === "all" || rule.severity?.toLowerCase() === severityFilter
  );
  const severities = ["all", "high", "medium", "low", "unknown"];

  const handleSave = async () => {
    setSaving(true);
    setSaveError(null);
    try {
      const { added, removed } = diff();
      await saveProfileSelection({
        name: profileName,
        benchmark_id: benchmarkId,
        profile_id: profileId,
        added,
        removed,
      });
    } catch (err) {
      setSaveError((err as Error).message);
    } finally {
      setSaving(false);
    }
  };

  const goToEdit = () => {
    if (!profileId) return;
    navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/edit`, {
      state: { distributionId: location.state?.distributionId, version: location.state?.version },
    });
  };

  return (
    <>
      <button
        type="button"
        className="rules-back-button"
        onClick={() => navigate("/scan", { state: { screen: "profile", distributionId: location.state?.distributionId, version: location.state?.version } })}
      >
        <span aria-hidden="true">←</span>
        Retour aux profils
      </button>

      <div className="rules-page">
        <div className="rules-header-row">
          <h2>{isCreate ? "Créer un profil personnalisé" : isViewOnly ? "Consulter le profil" : "Modifier le profil"}</h2>

          {isViewOnly && profileId && (
            <button type="button" className="rules-edit-button" onClick={goToEdit}>
              <span aria-hidden="true">✎</span>
              Modifier ce profil
            </button>
          )}
        </div>

        <div className="rules-body">
          <div className="rules-main">
            <section className="rules-controls" aria-label="Actions et filtres des règles">
              <div className="rules-progress">
                <div>
                  <strong>{activeRulesCount} / {rules.length}</strong>
                  <span> règles sélectionnées</span>
                </div>
                <div className="rules-progress-track" aria-hidden="true">
                  <span style={{ width: `${progress}%` }} />
                </div>
              </div>

              {!isViewOnly && (
                <div className="rules-bulk-actions">
                  <button type="button" onClick={selectAll} disabled={!rules.length || saving}>
                    Tout sélectionner
                  </button>
                  <button type="button" onClick={deselectAll} disabled={!selectedIds.size || saving}>
                    Tout désélectionner
                  </button>
                  <button type="button" onClick={resetToBaseline} disabled={saving}>
                    Réinitialiser
                  </button>
                </div>
              )}

              <div className="rules-filters" role="group" aria-label="Filtrer par sévérité">
                <span>Sévérité</span>
                {severities.map((severity) => (
                  <button
                    key={severity}
                    type="button"
                    className={severityFilter === severity ? "is-selected" : ""}
                    onClick={() => setSeverityFilter(severity)}
                  >
                    {severity === "all" ? "Toutes" : severity}
                  </button>
                ))}
              </div>
            </section>

            <div className="rule-list">
              {visibleRules.map((rule) => (
                <RuleRow
                  key={rule.id}
                  rule={rule}
                  checked={selectedIds.has(rule.id)}
                  active={selectedRule?.id === rule.id}
                  readOnly={isViewOnly}
                  onToggle={() => toggleRule(rule.id)}
                  onSelect={() => setSelectedRule((current) => (current?.id === rule.id ? null : rule))}
                />
              ))}
            </div>

            <p>{selectedIds.size} règle(s) sélectionnée(s)</p>

          </div>

          <RuleDetailPanel rule={selectedRule} onClose={() => setSelectedRule(null)} />
        </div>
      </div>
    </>
  );
}