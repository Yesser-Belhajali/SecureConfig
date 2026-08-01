import { Outlet } from 'react-router-dom'
import { Header } from '../components/Header'
import { Footer } from '../components/Footer'

export function Root() {
  return (
    <div className="min-h-screen flex flex-col" style={{ backgroundColor: '#0D0F12' }}>
      <Header />
      <main className="flex-1">
        <Outlet />
      </main>
      <Footer />
    </div>
  )
}
