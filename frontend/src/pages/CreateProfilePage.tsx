import { useParams } from "react-router-dom";
import { ProfileRulesScreen } from "../features/scan/ui/ProfileRulesScreen";

export function CreateProfilePage() {
  const { benchmarkId } = useParams<{ benchmarkId: string }>();

  if (!benchmarkId) return <p>Benchmark manquant.</p>;

  return <ProfileRulesScreen benchmarkId={benchmarkId} />;
}