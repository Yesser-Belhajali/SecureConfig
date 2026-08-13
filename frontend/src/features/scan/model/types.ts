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



export interface RuleReference {
  href: string;
  text: string;
}

export interface RuleFix {
  system: string;
  content: string;
}

export interface Rule {
  id: string;
  title: string;
  description: string;
  rationale: string;
  severity: string;
  question: string;
  references: RuleReference[];
  fixes: RuleFix[];
  selected?: boolean; // présent seulement quand la donnée vient de /rules/all
}

export interface SaveProfilePayload {
  name: string;
  description?: string;
  base_profile_id?: string; // absent = from-scratch, cf. create_tailoring_profile(base_profile_id=NULL)
  added: string[];
  removed: string[];
}

// Réponse du backend après création réussie (mappe le new_id renvoyé par create_tailoring_profile).
export interface SaveProfileResponse {
  id: string;
}