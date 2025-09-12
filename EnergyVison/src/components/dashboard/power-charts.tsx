'use client';

import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import {
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
  ChartLegend,
  ChartLegendContent,
} from '@/components/ui/chart';
import { Area, AreaChart, CartesianGrid, XAxis, YAxis, Bar, BarChart, Line, LineChart, ComposedChart } from 'recharts';
import type { TimeSeriesData } from '@/lib/types';

const chartConfigSolar = {
  power: {
    label: 'Power (kW)',
    color: 'hsl(var(--accent))',
  },
};

const chartConfigBattery = {
    battery: {
      label: 'Battery (%)',
      color: 'hsl(var(--chart-2))',
    },
  };

const chartConfigInverter = {
  voltage: {
    label: 'Voltage (V)',
    color: 'hsl(var(--chart-1))',
  },
  current: {
    label: 'Current (A)',
    color: 'hsl(var(--chart-3))',
  },
  load: {
    label: 'Load (kW)',
    color: 'hsl(var(--chart-5))',
  },
};

interface PowerChartsProps {
  solarData: TimeSeriesData[];
  batteryData: TimeSeriesData[];
  solarParamsData: TimeSeriesData[];
  acParamsData: TimeSeriesData[];
}

export default function PowerCharts({ solarData, batteryData, solarParamsData, acParamsData }: PowerChartsProps) {
  
    // Combine AC parameters and Load data for the inverter chart
    const combinedInverterData = acParamsData.map((ac, index) => ({
        ...ac,
        load: batteryData[index]?.load ?? 0,
      }));

  return (
    <div className="grid grid-cols-1 gap-6 lg:grid-cols-2">
      <Card>
        <CardHeader>
          <CardTitle>Power Generation</CardTitle>
          <CardDescription>Today's solar power generation trend.</CardDescription>
        </CardHeader>
        <CardContent>
          <ChartContainer config={chartConfigSolar} className="h-[250px] w-full">
            <AreaChart data={solarData} margin={{ left: -20, right: 10 }}>
              <defs>
                <linearGradient id="fillPower" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="var(--color-power)" stopOpacity={0.8} />
                  <stop offset="95%" stopColor="var(--color-power)" stopOpacity={0.1} />
                </linearGradient>
              </defs>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="time"
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                fontSize={12}
              />
              <YAxis
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                unit="kW"
                fontSize={12}
              />
              <ChartTooltip
                cursor={false}
                content={<ChartTooltipContent indicator="dot" />}
              />
              <Area
                dataKey="power"
                type="natural"
                fill="url(#fillPower)"
                stroke="var(--color-power)"
                stackId="a"
              />
            </AreaChart>
          </ChartContainer>
        </CardContent>
      </Card>
      <Card>
        <CardHeader>
          <CardTitle>Battery Status</CardTitle>
          <CardDescription>Today's battery level.</CardDescription>
        </CardHeader>
        <CardContent>
          <ChartContainer config={chartConfigBattery} className="h-[250px] w-full">
            <BarChart data={batteryData} margin={{ left: -20, right: 10 }}>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="time"
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                fontSize={12}
              />
              <YAxis yAxisId="left" orientation="left" stroke="var(--color-battery)" tickLine={false} axisLine={false} unit="%" fontSize={12} />
              <ChartTooltip content={<ChartTooltipContent />} />
              <ChartLegend content={<ChartLegendContent />} />
              <Bar dataKey="battery" yAxisId="left" fill="var(--color-battery)" radius={4} />
            </BarChart>
          </ChartContainer>
        </CardContent>
      </Card>
      <Card>
        <CardHeader>
          <CardTitle>Solar Parameters</CardTitle>
          <CardDescription>Real-time solar voltage and current.</CardDescription>
        </CardHeader>
        <CardContent>
          <ChartContainer config={chartConfigInverter} className="h-[250px] w-full">
            <LineChart data={solarParamsData} margin={{ left: -20, right: 10 }}>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="time"
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                fontSize={12}
              />
              <YAxis yAxisId="left" orientation="left" stroke="var(--color-voltage)" tickLine={false} axisLine={false} unit="V" fontSize={12} />
              <YAxis yAxisId="right" orientation="right" stroke="var(--color-current)" tickLine={false} axisLine={false} unit="A" fontSize={12} />
              <ChartTooltip content={<ChartTooltipContent />} />
              <ChartLegend content={<ChartLegendContent />} />
              <Line dataKey="voltage" type="monotone" yAxisId="left" stroke="var(--color-voltage)" strokeWidth={2} dot={false} />
              <Line dataKey="current" type="monotone" yAxisId="right" stroke="var(--color-current)" strokeWidth={2} dot={false} />
            </LineChart>
          </ChartContainer>
        </CardContent>
      </Card>
      <Card>
        <CardHeader>
          <CardTitle>Inverter Parameters</CardTitle>
          <CardDescription>Real-time inverter voltage, current, and system load.</CardDescription>
        </CardHeader>
        <CardContent>
          <ChartContainer config={chartConfigInverter} className="h-[250px] w-full">
            <ComposedChart data={combinedInverterData} margin={{ left: -20, right: 10 }}>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="time"
                tickLine={false}
                axisLine={false}
                tickMargin={8}
                fontSize={12}
              />
              <YAxis yAxisId="left" orientation="left" stroke="var(--color-voltage)" tickLine={false} axisLine={false} unit="V" fontSize={12} />
              <YAxis yAxisId="right" orientation="right" stroke="var(--color-current)" tickLine={false} axisLine={false} unit="A" fontSize={12} />
              <YAxis yAxisId="load" orientation="right" stroke="var(--color-load)" tickLine={false} axisLine={false} unit="kW" domain={[0, 'dataMax + 2']} hide={true} />
              <ChartTooltip content={<ChartTooltipContent />} />
              <ChartLegend content={<ChartLegendContent />} />
              <Line dataKey="voltage" type="monotone" yAxisId="left" stroke="var(--color-voltage)" strokeWidth={2} dot={false} />
              <Line dataKey="current" type="monotone" yAxisId="right" stroke="var(--color-current)" strokeWidth={2} dot={false} />
              <Line dataKey="load" type="monotone" yAxisId="load" stroke="var(--color-load)" strokeWidth={2} dot={false} />
            </ComposedChart>
          </ChartContainer>
        </CardContent>
      </Card>
    </div>
  );
}
