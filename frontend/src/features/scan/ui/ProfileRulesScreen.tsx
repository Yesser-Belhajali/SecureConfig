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

  const isViewOnly = mode.kind === "view";
  const profileId = mode.profileId;
  const isCreate = mode.kind === "edit" && !profileId;

  const {
    rules,
    selectedIds,
    loading,
    error,
    hasChanges,
    toggleRule,
    selectAll,
    deselectAll,
    resetToBaseline,
    diff,
  } = useRuleSelection(benchmarkId, mode);

  const [selectedRule, setSelectedRule] = useState<Rule | null>(null);
  const [profileName, setProfileName] = useState("");
  const [profileDescription, setProfileDescription] = useState("");
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [severityFilter, setSeverityFilter] = useState("all");
  const [ruleSearch, setRuleSearch] = useState("");
  const [formOpen, setFormOpen] = useState(false);

  if (loading) {
    return <div className="rules-loading" role="status" aria-label="Chargement des règles"><span /></div>;
  }
  if (error) return <p>Erreur : {error}</p>;

  const { added: addedIds, removed: removedIds } = diff();
  const changesCount = addedIds.length + removedIds.length;
  const activeRulesCount = selectedIds.size;
  const progress = rules.length ? Math.round((activeRulesCount / rules.length) * 100) : 0;

  const normalizedSearch = ruleSearch.trim().toLowerCase();
  const visibleRules = rules.filter((rule) => {
    const matchesSeverity = severityFilter === "all" || rule.severity?.toLowerCase() === severityFilter;
    const matchesSearch = normalizedSearch.length === 0 || rule.title.toLowerCase().includes(normalizedSearch);
    return matchesSeverity && matchesSearch;
  });
  const severities = ["all", "high", "medium", "low", "unknown"];

  const handleSave = async () => {
    setSaving(true);
    setSaveError(null);
    try {
      const { added, removed } = diff();
      const result = await saveProfileSelection(benchmarkId, {
        name: profileName,
        description: profileDescription || undefined,
        base_profile_id: profileId,
        added,
        removed,
      });
      navigate("/scan", {
        state: {
          screen: "profile",
          distributionId: location.state?.distributionId,
          version: location.state?.version,
        },
      });
      void result;
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

  const goToScan = () => {
    if (!profileId) return;
    navigate(`/benchmarks/${benchmarkId}/profiles/${profileId}/scan`, {
      state: { distributionId: location.state?.distributionId, version: location.state?.version },
    });
  };

  const canSave = !isViewOnly && hasChanges && profileName.trim().length > 0 && !saving;

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

          <div style={{ display: "flex", gap: "0.75rem" }}>
            {profileId && (
              <button
                type="button"
                onClick={goToScan}
                style={{
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
                }}
              >
                <span aria-hidden="true">▶</span>
                Lancer le scan
              </button>
            )}

            {isViewOnly && profileId && (
              <button type="button" className="rules-edit-button" onClick={goToEdit}>
                <span aria-hidden="true">✎</span>
                Modifier ce profil
              </button>
            )}
          </div>
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

              <div className="rules-search">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                  <circle cx="11" cy="11" r="8" /><line x1="21" y1="21" x2="16.65" y2="16.65" />
                </svg>
                <input
                  type="text"
                  placeholder="Rechercher une règle par nom..."
                  value={ruleSearch}
                  onChange={(e) => setRuleSearch(e.target.value)}
                  aria-label="Rechercher une règle par nom"
                />
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

            {visibleRules.length === 0 ? (
              <p className="rules-empty">Aucune règle ne correspond à votre recherche.</p>
            ) : (
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
            )}

            <p>{selectedIds.size} règle(s) sélectionnée(s)</p>

            {!isViewOnly && hasChanges && (
              <div className={`rules-savebar ${formOpen ? "is-open" : ""}`}>
                {formOpen && (
                  <div className="rules-savebar-form">
                    <input
                      type="text"
                      placeholder="Nom du profil"
                      value={profileName}
                      onChange={(e) => setProfileName(e.target.value)}
                      disabled={saving}
                      autoFocus
                    />
                    <textarea
                      placeholder="Description (optionnelle)"
                      value={profileDescription}
                      onChange={(e) => setProfileDescription(e.target.value)}
                      disabled={saving}
                    />
                    {saveError && <p className="rules-save-error">Erreur : {saveError}</p>}
                  </div>
                )}

                <div className="rules-savebar-trigger">
                  <span className="rules-savebar-count">
                    {changesCount} modification{changesCount > 1 ? "s" : ""}
                    {addedIds.length > 0 && removedIds.length > 0
                      ? ` (${addedIds.length} ajoutée${addedIds.length > 1 ? "s" : ""}, ${removedIds.length} retirée${removedIds.length > 1 ? "s" : ""})`
                      : addedIds.length > 0
                      ? ` (${addedIds.length} ajoutée${addedIds.length > 1 ? "s" : ""})`
                      : removedIds.length > 0
                      ? ` (${removedIds.length} retirée${removedIds.length > 1 ? "s" : ""})`
                      : ""}
                  </span>

                  {!formOpen ? (
                    <button type="button" className="rules-savebar-open" onClick={() => setFormOpen(true)}>
                      Enregistrer ce profil
                    </button>
                  ) : (
                    <div className="rules-savebar-actions">
                      <button type="button" className="rules-savebar-cancel" onClick={() => setFormOpen(false)} disabled={saving}>
                        Annuler
                      </button>
                      <button type="button" className="rules-savebar-confirm" onClick={handleSave} disabled={!canSave}>
                        {saving ? "Enregistrement..." : isCreate ? "Créer le profil" : "Enregistrer"}
                      </button>
                    </div>
                  )}
                </div>
              </div>
            )}
          </div>

          <RuleDetailPanel rule={selectedRule} onClose={() => setSelectedRule(null)} />
        </div>
      </div>
    </>
  );
}
