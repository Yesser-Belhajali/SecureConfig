import { useState } from "react";
import { useProfileRules } from "../model/useProfileRules";
import { saveProfileSelection } from "../model/api";
import type { Rule } from "../model/types";
import RuleRow from "./RuleRow";
import RuleDetailPanel from "./RuleDetailPanel";
import "./rule-panel.css";

interface ProfileRulesScreenProps {
  benchmarkId: string;
  profileId: string;
}

export function ProfileRulesScreen({ benchmarkId, profileId }: ProfileRulesScreenProps) {
  const { profileRules, removedIds, loading, error, toggleRule, resetToBaseline } =
    useProfileRules(benchmarkId, profileId);

  const [selectedRule, setSelectedRule] = useState<Rule | null>(null);
  const [profileName, setProfileName] = useState("");
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [severityFilter, setSeverityFilter] = useState("all");
  const activeRulesCount = profileRules.length - removedIds.size;
  const progress = profileRules.length ? Math.round((activeRulesCount / profileRules.length) * 100) : 0;
  const visibleRules = profileRules.filter((rule) => severityFilter === "all" || rule.severity?.toLowerCase() === severityFilter);
  const selectAll = () => profileRules.forEach((rule) => { if (removedIds.has(rule.id)) toggleRule(rule.id); });
  const deselectAll = () => profileRules.forEach((rule) => { if (!removedIds.has(rule.id)) toggleRule(rule.id); });
  const severities = ["all", "high", "medium", "low", "unknown"];

  if (loading) return <p>Chargement des règles du profil...</p>;
  if (error) return <p>Erreur : {error}</p>;

  const handleSave = async () => {
    setSaving(true);
    setSaveError(null);
    try {
      await saveProfileSelection({
        name: profileName,
        benchmark_id: benchmarkId,
        profile_id: profileId,
        removed: Array.from(removedIds),
      });
    } catch (err) {
      setSaveError((err as Error).message);
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="rules-page">
      <h2>Règles du profil</h2>

      <div className="rules-body">
        <div className="rules-main">
          <div className="rule-list">
          <section className="rules-controls" aria-label="Actions et filtres">
            <div className="rules-progress"><div><strong>{activeRulesCount} / {profileRules.length}</strong><span> règles actives</span></div><div className="rules-progress-track"><span style={{ width: `${progress}%` }} /></div></div>
            <div className="rules-bulk-actions"><button type="button" onClick={selectAll}>Tout sélectionner</button><button type="button" onClick={deselectAll}>Tout désélectionner</button><button type="button" onClick={resetToBaseline}>Réinitialiser</button></div>
            <div className="rules-filters" role="group" aria-label="Filtrer par sévérité">
              <span>Sévérité</span>
              {severities.map((severity) => <button key={severity} type="button" className={severityFilter === severity ? "is-selected" : ""} onClick={() => setSeverityFilter(severity)}>{severity === "all" ? "Toutes" : severity}</button>)}
            </div>
          </section>
            {visibleRules.map((rule) => (
              <RuleRow
                key={rule.id}
                rule={rule}
                checked={!removedIds.has(rule.id)}
                active={selectedRule?.id === rule.id}
                onToggle={() => toggleRule(rule.id)}
                onSelect={() => setSelectedRule((current) => (current?.id === rule.id ? null : rule))}
              />
            ))}
          </div>

          <p>{removedIds.size} règle(s) retirée(s) par rapport au profil d'origine</p>

          <input
            type="text"
            placeholder="Nom du profil"
            value={profileName}
            onChange={(e) => setProfileName(e.target.value)}
          />

          <button onClick={handleSave} disabled={saving || !profileName}>
            {saving ? "Enregistrement..." : "Valider"}
          </button>

          <button onClick={resetToBaseline} disabled={saving}>
            Réinitialiser
          </button>

          {saveError && <p>Erreur lors de l'enregistrement : {saveError}</p>}
        </div>

        <RuleDetailPanel rule={selectedRule} onClose={() => setSelectedRule(null)} />
      </div>
    </div>
  );
}