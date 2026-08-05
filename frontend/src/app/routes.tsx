import { createBrowserRouter } from "react-router-dom";
import { Root } from "./Root";
import { Home } from "../pages/Home";
import { Scan } from "../pages/Scan";
import { About } from "../pages/About";
import { ProfileRulesPage } from "../pages/ProfileRulesPage";
import { CreateProfilePage } from "../pages/CreateProfilePage";
import { NotFound } from "../pages/NotFound";

export const router = createBrowserRouter([
  {
    path: "/",
    element: <Root />,
    children: [
      { index: true, element: <Home /> },
      { path: "scan", element: <Scan /> },
      { path: "about", element: <About /> },
      {
        path: "benchmarks/:benchmarkId/profiles/:profileId",
        element: <ProfileRulesPage />,
      },
      {
        path: "benchmarks/:benchmarkId/profiles/new",
        element: <CreateProfilePage />,
      },
      { path: "*", element: <NotFound /> },
    ],
  },
]);