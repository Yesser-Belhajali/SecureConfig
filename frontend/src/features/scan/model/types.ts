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
  // Le backend renvoie toujours ce champ, mais sa valeur n'est fiable/pertinente
  // que pour les données issues de GET /profiles/{id}/rules/all. Sur /rules et
  // /profiles/{id}/rules, il vaut respectivement toujours false et toujours true —
  // ne pas s'y fier dans ces cas. D'où le typage optionnel côté usage.
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