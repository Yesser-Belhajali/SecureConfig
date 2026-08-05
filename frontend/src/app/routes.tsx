// src/app/routes.ts
import { createBrowserRouter } from "react-router-dom";
import { Root } from "./Root";
import { Home } from "../pages/Home";
import { Scan } from "../pages/Scan";
import { ProfileRulesPage } from "../pages/ProfileRulesPage";
import { NotFound } from "../pages/NotFound";

export const router = createBrowserRouter([
  {
    path: "/",
    element: <Root />,
    children: [
      { index: true, element: <Home /> },
      { path: "scan", element: <Scan /> },
      {
        path: "benchmarks/:benchmarkId/profiles/:profileId",
        element: <ProfileRulesPage />,
      },
      { path: "*", element: <NotFound /> },
    ],
  },
]);