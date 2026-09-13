// features/scan/model/useScanStream.ts
import { useEffect, useMemo, useRef, useState } from "react";
import { API_BASE_URL, cancelActiveOperation } from "./api"; // 1. AJOUT DE L'IMPORT
import type { RuleResult } from "./types";

type ScanEvent =
  | (RuleResult & { type: "result" })
  | { type: "total"; count: number }
  | { type: "done"; score: number }
  | { type: "error"; message: string };

const UNSCORED_STATUSES = new Set(["NOT_SELECTED", "NOT_APPLICABLE", "INFORMATIONAL", "NOT_CHECKED"]);
const PASSING_STATUSES = new Set(["PASS", "FIXED"]);

export function useScanStream(benchmarkId: string, profileId: string, autoStart = false) {
  const [inProgressTitle, setInProgressTitle] = useState<string | null>(null);
  const [results, setResults] = useState<RuleResult[]>([]);
  const [totalRules, setTotalRules] = useState<number | null>(null);
  const [score, setScore] = useState<number | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [status, setStatus] = useState<"idle" | "running" | "done" | "error" | "cancelled">("idle");
  const esRef = useRef<EventSource | null>(null);
  const startingRef = useRef(false);

  const start = () => {
    if (esRef.current || startingRef.current) return;
    startingRef.current = true;

    setResults([]);
    setScore(null);
    setTotalRules(null);
    setStatus("running");
    setErrorMessage(null); // Nettoyage de l'erreur au démarrage

    const url = `${API_BASE_URL}/benchmarks/${benchmarkId}/profiles/${profileId}/scan`;
    const es = new EventSource(url);
    esRef.current = es;
    startingRef.current = false;

    es.onmessage = (e) => {
      const data: ScanEvent = JSON.parse(e.data);

      if (data.type === "total") {
        setTotalRules(data.count);
      } else if (data.type === "result") {
        setInProgressTitle(data.title);
        setResults((prev) => [...prev, data]);
      } else if (data.type === "error") {
        setErrorMessage(data.message);
        setStatus("error");
        es.close();
        esRef.current = null;
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

  const stop = () => {
    cancelActiveOperation(); // 2. APPEL À L'API
    if (esRef.current) {
      esRef.current.close();
      esRef.current = null;
    }
    setStatus("cancelled");
  };

  useEffect(() => {
    if (autoStart) start();
    return () => {
      esRef.current?.close();
      esRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const liveScore = useMemo(() => {
    const scorable = results.filter((r) => !UNSCORED_STATUSES.has(r.status));
    if (scorable.length === 0) return null;

    let weightedPass = 0;
    let weightedTotal = 0;
    for (const r of scorable) {
      weightedTotal += r.weight;
      if (PASSING_STATUSES.has(r.status)) weightedPass += r.weight;
    }
    return weightedTotal > 0 ? (weightedPass / weightedTotal) * 100 : null;
  }, [results]);

  // 3. AJOUT DE errorMessage DANS LE RETURN
  return { start, stop, inProgressTitle, results, totalRules, score, liveScore, status, errorMessage }; 
}