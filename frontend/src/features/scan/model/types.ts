export interface Profile {
  id: string;
  title: string;
  description?: string;
  extends?: string;
}

export interface ProfilesResponse {
  profiles: Profile[];
  tailoring_profiles: Profile[];
}

export interface Rule {
  id: string;
  title: string;
  description: string;
  rationale: string;
  severity: string;
  selected?: boolean; // présent seulement quand la donnée vient de /rules/all

}

export interface SaveProfilePayload {
  name: string;
  benchmark_id: string;
  profile_id?: string;
  added: string[];
  removed: string[];
}