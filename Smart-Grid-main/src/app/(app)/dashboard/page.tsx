import { Sun, BatteryCharging, Thermometer, CloudSun, Gauge, Zap } from 'lucide-react';
import StatCard from '@/components/dashboard/stat-card';
import PowerCharts from '@/components/dashboard/power-charts';
import type { TimeSeriesData } from '@/lib/types';
import { solarGenerationData, batteryLoadData, solarParametersData, acParametersData } from '@/lib/data';

async function getDashboardData(): Promise<{
  solarGenerationData: TimeSeriesData[],
  batteryLoadData: TimeSeriesData[],
  solarParametersData: TimeSeriesData[],
  acParametersData: TimeSeriesData[],
}> {
  // In a real app, you would fetch this data from your server.
  const baseUrl = process.env.NEXT_PUBLIC_API_BASE_URL || 'http://localhost:9002';

  try {
      const response = await fetch(`${baseUrl}/api/dashboard-data`, {
        next: { revalidate: 60 } // Re-fetch data every 60 seconds
      });
      if (!response.ok) {
        throw new Error('Failed to fetch dashboard data');
      }
      const data = await response.json();
      return {
          solarGenerationData: data.solarGenerationData,
          batteryLoadData: data.batteryLoadData,
          solarParametersData: data.solarParametersData,
          acParametersData: data.acParametersData,
      }
    } catch (error) {
      console.error('API call failed, returning static data:', error);
    }
  
  // Returning static data as a fallback.
  return Promise.resolve({
    solarGenerationData,
    batteryLoadData,
    solarParametersData,
    acParametersData
  });
}

export default async function DashboardPage() {
  const { 
    solarGenerationData: fetchedSolarData, 
    batteryLoadData: fetchedBatteryData,
    solarParametersData: fetchedSolarParams,
    acParametersData: fetchedAcParams,
  } = await getDashboardData();


  return (
    <main className="flex-1 overflow-auto p-4 md:p-6">
      <div className="grid gap-6">
        <div className="grid grid-cols-1 gap-6 sm:grid-cols-2 lg:grid-cols-4">
          <StatCard
            title="Temperature"
            value="25°C"
            icon={Thermometer}
            description="Ambient temperature"
          />
          <StatCard
            title="Illuminance"
            value="800 lux"
            icon={Sun}
            description="Outdoor light level"
          />
          <StatCard
            title="Weather Report"
            value="Partly Cloudy"
            icon={CloudSun}
            description="Light breeze"
          />
          <StatCard
            title="Power Factor"
            value="0.98"
            icon={Gauge}
            description="Optimal efficiency"
          />
          <StatCard
            title="Battery Voltage"
            value="48.2 V"
            icon={BatteryCharging}
            description="Nominal voltage"
          />
          <StatCard
            title="Frequency"
            value="50.1 Hz"
            icon={Zap}
            description="Stable frequency"
          />
           <StatCard
            title="Solar Power"
            value="4.2 kW"
            icon={Sun}
            description="+20.1% from last hour"
          />
           <StatCard
            title="Energy"
            value="15.3 kWh"
            icon={Zap}
            description="Total generated today"
          />
        </div>

        <PowerCharts
          solarData={fetchedSolarData}
          batteryData={fetchedBatteryData}
          solarParamsData={fetchedSolarParams}
          acParamsData={fetchedAcParams}
        />
        
      </div>
    </main>
  );
}
