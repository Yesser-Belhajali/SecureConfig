import { createBrowserRouter } from "react-router-dom";
import { Root } from "./Root";
import { Home } from "../pages/Home";
import { Scan } from "../pages/Scan";
import { About } from "../pages/About";
import { ViewProfilePage, EditProfilePage, CreateProfilePage } from "../pages/ProfileRulesPage";
import { NotFound } from "../pages/NotFound";
import { ScanRunPage } from "../pages/ScanRunPage";
import { RemediationRunPage } from "../pages/RemediationRunPage";

export const router = createBrowserRouter([
  {
    path: "/",
    element: <Root />,
    children: [
      { index: true, element: <Home /> },
      { path: "scan", element: <Scan /> },
      { path: "benchmarks", element: <Scan /> },
      { path: "benchmarks/:benchmarkId/profiles", element: <Scan /> },
      { path: "about", element: <About /> },
      {
        path: "benchmarks/:benchmarkId/profiles/:profileId/view",
        element: <ViewProfilePage />,
      },
      {
        path: "benchmarks/:benchmarkId/profiles/:profileId/edit",
        element: <EditProfilePage />,
      },
      {
        path: "benchmarks/:benchmarkId/create-profile",
        element: <CreateProfilePage />,
      },
      {
        path: "/benchmarks/:benchmarkId/profiles/:profileId/scan",
        element: <ScanRunPage />,
      },
      {
        path: "/benchmarks/:benchmarkId/profiles/:profileId/remediate",
        element: <RemediationRunPage />,
      },
      { path: "*", element: <NotFound /> },
    ],
  },
]);