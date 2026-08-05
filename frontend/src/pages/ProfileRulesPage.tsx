import { useParams } from "react-router-dom";
import { ProfileRulesScreen } from "../features/scan/ui/ProfileRulesScreen";

export function ProfileRulesPage() {
  const { benchmarkId, profileId } = useParams<{ benchmarkId: string; profileId: string }>();

  if (!benchmarkId || !profileId) {
    return <p>Paramètres manquants dans l'URL.</p>;
  }

  return <ProfileRulesScreen benchmarkId={benchmarkId} profileId={profileId} />;
}