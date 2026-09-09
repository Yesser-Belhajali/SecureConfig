// features/scan/model/useRemediationStream.ts
import { useEffect, useRef, useState } from "react";
import { API_BASE_URL } from "./api";
import type { RuleResult } from "./types";

type RemediateEvent =
  | (RuleResult & { type: "result" })
  | { type: "done"; score: number }
  | { type: "error"; message: string };

export function useRemediationStream(benchmarkId: string, profileId: string) {
  const [results, setResults] = useState<RuleResult[]>([]);
  const [score, setScore] = useState<number | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [status, setStatus] = useState<"idle" | "running" | "done" | "error">("idle");
  const abortRef = useRef<AbortController | null>(null);
  const mountedRef = useRef(false);

  const start = async (ruleIds: string[]) => {
    if (abortRef.current || ruleIds.length === 0) return;

    setResults([]);
    setScore(null);
    setErrorMessage(null);
    setStatus("running");

    const controller = new AbortController();
    abortRef.current = controller;

    try {
      const response = await fetch(
        `${API_BASE_URL}/benchmarks/${benchmarkId}/profiles/${profileId}/remediate`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ rule_ids: ruleIds }),
          signal: controller.signal,
        }
      );

      if (!response.ok) {
        const body = await response.json().catch(() => null);
        setErrorMessage(body?.error ?? `Erreur HTTP ${response.status}`);
        setStatus("error");
        abortRef.current = null;
        return;
      }

      if (!response.body) {
        setErrorMessage("Pas de flux de réponse");
        setStatus("error");
        abortRef.current = null;
        return;
      }

      const reader = response.body.getReader();
      const decoder = new TextDecoder();
      let buffer = "";

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });

        const parts = buffer.split("\n\n");
        buffer = parts.pop() ?? "";

        for (const part of parts) {
          if (!part.startsWith("data: ")) continue;
          const json = part.slice(6);
          if (!json) continue;

          let data: RemediateEvent;
          try {
            data = JSON.parse(json);
          } catch {
            continue;
          }

          if (data.type === "result") {
            setResults((prev) => [...prev, data]);
          } else if (data.type === "done") {
            setScore(data.score);
            setStatus("done");
          } else if (data.type === "error") {
            setErrorMessage(data.message);
          }
        }
      }

      setStatus((current) => (current === "running" ? "error" : current));
    } catch (err) {
      if ((err as Error).name !== "AbortError") {
        setErrorMessage((err as Error).message);
        setStatus("error");
      }
    } finally {
      abortRef.current = null;
    }
  };

  const cancel = () => {
    abortRef.current?.abort();
    abortRef.current = null;
  };

  useEffect(() => {
    mountedRef.current = true;
    return () => {
      mountedRef.current = false;
      abortRef.current?.abort();
      abortRef.current = null;
    };
  }, []);

  return { start, cancel, results, score, errorMessage, status };
}