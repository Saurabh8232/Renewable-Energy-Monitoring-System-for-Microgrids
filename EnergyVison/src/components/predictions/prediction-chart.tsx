'use client';
import {
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
  ChartLegend,
  ChartLegendContent,
} from '@/components/ui/chart';
import { Area, AreaChart, CartesianGrid, XAxis, YAxis, Line, ComposedChart } from 'recharts';
import type { PredictionData } from '@/lib/types';

const chartConfig = {
  actual: {
    label: 'Actual (kW)',
    color: 'hsl(var(--chart-1))',
  },
  predicted: {
    label: 'Predicted (kW)',
    color: 'hsl(var(--chart-2))',
  },
};

interface PredictionChartProps {
  data: PredictionData[];
}

export default function PredictionChart({ data }: PredictionChartProps) {
  return (
    <div className="h-[300px] w-full">
      <ChartContainer config={chartConfig} className="h-full w-full">
        <ComposedChart data={data} margin={{ left: -20, right: 10, top: 10, bottom: 10 }}>
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
          <ChartLegend content={<ChartLegendContent />} />
          <Area
            dataKey="actual"
            type="monotone"
            fill="var(--color-actual)"
            fillOpacity={0.4}
            stroke="var(--color-actual)"
            stackId="a"
          />
          <Line
            dataKey="predicted"
            type="monotone"
            stroke="var(--color-predicted)"
            strokeWidth={2}
            dot={false}
            strokeDasharray="5 5"
          />
        </ComposedChart>
      </ChartContainer>
    </div>
  );
}
