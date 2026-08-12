import { useParams } from "react-router-dom";
import { ScanRunScreen } from "../features/scan/ui/ScanRunScreen";

export function ScanRunPage() {
  const { benchmarkId, profileId } = useParams<{ benchmarkId: string; profileId: string }>();

  if (!benchmarkId || !profileId) return null;

  return <ScanRunScreen benchmarkId={benchmarkId} profileId={profileId} />;
}