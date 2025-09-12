import { NextResponse } from 'next/server';
import type { DashboardData, Alert, PredictionData, Device } from '@/lib/types';
import { staticDashboardData } from '@/lib/data';
import { format } from 'date-fns';

// Use a simple in-memory variable to store the latest dashboard data.
// In a real production app, you would use a database like Firestore or Redis.
let latestDashboardData: DashboardData = JSON.parse(JSON.stringify(staticDashboardData));

// Helper function to generate a random number within a range
const getRandomValue = (min: number, max: number, decimals: number = 2) => {
    return parseFloat((Math.random() * (max - min) + min).toFixed(decimals));
};

// A pool of potential alerts to be triggered randomly.
const alertPool: Omit<Alert, 'id' | 'timestamp'>[] = [
    { level: 'critical', message: 'Overload: System load exceeds capacity.' },
    { level: 'warning', message: 'High Load Warning: System load is approaching maximum capacity.' },
    { level: 'info', message: 'Load Normal: System load has returned to normal levels.' },
    { level: 'critical', message: 'Solar generating but battery not charging. Check connections.' },
    { level: 'warning', message: 'Strong sunlight but panel underperforming.' },
    { level: 'info', message: 'Sudden drop in sunlight detected. Potential cloud cover.' },
    { level: 'critical', message: 'Battery critically low – Discharge risk.' },
];


const generateRandomDashboardData = (): DashboardData => {
    const data = JSON.parse(JSON.stringify(latestDashboardData)) as DashboardData;

    // Randomize metrics for the stat cards
    data.metrics.inverterVoltage = getRandomValue(220, 240);
    data.metrics.inverterCurrent = getRandomValue(5, 11);
    data.metrics.powerFactor = getRandomValue(0.95, 0.99);
    data.metrics.batteryPercentage = getRandomValue(40, 100, 0);
    data.metrics.windSpeed = getRandomValue(0, 15);
    data.metrics.cloudCoverage = getRandomValue(0, 100, 0);
    data.metrics.rain = getRandomValue(0, 5);
    data.metrics.solarPower = getRandomValue(0, 5);
    data.metrics.energyGeneration = getRandomValue(10, 25);
    data.metrics.energyConsumption = getRandomValue(3, 8);

    // Make the charts look like they are updating by randomizing the last value
    const randomizeLastPoint = (arr: any[], key: string, min: number, max: number) => {
        if (arr.length > 0) {
            arr[arr.length - 1][key] = getRandomValue(min, max);
        }
    };
    
    randomizeLastPoint(data.solarGenerationData, 'power', 0, 5);
    randomizeLastPoint(data.batteryLoadData, 'battery', 40, 100);
    randomizeLastPoint(data.batteryLoadData, 'load', 1, 3);
    randomizeLastPoint(data.solarParametersData, 'voltage', 350, 410);
    randomizeLastPoint(data.solarParametersData, 'current', 1, 11);
    randomizeLastPoint(data.acParametersData, 'voltage', 220, 240);
    randomizeLastPoint(data.acParametersData, 'current', 5, 11);

    // Randomize prediction data
    data.predictionData.forEach((point: PredictionData) => {
        if (point.actual) {
            point.actual = getRandomValue(point.actual * 0.95, point.actual * 1.05);
        }
        point.predicted = getRandomValue(point.predicted * 0.95, point.predicted * 1.05);
    });

    // Randomize device status
    data.devices.forEach((device: Device) => {
        device.status = Math.random() > 0.3 ? 'Connected' : 'Disconnected';
    });


    // ~20% chance to add a new alert to simulate real-time events.
    if (Math.random() < 0.2) {
        const randomAlert = alertPool[Math.floor(Math.random() * alertPool.length)];
        const newAlert: Alert = {
            ...randomAlert,
            id: `alert-${Date.now()}-${Math.random()}`,
            timestamp: new Date().toISOString(),
        };
        data.alerts.push(newAlert);
    }

    // Keep the total number of alerts from growing indefinitely.
    if (data.alerts.length > 20) {
        data.alerts.shift();
    }


    latestDashboardData = data;
    return data;
}


export async function GET() {
  // This endpoint is called by the dashboard to get the most recent data.
  // It now returns freshly randomized data on each call to simulate a live feed.
  try {
    const randomData = generateRandomDashboardData();
    return NextResponse.json(randomData);
  } catch (error) {
    console.error('Error generating random dashboard data:', error);
    // If there's an error, fall back to the initial static data.
    return NextResponse.json(staticDashboardData, { status: 500 });
  }
}

export async function POST(request: Request) {
  // This endpoint is called by your ESP32 to send new sensor data.
  // For the simulation, we'll just log that we received it, but in a real
  // scenario, you'd update `latestDashboardData` here.
  try {
    const newData = await request.json();
    console.log("Received data from ESP32:", newData);
    // In a real app, you would update your data store here based on received data,
    // and your alert generation logic would live here.
    // latestDashboardData = { ... }; 
    
    return NextResponse.json({ message: 'Data received successfully' }, { status: 200 });
  } catch (error) {
    console.error('Error processing incoming data:', error);
    return NextResponse.json({ message: 'Invalid data format' }, { status: 400 });
  }
}
