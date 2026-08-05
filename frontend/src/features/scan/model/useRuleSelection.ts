import { useCallback, useEffect, useState } from "react";
import { getAllRules, getSelectedRulesForProfile } from "./api";
import type { Rule } from "./types";

interface UseRuleSelectionResult {
  rules: Rule[];
  selectedIds: Set<string>;
  loading: boolean;
  error: string | null;
  toggleRule: (ruleId: string) => void;
  resetToBaseline: () => void;
  diff: () => { added: string[]; removed: string[] };
}

/**
 * profileId fourni  -> mode "profil" : charge les règles du profil, toutes pré-cochées
 * profileId absent  -> mode "création" : charge toutes les règles du benchmark, rien coché
 */
export function useRuleSelection(
  benchmarkId: string,
  profileId?: string
): UseRuleSelectionResult {
  const [rules, setRules] = useState<Rule[]>([]);
  const [originalIds, setOriginalIds] = useState<Set<string>>(new Set());
  const [selectedIds, setSelectedIds] = useState<Set<string>>(new Set());
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const loadData = useCallback(() => {
    let cancelled = false;
    setLoading(true);
    setError(null);

    const fetchRules = profileId
      ? getSelectedRulesForProfile(benchmarkId, profileId)
      : getAllRules(benchmarkId);

    fetchRules
      .then((fetchedRules) => {
        if (cancelled) return;
        setRules(fetchedRules);

        const initialIds = profileId
          ? new Set(fetchedRules.map((r) => r.id))
          : new Set<string>();

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
  }, [benchmarkId, profileId]);

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

  return { rules, selectedIds, loading, error, toggleRule, resetToBaseline, diff };
}