import type { Profile, ProfilesResponse, Rule, SaveProfilePayload, SaveProfileResponse } from "./types";

const API_BASE_URL = "http://localhost:8000";

async function handleResponse<T>(response: Response): Promise<T> {
  if (!response.ok) {
    const errorBody = await response.json().catch(() => null);
    throw new Error(errorBody?.error ?? `Erreur HTTP ${response.status}`);
  }
  return response.json();
}

export async function getProfiles(benchmarkId: string): Promise<ProfilesResponse> {
  const response = await fetch(`${API_BASE_URL}/benchmarks/${benchmarkId}/profiles`);
  return handleResponse<ProfilesResponse>(response);
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

export async function getAllRulesWithSelection(
  benchmarkId: string,
  profileId: string
): Promise<Rule[]> {
  const response = await fetch(
    `${API_BASE_URL}/benchmarks/${benchmarkId}/profiles/${profileId}/rules/all`
  );
  return handleResponse<Rule[]>(response);
}

// POST /benchmarks/{benchmarkId}/profiles
// benchmarkId est un paramètre séparé, pas un champ du payload : reste cohérent
// avec les autres fonctions de ce fichier et avec extract_benchmark_id côté C,
// qui parse déjà l'id de benchmark depuis l'URL.
export async function saveProfileSelection(
  benchmarkId: string,
  payload: SaveProfilePayload
): Promise<SaveProfileResponse> {
  const response = await fetch(`${API_BASE_URL}/benchmarks/${benchmarkId}/profiles`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  return handleResponse<SaveProfileResponse>(response);
}