'use client';
import { useState } from 'react';
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from '@/components/ui/table';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import type { Device } from '@/lib/types';
import { cn } from '@/lib/utils';

interface DeviceListProps {
  devices: Device[];
}

export default function DeviceList({ devices: initialDevices }: DeviceListProps) {
  const [devices] = useState(initialDevices || []);
  
  return (
    <Card className="mt-6">
      <CardHeader>
        <CardTitle>All Devices</CardTitle>
        <CardDescription>An overview of all registered devices and their current status.</CardDescription>
      </CardHeader>
      <CardContent>
        <div className="rounded-md border">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead className="w-[180px]">Status</TableHead>
                  <TableHead>Device Name</TableHead>
                  <TableHead>Type</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {devices.map((device) => (
                  <TableRow key={device.id}>
                    <TableCell>
                      <div className="flex items-center gap-2">
                        <span className={cn(
                          "h-2.5 w-2.5 rounded-full",
                          device.status === 'Connected' ? 'bg-green-500' : 'bg-red-500'
                        )}></span>
                        <span className="font-medium">{device.status}</span>
                      </div>
                    </TableCell>
                    <TableCell className="font-medium">{device.name}</TableCell>
                    <TableCell>
                      <Badge variant="outline">{device.type}</Badge>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
        </div>
      </CardContent>
    </Card>
  );
}
