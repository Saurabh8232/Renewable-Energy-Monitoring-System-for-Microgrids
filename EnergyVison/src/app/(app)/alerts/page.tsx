import { AlertTriangle, Info, ShieldAlert } from 'lucide-react';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { cn } from '@/lib/utils';
import type { Alert, DashboardData } from '@/lib/types';
import { format, formatDistanceToNow } from 'date-fns';
import { headers } from 'next/headers';

const alertIcons = {
  info: <Info className="h-5 w-5 text-blue-500" />,
  warning: <AlertTriangle className="h-5 w-5 text-yellow-500" />,
  critical: <ShieldAlert className="h-5 w-5 text-red-500" />,
};

const alertColors = {
  info: 'border-blue-500/50 bg-blue-500/10',
  warning: 'border-yellow-500/50 bg-yellow-500/10',
  critical: 'border-destructive/50 bg-destructive/10 text-destructive-foreground',
};

async function getAlerts(): Promise<Alert[]> {
  try {
      const host = headers().get('host') || 'localhost:9002';
      const protocol = process.env.NODE_ENV === 'production' ? 'https' : 'http';
      const url = `${protocol}://${host}/api/dashboard-data`;
      
      const response = await fetch(url, {
        next: { revalidate: 1 } // Re-fetch data very frequently.
      });

      if (!response.ok) {
        console.error('Failed to fetch dashboard data, status:', response.status);
        return []; // Fallback to empty array on error
      }
      const data: DashboardData = await response.json();
      return data.alerts.reverse(); // Return and reverse alerts from the API
    } catch (error) {
      console.error('API call failed, returning empty array:', error);
      return [];
    }
}

export default async function AlertsPage() {
  const alerts = await getAlerts();

  return (
    <main className="flex-1 overflow-auto p-4 md:p-6">
      <Card>
        <CardHeader>
          <CardTitle>System Alerts</CardTitle>
          <CardDescription>A log of all system events and notifications.</CardDescription>
        </CardHeader>
        <CardContent>
          <div className="space-y-4">
            {alerts.map((alert) => {
                const alertDate = new Date(alert.timestamp);
                return (
                    <div key={alert.id} className={cn('flex items-start gap-4 rounded-lg border p-4', alertColors[alert.level])}>
                        <div className="mt-1">
                            {alertIcons[alert.level]}
                        </div>
                        <div className="flex-1 space-y-1">
                        <p className="font-medium">{alert.message}</p>
                        <div className="text-sm text-muted-foreground flex items-center gap-2">
                            <span>{formatDistanceToNow(alertDate, { addSuffix: true })}</span>
                            <span className="text-xs text-muted-foreground/80">({format(alertDate, "yyyy-MM-dd HH:mm:ss")})</span>
                        </div>
                        </div>
                    </div>
                );
            })}
          </div>
        </CardContent>
      </Card>
    </main>
  );
}
