export type Device = {
  id: string;
  name: string;
  status: 'Connected' | 'Disconnected';
  type: string;
};

export type TimeSeriesData = {
  time: string;
  [key: string]: number | string;
};

export type Alert = {
  id:string;
  level: 'info' | 'warning' | 'critical';
  message: string;
  timestamp: string;
};

export type PredictionData = {
  time: string;
  actual?: number;
  predicted: number;
}

export type DashboardMetrics = {
    windSpeed: number;
    cloudCoverage: number;
    rain: number;
    latitude: number;
    longitude: number;
    solarPower: number;
    energyGeneration: number;
    energyConsumption: number;
    inverterVoltage: number;
    inverterCurrent: number;
    batteryPercentage: number;
    powerFactor?: number;
}

export type DashboardData = {
  solarGenerationData: TimeSeriesData[];
  batteryLoadData: TimeSeriesData[];
  solarParametersData: TimeSeriesData[];
  acParametersData: TimeSeriesData[];
  predictionData: PredictionData[];
  alerts: Alert[];
  devices: Device[];
  metrics: DashboardMetrics;
};
