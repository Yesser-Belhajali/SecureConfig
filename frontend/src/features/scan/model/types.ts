export interface Profile {
  id: string;
  title: string;
}

export interface Rule {
  id: string;
  title: string;
  description: string;
  rationale: string;
  severity: string;
}

export interface SaveProfilePayload {
  name: string;
  benchmark_id: string;
  profile_id?: string;
  added: string[];
  removed: string[];
}