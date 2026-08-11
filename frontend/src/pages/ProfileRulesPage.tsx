import { useParams } from "react-router-dom";
import { ProfileRulesScreen } from "../features/scan/ui/ProfileRulesScreen";

export function ViewProfilePage() {
  const { benchmarkId, profileId } = useParams<{ benchmarkId: string; profileId: string }>();
  if (!benchmarkId || !profileId) return <p>Paramètres manquants.</p>;
  return <ProfileRulesScreen benchmarkId={benchmarkId} mode={{ kind: "view", profileId }} />;
}

export function EditProfilePage() {
  const { benchmarkId, profileId } = useParams<{ benchmarkId: string; profileId: string }>();
  if (!benchmarkId || !profileId) return <p>Paramètres manquants.</p>;
  return <ProfileRulesScreen benchmarkId={benchmarkId} mode={{ kind: "edit", profileId }} />;
}

export function CreateProfilePage() {
  const { benchmarkId } = useParams<{ benchmarkId: string }>();
  if (!benchmarkId) return <p>Benchmark manquant.</p>;
  return <ProfileRulesScreen benchmarkId={benchmarkId} mode={{ kind: "edit" }} />;
}