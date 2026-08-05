import type { Profile, Rule } from "./types";

const API_BASE_URL = "http://localhost:8000";

async function handleResponse<T>(response: Response): Promise<T> {
  if (!response.ok) {
    const errorBody = await response.json().catch(() => null);
    throw new Error(errorBody?.error ?? `Erreur HTTP ${response.status}`);
  }
  return response.json();
}

export async function getProfiles(benchmarkId: string): Promise<Profile[]> {
  const response = await fetch(`${API_BASE_URL}/benchmarks/${benchmarkId}/profiles`);
  return handleResponse<Profile[]>(response);
}

export async function getAllRules(benchmarkId: string): Promise<Rule[]> {
  const response = await fetch(`${API_BASE_URL}/benchmarks/${benchmarkId}/rules`);
  return handleResponse<Rule[]>(response);
}

export async function getSelectedRulesForProfile(
  benchmarkId: string,
  profileId: string
): Promise<Rule[]> {
  const response = await fetch(
    `${API_BASE_URL}/benchmarks/${benchmarkId}/profiles/${profileId}/rules`
  );
  return handleResponse<Rule[]>(response);
}

export interface SaveProfilePayload {
  name: string;
  benchmark_id: string;
  profile_id: string;
  removed: string[];
}

export async function saveProfileSelection(payload: SaveProfilePayload): Promise<void> {
  const response = await fetch(`${API_BASE_URL}/profiles/custom`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  await handleResponse<unknown>(response);
}