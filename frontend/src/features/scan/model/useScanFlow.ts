import { useEffect, useState } from "react";
import { useNavigate, useParams, useSearchParams } from "react-router-dom";
import { getProfiles, deleteProfile as deleteProfileRequest } from "./api";
import type { Profile } from "./types";
import { distributions, buildBenchmarkId, parseBenchmarkId, type Distribution } from "./distributions";

export type Screen = "system" | "profile";

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
  const navigate = useNavigate();
  const params = useParams<{ benchmarkId?: string }>();
  const [searchParams, setSearchParams] = useSearchParams();

  // L'URL est la seule source de vérité pour l'écran affiché :
  // /benchmarks (système) vs /benchmarks/:benchmarkId/profiles (profil).
  // Plus aucun state React ni location.state pour ça -> refresh, retour
  // arrière du navigateur et liens partagés restent cohérents.
  const screen: Screen = params.benchmarkId ? "profile" : "system";

  const urlBenchmarkId = params.benchmarkId ?? "";
  const parsedFromUrl = urlBenchmarkId ? parseBenchmarkId(urlBenchmarkId) : null;

  // Écran système : distribution/version choisies vivent en query params.
  // Écran profil : on les retrouve à partir du benchmarkId de la route.
  const distributionId =
    screen === "profile" ? (parsedFromUrl?.distributionId ?? "") : (searchParams.get("distribution") ?? "");
  const version =
    screen === "profile" ? (parsedFromUrl?.version ?? "") : (searchParams.get("version") ?? "");

  const [selectedProfile, setSelectedProfile] = useState("");

  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [tailoringProfiles, setTailoringProfiles] = useState<Profile[]>([]);
  const [loadingProfiles, setLoadingProfiles] = useState(false);
  const [profilesError, setProfilesError] = useState<string | null>(null);

  const [deletingProfileId, setDeletingProfileId] = useState<string | null>(null);
  const [deleteErrors, setDeleteErrors] = useState<Record<string, string>>({});

  const selectedDistribution = distributions.find((item) => item.id === distributionId);
  const benchmarkId =
    screen === "profile" ? urlBenchmarkId : (selectedDistribution ? buildBenchmarkId(distributionId, version) : "");
  const canContinue = Boolean(benchmarkId);

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
      setSearchParams({}, { replace: true });
    } else {
      setSearchParams({ distribution: nextDistributionId, version: nextVersion }, { replace: true });
    }
  };

  const resetSystemChoice = () => {
    setSearchParams({}, { replace: true });
  };

  const goToProfileScreen = () => {
    if (!benchmarkId) return;
    navigate(`/benchmarks/${benchmarkId}/profiles`);
  };

  const goToSystemScreen = () => {
    // conserve la sélection courante pour préremplir "Modifier le choix"
    const search =
      distributionId && version ? `?${new URLSearchParams({ distribution: distributionId, version }).toString()}` : "";
    navigate(`/benchmarks${search}`);
  };

  const deleteProfile = (profileId: string) => {
    if (deletingProfileId) return;

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