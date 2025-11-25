import NavBar from "@/components/common/Navbar"
import HeroSection from "@/components/common/HeroSection"
import FeaturesSection from "@/components/common/FeaturesSection"
import FooterSection from "@/components/common/FooterSection"

export default function LandingPage() {
  return (
    <div className="min-h-screen bg-[#0f1419]">
      <NavBar />
      <HeroSection />
      <FeaturesSection />
      <FooterSection />
    </div>
  )
}
