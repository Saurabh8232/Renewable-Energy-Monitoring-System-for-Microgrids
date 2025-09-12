import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import PredictionChart from '@/components/predictions/prediction-chart';
import { staticDashboardData } from '@/lib/data';
import type { DashboardData, PredictionData } from '@/lib/types';
import { headers } from 'next/headers';

async function getPredictionData(): Promise<PredictionData[]> {
  try {
    const host = headers().get('host') || 'localhost:9002';
    const protocol = process.env.NODE_ENV === 'production' ? 'https' : 'http';
    const url = `${protocol}://${host}/api/dashboard-data`;

    const response = await fetch(url, {
      next: { revalidate: 1 } // Re-fetch data very frequently.
    });

    if (!response.ok) {
      console.error('Failed to fetch prediction data, status:', response.status);
      return staticDashboardData.predictionData; // Fallback to static data
    }
    const data: DashboardData = await response.json();
    return data.predictionData;
  } catch (error) {
    console.error('API call failed, returning static data:', error);
    return staticDashboardData.predictionData;
  }
}

export default async function PredictionsPage() {
    const predictionData = await getPredictionData();
  return (
    <main className="flex-1 overflow-auto p-4 md:p-6">
       <Card>
        <CardHeader>
          <CardTitle>Solar Power Prediction</CardTitle>
          <CardDescription>
            Predicted solar power generation for the next few hours based on historical data and weather forecasts.
          </CardDescription>
        </CardHeader>
        <CardContent>
            <PredictionChart data={predictionData} />
        </CardContent>
      </Card>
    </main>
  );
}
