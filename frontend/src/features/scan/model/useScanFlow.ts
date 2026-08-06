import { useEffect, useState } from "react";
import { useLocation } from "react-router-dom";
import { getProfiles } from "./api";
import type { Profile } from "./types";
import { distributions, buildBenchmarkId, type Distribution } from "./distributions";

export type Screen = "system" | "profile";

interface ScanLocationState {
  screen?: Screen;
  distributionId?: string;
  version?: string;
}

interface UseScanFlowResult {
  screen: Screen;
  distributionId: string;
  version: string;
  selectedDistribution: Distribution | undefined;
  benchmarkId: string;
  canContinue: boolean;
  selectedProfile: string;
  profiles: Profile[];
  loadingProfiles: boolean;
  profilesError: string | null;
  toggleVersion: (distributionId: string, version: string) => void;
  resetSystemChoice: () => void;
  goToProfileScreen: () => void;
  goToSystemScreen: () => void;
  setSelectedProfile: (profileId: string) => void;
}

export function useScanFlow(): UseScanFlowResult {
  const location = useLocation();
  const navigationState = location.state as ScanLocationState | null;

  const [screen, setScreen] = useState<Screen>(
    navigationState?.screen === "profile" ? "profile" : "system"
  );
  const [distributionId, setDistributionId] = useState(navigationState?.distributionId ?? "");
  const [version, setVersion] = useState(navigationState?.version ?? "");
  const [selectedProfile, setSelectedProfile] = useState("");

  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [loadingProfiles, setLoadingProfiles] = useState(false);
  const [profilesError, setProfilesError] = useState<string | null>(null);

  const selectedDistribution = distributions.find((item) => item.id === distributionId);
  const benchmarkId = selectedDistribution ? buildBenchmarkId(distributionId, version) : "";
  const canContinue = Boolean(benchmarkId);

  useEffect(() => {
    if (screen !== "profile" || !benchmarkId) return;

    let cancelled = false;
    setLoadingProfiles(true);
    setProfilesError(null);
    setSelectedProfile("");

    getProfiles(benchmarkId)
      .then((data) => {
        if (!cancelled) setProfiles(data);
      })
      .catch((err) => {
        if (!cancelled) setProfilesError(err.message);
      })
      .finally(() => {
        if (!cancelled) setLoadingProfiles(false);
      });

    return () => {
      cancelled = true;
    };
  }, [screen, benchmarkId]);

  const toggleVersion = (nextDistributionId: string, nextVersion: string) => {
    const isSameSelection = distributionId === nextDistributionId && version === nextVersion;
    if (isSameSelection) {
      setDistributionId("");
      setVersion("");
    } else {
      setDistributionId(nextDistributionId);
      setVersion(nextVersion);
    }
  };

  const resetSystemChoice = () => {
    setDistributionId("");
    setVersion("");
  };

  const goToProfileScreen = () => setScreen("profile");
  const goToSystemScreen = () => setScreen("system");

  return {
    screen,
    distributionId,
    version,
    selectedDistribution,
    benchmarkId,
    canContinue,
    selectedProfile,
    profiles,
    loadingProfiles,
    profilesError,
    toggleVersion,
    resetSystemChoice,
    goToProfileScreen,
    goToSystemScreen,
    setSelectedProfile,
  };
}