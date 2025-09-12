# **App Name**: EnergyVision

## Core Features:

- MQTT Data Ingestion: Ingest device data sent via MQTT protocol, ensuring secure and reliable communication.
- HTTP Data Ingestion: Accept device data transmitted via HTTP protocol, providing an alternative communication channel.
- Dashboard Overview: Display key energy parameters (Solar Power Generation, Battery Level, Load Consumption, Grid Status) in a clear, concise manner.
- Interactive Power Generation Graphs: Visualize power generation data through interactive graphs (e.g., line charts, bar charts).
- Battery and Load Graphs: Show real-time Battery and Load data with detailed information such as voltage, current, and historical performance to diagnose device health and prevent downtime. Utilize generative AI as a tool that makes intelligent predictions about optimal battery charging schedules. The app uses an LLM to decide, in which way to include additional data about device context.
- Device Status Monitoring: Present the connection status (Connected/Disconnected) of all registered devices.
- Data Transformation: Transform incoming JSON data to desired output using JS functions

## Style Guidelines:

- Primary color: Deep blue (#3F51B5) to convey reliability and technological focus.
- Background color: Black (#000000) for a modern feel.
- Accent color: Vibrant orange (#FF9800) to highlight important data points and interactive elements.
- Body and headline font: 'Inter' (sans-serif) for a modern, neutral, and easily readable interface.
- Use a consistent set of minimalist icons to represent different energy parameters and device statuses.
- Implement a clean and intuitive layout with a clear navigation bar for easy access to the Dashboard and Devices sections.
- Subtle animations to provide feedback on user interactions and data updates (e.g., loading indicators, data refresh).