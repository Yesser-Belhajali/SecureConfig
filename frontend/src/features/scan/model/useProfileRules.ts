import { useCallback, useEffect, useState } from "react";
import { getSelectedRulesForProfile } from "./api";
import type { Rule } from "./types";

interface UseProfileRulesResult {
  profileRules: Rule[];
  removedIds: Set<string>;
  loading: boolean;
  error: string | null;
  toggleRule: (ruleId: string) => void;
}

export function useProfileRules(
  benchmarkId: string,
  profileId: string
): UseProfileRulesResult {
  const [profileRules, setProfileRules] = useState<Rule[]>([]);
  const [removedIds, setRemovedIds] = useState<Set<string>>(new Set());
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const loadData = useCallback(() => {
    let cancelled = false;
    setLoading(true);
    setError(null);

    getSelectedRulesForProfile(benchmarkId, profileId)
      .then((selectedRules) => {
        if (cancelled) return;
        setProfileRules(selectedRules);
        setRemovedIds(new Set());
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
    setRemovedIds((prev) => {
      const next = new Set(prev);
      if (next.has(ruleId)) {
        next.delete(ruleId); // recoché -> retiré de la liste des retraits
      } else {
        next.add(ruleId); // décoché -> ajouté à la liste des retraits
      }
      return next;
    });
  }, []);

  return { profileRules, removedIds, loading, error, toggleRule };
}