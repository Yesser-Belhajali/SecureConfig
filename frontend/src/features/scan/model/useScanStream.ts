// features/scan/model/useScanStream.ts
import { useEffect, useRef, useState } from "react";
import { API_BASE_URL, getSelectedRulesForProfile } from "./api";

type ScanEvent =
  | { type: "start"; rule_id: string; title: string }
  | { type: "result"; rule_id: string; status: string; severity: string }
  | { type: "done"; score: number };

interface RuleResult {
  rule_id: string;
  title: string;
  status: string;
  severity: string;
}

export function useScanStream(benchmarkId: string, profileId: string, autoStart = false) {
  const [inProgressTitle, setInProgressTitle] = useState<string | null>(null);
  const [results, setResults] = useState<RuleResult[]>([]);
  const [totalRules, setTotalRules] = useState<number | null>(null);
  const [score, setScore] = useState<number | null>(null);
  const [status, setStatus] = useState<"idle" | "running" | "done" | "error">("idle");
  const esRef = useRef<EventSource | null>(null);
  const titlesRef = useRef<Map<string, string>>(new Map());
  const startingRef = useRef(false);
  const mountedRef = useRef(false);

  const start = async () => {
    if (esRef.current || startingRef.current) return;
    startingRef.current = true; // verrou synchrone, posé immédiatement

    setResults([]);
    setScore(null);
    setTotalRules(null);
    setStatus("running");
    titlesRef.current = new Map();

    try {
      const selectedRules = await getSelectedRulesForProfile(benchmarkId, profileId);
      setTotalRules(selectedRules.length);
    } catch {
      // le total sert uniquement à la barre de progression - son absence ne
      // doit pas empêcher le scan lui-même de démarrer
      setTotalRules(null);
    }

    // si le composant a été démonté (StrictMode cleanup) pendant l'await,
    // on n'ouvre pas la connexion
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

      if (data.type === "start") {
        titlesRef.current.set(data.rule_id, data.title);
        setInProgressTitle(data.title);
      } else if (data.type === "result") {
        const title = titlesRef.current.get(data.rule_id) ?? data.rule_id;
        setResults((prev) => [...prev, { ...data, title }]);
        setInProgressTitle(null);
      } else if (data.type === "done") {
        setScore(data.score);
        setStatus("done");
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

  return { start, inProgressTitle, results, totalRules, score, status };
}