import { useNavigate } from "react-router-dom";
import type { Profile } from "../model/types";

interface ProfilesListProps {
  benchmarkId: string;
  profiles: Profile[];
}

export default function ProfilesList({ benchmarkId, profiles }: ProfilesListProps) {
  const navigate = useNavigate();

  return (
    <ul>
      {profiles.map((profile) => (
        <li key={profile.id}>
          {profile.title}
          <button
            onClick={() => navigate(`/benchmarks/${benchmarkId}/profiles/${profile.id}`)}
          >
            Ouvrir
          </button>
        </li>
      ))}
    </ul>
  );
}