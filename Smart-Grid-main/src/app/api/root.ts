import { NextResponse } from 'next/server';
import { 
  solarGenerationData, 
  batteryLoadData, 
  solarParametersData, 
  acParametersData 
} from '@/lib/data';

export async function GET() {
  // In a real-world application, you would fetch live data from your sensors
  // or a database here. For now, we are returning the static data.
  const data = {
    solarGenerationData,
    batteryLoadData,
    solarParametersData,
    acParametersData
  };

  return NextResponse.json(data);
}
