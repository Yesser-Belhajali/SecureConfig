import { useEffect, useState } from "react";
import { useLocation } from "react-router-dom";
import { getProfiles, deleteProfile as deleteProfileRequest } from "./api";
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
  tailoringProfiles: Profile[];
  loadingProfiles: boolean;
  profilesError: string | null;
  deletingProfileId: string | null;
  deleteErrors: Record<string, string>;
  toggleVersion: (distributionId: string, version: string) => void;
  resetSystemChoice: () => void;
  goToProfileScreen: () => void;
  goToSystemScreen: () => void;
  setSelectedProfile: (profileId: string) => void;
  deleteProfile: (profileId: string) => void;
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
  const [tailoringProfiles, setTailoringProfiles] = useState<Profile[]>([]);
  const [loadingProfiles, setLoadingProfiles] = useState(false);
  const [profilesError, setProfilesError] = useState<string | null>(null);

  const [deletingProfileId, setDeletingProfileId] = useState<string | null>(null);
  const [deleteErrors, setDeleteErrors] = useState<Record<string, string>>({});

  const selectedDistribution = distributions.find((item) => item.id === distributionId);
  const benchmarkId = selectedDistribution ? buildBenchmarkId(distributionId, version) : "";
  const canContinue = Boolean(benchmarkId);

  // Resynchronise l'état interne à chaque nouvelle navigation vers /scan,
  // même si React ne démonte pas le composant entre deux visites (même route).
  // Dépendances sur des primitives (string), pas sur l'objet navigationState
  // lui-même : location.state est recréé à chaque render, donc le mettre
  // directement en dépendance provoquerait une boucle infinie de l'effet.
  useEffect(() => {
    if (navigationState?.screen) setScreen(navigationState.screen);
    if (navigationState?.distributionId !== undefined) setDistributionId(navigationState.distributionId);
    if (navigationState?.version !== undefined) setVersion(navigationState.version);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [navigationState?.screen, navigationState?.distributionId, navigationState?.version]);

  useEffect(() => {
    if (screen !== "profile" || !benchmarkId) return;

    let cancelled = false;
    setLoadingProfiles(true);
    setProfilesError(null);
    setSelectedProfile("");

    getProfiles(benchmarkId)
      .then((data) => {
        if (!cancelled) {
          setProfiles(data.profiles ?? []);
          setTailoringProfiles(data.tailoring_profiles ?? []);
        }
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

  const deleteProfile = (profileId: string) => {
    if (deletingProfileId) return; // une suppression à la fois

    setDeletingProfileId(profileId);
    setDeleteErrors((current) => {
      const next = { ...current };
      delete next[profileId];
      return next;
    });

    deleteProfileRequest(benchmarkId, profileId)
      .then(() => {
        setTailoringProfiles((current) => current.filter((p) => p.id !== profileId));
        setSelectedProfile((current) => (current === profileId ? "" : current));
      })
      .catch((err) => {
        setDeleteErrors((current) => ({ ...current, [profileId]: err.message }));
      })
      .finally(() => {
        setDeletingProfileId(null);
      });
  };

  return {
    screen,
    distributionId,
    version,
    selectedDistribution,
    benchmarkId,
    canContinue,
    selectedProfile,
    profiles,
    tailoringProfiles,
    loadingProfiles,
    profilesError,
    deletingProfileId,
    deleteErrors,
    toggleVersion,
    resetSystemChoice,
    goToProfileScreen,
    goToSystemScreen,
    setSelectedProfile,
    deleteProfile,
  };
}