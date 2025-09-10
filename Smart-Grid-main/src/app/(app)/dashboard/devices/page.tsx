import DeviceList from '@/components/devices/device-list';
import { devices } from '@/lib/data';

export default function DevicesPage() {
  return (
    <main className="flex-1 overflow-auto p-4 md:p-6">
      <DeviceList devices={devices} />
    </main>
  );
}
