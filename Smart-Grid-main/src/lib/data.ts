import type { Device, TimeSeriesData } from './types';

export const devices: Device[] = [
  { id: 'esp32-01', name: 'ESP32', status: 'Connected', type: 'Microcontroller' },
];

export const solarGenerationData: TimeSeriesData[] = [
  { time: '00:00', power: 0 },
  { time: '02:00', power: 0 },
  { time: '04:00', power: 0 },
  { time: '06:00', power: 0.5 },
  { time: '08:00', power: 2.1 },
  { time: '10:00', power: 3.5 },
  { time: '12:00', power: 4.2 },
  { time: '14:00', power: 3.8 },
  { time: '16:00', power: 2.5 },
  { time: '18:00', power: 0.8 },
  { time: '20:00', power: 0 },
  { time: '22:00', power: 0 },
];

export const batteryLoadData: TimeSeriesData[] = [
  { time: '00:00', battery: 60, load: 1.2 },
  { time: '02:00', battery: 55, load: 1.1 },
  { time: '04:00', battery: 50, load: 1.0 },
  { time: '06:00', battery: 52, load: 1.5 },
  { time: '08:00', battery: 60, load: 2.0 },
  { time: '10:00', battery: 75, load: 1.8 },
  { time: '12:00', battery: 85, load: 1.7 },
  { time: '14:00', battery: 90, load: 1.9 },
  { time: '16:00', battery: 88, load: 2.2 },
  { time: '18:00', battery: 82, load: 2.5 },
  { time: '20:00', battery: 75, load: 2.1 },
  { time: '22:00', battery: 68, load: 1.5 },
];

export const solarParametersData: TimeSeriesData[] = [
  { time: '00:00', voltage: 0, current: 0 },
  { time: '02:00', voltage: 0, current: 0 },
  { time: '04:00', voltage: 0, current: 0 },
  { time: '06:00', voltage: 350, current: 1.5 },
  { time: '08:00', voltage: 380, current: 5.5 },
  { time: '10:00', voltage: 400, current: 8.8 },
  { time: '12:00', voltage: 410, current: 10.2 },
  { time: '14:00', voltage: 405, current: 9.3 },
  { time: '16:00', voltage: 380, current: 6.5 },
  { time: '18:00', voltage: 360, current: 2.2 },
  { time: '20:00', voltage: 0, current: 0 },
  { time: '22:00', voltage: 0, current: 0 },
];

export const acParametersData: TimeSeriesData[] = [
  { time: '00:00', voltage: 228, current: 5.2 },
  { time: '02:00', voltage: 225, current: 5.1 },
  { time: '04:00', voltage: 226, current: 5.0 },
  { time: '06:00', voltage: 230, current: 6.5 },
  { time: '08:00', voltage: 232, current: 8.1 },
  { time: '10:00', voltage: 231, current: 7.9 },
  { time: '12:00', voltage: 233, current: 7.5 },
  { time: '14:00', voltage: 230, current: 8.2 },
  { time: '16:00', voltage: 229, current: 9.0 },
  { time: '18:00', voltage: 235, current: 10.5 },
  { time: '20:00', voltage: 232, current: 8.5 },
  { time: '22:00', voltage: 230, current: 6.8 },
];
