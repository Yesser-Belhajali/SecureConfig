import { Outlet, ScrollRestoration } from 'react-router-dom'
import { Header } from '../components/Header'
import { Footer } from '../components/Footer'

export function Root() {
  return (
    <div className="min-h-screen flex flex-col" style={{ backgroundColor: '#ffffff' }}>
      <Header />
      <main className="flex-1">
        <Outlet />
        <ScrollRestoration />
      </main>
      <Footer />
    </div>
  )
}
