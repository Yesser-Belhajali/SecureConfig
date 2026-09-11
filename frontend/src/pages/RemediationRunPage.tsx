import { useParams } from "react-router-dom";
import { RemediationRunScreen } from "../features/scan/ui/RemediationRunScreen";

export function RemediationRunPage() {
  const { benchmarkId, profileId } = useParams<{ benchmarkId: string; profileId: string }>();

  if (!benchmarkId || !profileId) return null;

  return <RemediationRunScreen benchmarkId={benchmarkId} profileId={profileId} />;
}