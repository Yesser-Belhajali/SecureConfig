import { useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import { useRuleSelection } from "../model/useRuleSelection";
import { saveProfileSelection } from "../model/api";
import type { Rule } from "../model/types";
import RuleRow from "./RuleRow";
import RuleDetailPanel from "./RuleDetailPanel";
import "./rule-panel.css";

interface ProfileRulesScreenProps {
  benchmarkId: string;
  profileId?: string; // absent -> mode création
}

export function ProfileRulesScreen({ benchmarkId, profileId }: ProfileRulesScreenProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const isCreate = !profileId;
  const { rules, selectedIds, loading, error, toggleRule, resetToBaseline, diff } =
    useRuleSelection(benchmarkId, profileId);

  const [selectedRule, setSelectedRule] = useState<Rule | null>(null);
  const [profileName, setProfileName] = useState("");
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);

  if (loading) {
    return <div className="rules-loading" role="status" aria-label="Chargement des règles"><span /></div>;
  }
  if (error) return <p>Erreur : {error}</p>;

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

  return (
    <>
      <button type="button" className="rules-back-button" onClick={() => navigate("/scan", { state: { screen: "profile", distributionId: location.state?.distributionId, version: location.state?.version } })}>
        <span aria-hidden="true">←</span>
        Retour aux profils
      </button>

    <div className="rules-page">
      <h2>{isCreate ? "Créer un profil personnalisé" : "Règles du profil"}</h2>

      <div className="rules-body">
        <div className="rules-main">
          <div className="rule-list">
            {rules.map((rule) => (
              <RuleRow
                key={rule.id}
                rule={rule}
                checked={selectedIds.has(rule.id)}
                active={selectedRule?.id === rule.id}
                onToggle={() => toggleRule(rule.id)}
                onSelect={() => setSelectedRule((current) => (current?.id === rule.id ? null : rule))}
              />
            ))}
          </div>

          <p>{selectedIds.size} règle(s) sélectionnée(s)</p>

          <input
            type="text"
            placeholder="Nom du profil"
            value={profileName}
            onChange={(e) => setProfileName(e.target.value)}
          />

          <button onClick={handleSave} disabled={saving || !profileName}>
            {saving ? "Enregistrement..." : isCreate ? "Créer le profil" : "Valider"}
          </button>

          <button onClick={resetToBaseline} disabled={saving}>
            Réinitialiser
          </button>

          {saveError && <p>Erreur lors de l'enregistrement : {saveError}</p>}
        </div>

        <RuleDetailPanel rule={selectedRule} onClose={() => setSelectedRule(null)} />
      </div>
    </div>
    </>
  );
}