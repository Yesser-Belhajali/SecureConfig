import { useCallback, useEffect, useState } from "react";
import { getAllRules, getAllRulesWithSelection, getSelectedRulesForProfile } from "./api";
import type { Rule } from "./types";

interface UseRuleSelectionResult {
  rules: Rule[];
  selectedIds: Set<string>;
  loading: boolean;
  error: string | null;
  toggleRule: (ruleId: string) => void;
  selectAll: () => void;
  deselectAll: () => void;
  resetToBaseline: () => void;
  diff: () => { added: string[]; removed: string[] };
}

export type SelectionMode =
  | { kind: "view-profile"; profileId: string } // retrait uniquement, ne charge que les règles du profil
  | { kind: "edit-profile"; profileId: string } // ajout + retrait, charge tout le benchmark avec l'état du profil
  | { kind: "create-from-scratch" }; // charge tout le benchmark, rien pré-coché

export function useRuleSelection(
  benchmarkId: string,
  mode: SelectionMode
): UseRuleSelectionResult {
  const [rules, setRules] = useState<Rule[]>([]);
  const [originalIds, setOriginalIds] = useState<Set<string>>(new Set());
  const [selectedIds, setSelectedIds] = useState<Set<string>>(new Set());
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // extrait à part pour avoir une dépendance stable et lisible dans useCallback
  const profileId = mode.kind !== "create-from-scratch" ? mode.profileId : undefined;

  const loadData = useCallback(() => {
    let cancelled = false;
    setLoading(true);
    setError(null);

    let fetchRules: Promise<Rule[]>;
    if (mode.kind === "view-profile") {
      fetchRules = getSelectedRulesForProfile(benchmarkId, mode.profileId);
    } else if (mode.kind === "edit-profile") {
      fetchRules = getAllRulesWithSelection(benchmarkId, mode.profileId);
    } else {
      fetchRules = getAllRules(benchmarkId);
    }

    fetchRules
      .then((fetchedRules) => {
        if (cancelled) return;
        setRules(fetchedRules);

        let initialIds: Set<string>;
        if (mode.kind === "view-profile") {
          // cet endpoint ne renvoie déjà que les règles sélectionnées
          initialIds = new Set(fetchedRules.map((r) => r.id));
        } else if (mode.kind === "edit-profile") {
          // toutes les règles sont chargées, seule une partie est cochée au départ
          initialIds = new Set(fetchedRules.filter((r) => r.selected).map((r) => r.id));
        } else {
          initialIds = new Set();
        }

        setOriginalIds(initialIds);
        setSelectedIds(initialIds);
      })
      .catch((err) => {
        if (!cancelled) setError(err.message);
      })
      .finally(() => {
        if (!cancelled) setLoading(false);
      });

    return () => {
      cancelled = true;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [benchmarkId, mode.kind, profileId]);

  useEffect(() => {
    const cleanup = loadData();
    return cleanup;
  }, [loadData]);

  const toggleRule = useCallback((ruleId: string) => {
    setSelectedIds((prev) => {
      const next = new Set(prev);
      if (next.has(ruleId)) {
        next.delete(ruleId);
      } else {
        next.add(ruleId);
      }
      return next;
    });
  }, []);

  const selectAll = useCallback(() => {
    setSelectedIds(new Set(rules.map((rule) => rule.id)));
  }, [rules]);

  const deselectAll = useCallback(() => {
    setSelectedIds(new Set());
  }, []);

  const resetToBaseline = useCallback(() => {
    setSelectedIds(new Set(originalIds));
  }, [originalIds]);

  const diff = useCallback(() => {
    const added: string[] = [];
    const removed: string[] = [];

    selectedIds.forEach((id) => {
      if (!originalIds.has(id)) added.push(id);
    });
    originalIds.forEach((id) => {
      if (!selectedIds.has(id)) removed.push(id);
    });

    return { added, removed };
  }, [selectedIds, originalIds]);

  return {
    rules,
    selectedIds,
    loading,
    error,
    toggleRule,
    selectAll,
    deselectAll,
    resetToBaseline,
    diff,
  };
}