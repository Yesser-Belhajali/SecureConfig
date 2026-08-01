import { createBrowserRouter } from 'react-router-dom'
import { Root } from './Root'
import { Home } from '../pages/Home'
import { About } from '../pages/About'
import { Scan } from '../pages/Scan'
import { NotFound } from '../pages/NotFound'

export const router = createBrowserRouter([
  {
    path: '/',
    Component: Root,
    children: [
      { index: true, Component: Home },
      { path: 'scan', Component: Scan },
      { path: 'about', Component: About },
      { path: '*', Component: NotFound },
    ],
  },
])
