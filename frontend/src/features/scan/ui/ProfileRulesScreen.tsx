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
    <div>
      <h2>Règles du profil</h2>

      <div className="rule-list">
        {profileRules.map((rule) => (
          <RuleRow
            key={rule.id}
            rule={rule}
            checked={!removedIds.has(rule.id)}
            onToggle={() => toggleRule(rule.id)}
            onSelect={() => setSelectedRule(rule)}
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

      <RuleDetailPanel rule={selectedRule} onClose={() => setSelectedRule(null)} />
    </div>
  );
}