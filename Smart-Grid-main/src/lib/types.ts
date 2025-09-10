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
