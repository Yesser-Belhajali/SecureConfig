// features/scan/model/useScanStream.ts
import { useEffect, useMemo, useRef, useState } from "react";
import { API_BASE_URL, getSelectedRulesForProfile } from "./api";
import type { RuleResult } from "./types";

type ScanEvent =
  | (RuleResult & { type: "result" })
  | { type: "done"; score: number };

const SCORABLE_STATUSES = new Set(["PASS", "FAIL", "FIXED"]);

export function useScanStream(benchmarkId: string, profileId: string, autoStart = false) {
  const [inProgressTitle, setInProgressTitle] = useState<string | null>(null);
  const [results, setResults] = useState<RuleResult[]>([]);
  const [totalRules, setTotalRules] = useState<number | null>(null);
  const [score, setScore] = useState<number | null>(null);
  const [status, setStatus] = useState<"idle" | "running" | "done" | "error">("idle");
  const esRef = useRef<EventSource | null>(null);
  const startingRef = useRef(false);
  const mountedRef = useRef(false);

  const start = async () => {
    if (esRef.current || startingRef.current) return;
    startingRef.current = true;

    setResults([]);
    setScore(null);
    setTotalRules(null);
    setStatus("running");

    try {
      const selectedRules = await getSelectedRulesForProfile(benchmarkId, profileId);
      setTotalRules(selectedRules.length);
    } catch {
      setTotalRules(null);
    }

    if (!mountedRef.current) {
      startingRef.current = false;
      return;
    }

    const url = `${API_BASE_URL}/benchmarks/${benchmarkId}/profiles/${profileId}/scan`;
    const es = new EventSource(url);
    esRef.current = es;
    startingRef.current = false;

    es.onmessage = (e) => {
      const data: ScanEvent = JSON.parse(e.data);

      if (data.type === "result") {
        setInProgressTitle(data.title);
        setResults((prev) => [...prev, data]);
      } else if (data.type === "done") {
        setScore(data.score);
        setStatus("done");
        setInProgressTitle(null);
        es.close();
        esRef.current = null;
      }
    };

    es.onerror = () => {
      setStatus("error");
      es.close();
      esRef.current = null;
    };
  };

  useEffect(() => {
    mountedRef.current = true;
    if (autoStart) start();
    return () => {
      mountedRef.current = false;
      esRef.current?.close();
      esRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const liveScore = useMemo(() => {
    const scorable = results.filter((r) => SCORABLE_STATUSES.has(r.status));
    if (scorable.length === 0) return null;

    let weightedPass = 0;
    let weightedTotal = 0;
    for (const r of scorable) {
      weightedTotal += r.weight;
      if (r.status === "PASS") weightedPass += r.weight;
    }
    return weightedTotal > 0 ? (weightedPass / weightedTotal) * 100 : null;
  }, [results]);

  return { start, inProgressTitle, results, totalRules, score, liveScore, status };
}