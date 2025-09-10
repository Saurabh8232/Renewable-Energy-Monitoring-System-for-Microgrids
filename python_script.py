from flask import Flask, request, jsonify
import requests

app = Flask(__name__)

# ===================== ESP32 + WEATHER STORAGE ==========
THINGSBOARD_TOKEN = None
frequency = power_factor = voltage = current = power = energy = None
solar_voltage = solar_current = solar_power = battery_percentage = light_intensity = None
battery_voltage = inverter_load = overload_status = None
temperature = cloudcover = windspeed = precipitation = None
LAT = LON = IP = None
payload = {}

# ===================== ESP32 ERROR STORAGE ============
E_light = E_solar = E_battery = Epem = Esdcard = None


# ===================== WEATHER DATA ===================

def safe_first(lst):
    """Safely return the first element of a list, or None if empty"""
    return lst[0] if lst and len(lst) > 0 else None

def fetch_weather():
    global temperature, cloudcover, windspeed, precipitation
    try:
        url = (
            f"https://api.open-meteo.com/v1/forecast?"
            f"latitude={LAT}&longitude={LON}&hourly=temperature_2m,cloudcover,windspeed_10m,precipitation"
        )
        response = requests.get(url, timeout=10)
        data = response.json()

        hourly = data.get("hourly", {})
        temperature   = safe_first(hourly.get("temperature_2m", []))
        cloudcover    = safe_first(hourly.get("cloudcover", []))
        windspeed     = safe_first(hourly.get("windspeed_10m", []))
        precipitation = safe_first(hourly.get("precipitation", []))

    except Exception as e:
        print("Error fetching weather:", str(e))
        temperature = cloudcover = windspeed = precipitation = None


# ===================== OVERLOAD CONDITION =============
def overload():
    global overload_status, inverter_load, power

    try:
        if not inverter_load or power is None:
            overload_status = "Invalid or missing load data"
            return

        # Define thresholds dynamically
        warning_limit = inverter_load * 0.90  # 90% load pe warning
        overload_limit = inverter_load * 1.00  # 100% se upar overload

        if power > overload_limit:
            overload_status = f"Overload! ({power:.2f}W > {inverter_load}W)"
        elif power > warning_limit:
            overload_status = f"High Load Warning. ({power:.2f}W / {inverter_load}W)"
        else:
            overload_status = f"Load Normal ({power:.2f} W)"
    except Exception as e:
        overload_status = f"Error in overload logic: {e}"


# ===================== ESP32 DATA =====================
@app.route("/esp32-data", methods=["POST"])
def receive_data():
    global payload, LAT, LON, THINGSBOARD_TOKEN, IP, inverter_load
    global frequency, power_factor, voltage, current, power, energy
    global solar_voltage, solar_current, solar_power, battery_percentage, overload_status
    global light_intensity, battery_voltage, E_light, E_solar, E_battery, Epem, Esdcard

    try:
        data = request.get_json()

        # Update globals
        inverter_load = data.get("InverterLoad")
        frequency = data.get("Frequency")
        power_factor = data.get("PowerFactor")
        voltage = data.get("Voltage")
        current = data.get("Current")
        power = data.get("Power")
        energy = data.get("Energy")
        solar_voltage = data.get("solarVoltage")
        solar_current = data.get("solarCurrent")
        solar_power = data.get("solarPower")
        battery_percentage = data.get("batteryPercentage")
        light_intensity = data.get("lightIntensity")
        battery_voltage = data.get("batteryVoltage")
        THINGSBOARD_TOKEN = data.get("THINGSBOARD_TOKEN")
        LAT = data.get("latitude")
        LON = data.get("longitude")
        IP = data.get("deviceIP")
        E_light = data.get("Light")
        E_solar = data.get("Solar")
        E_battery = data.get("Battery")
        Epem = data.get("PZEM")
        Esdcard = data.get("SDcard")

        # Update weather
        fetch_weather()

        # Check overload
        overload()

        # Build payload once and store globally
        payload = {
            k: v
            for k, v in {
                "overload_status": overload_status,
                "InverterLoad": inverter_load,
                "Frequency": frequency,
                "PowerFactor": power_factor,
                "Voltage": voltage,
                "Current": current,
                "Power": power,
                "Energy": energy,
                "SolarVoltage": solar_voltage,
                "SolarCurrent": solar_current,
                "SolarPower": solar_power,
                "BatteryPercentage": battery_percentage,
                "LightIntensity": light_intensity,
                "BatteryVoltage": battery_voltage,
                "Temperature": temperature,
                "CloudPercent": cloudcover,
                "WindSpeed": windspeed,
                "RainInMM": precipitation,
                "deviceIP": IP,
                "latitude": LAT,
                "longitude": LON,
                "ErrorLight": E_light,
                "ErrorSolar": E_solar,
                "ErrorBattery": E_battery,
                "ErrorPzem": Epem,
                "ErrorSDcard": Esdcard,
            }.items()
            if v is not None
        }

        # Send to ThingsBoard
        THINGSBOARD_URL = (
            f"http://demo.thingsboard.io/api/v1/{THINGSBOARD_TOKEN}/telemetry"
        )
        response = requests.post(THINGSBOARD_URL, json=payload, timeout=5)

        return (
            jsonify(
                {
                    "status": "success",
                    "message": "ESP32 data received",
                    "thingsboard_status": response.status_code,
                    "payload_sent": payload,
                }
            ),
            200,
        )

    except Exception as e:
        print("Error:", str(e))
        return jsonify({"status": "error", "message": str(e)}), 400


# ===================== HOME PAGE =====================
@app.route("/")
def home():
    global payload
    return jsonify(payload)  # always return latest stored payload


# ===================== MAIN APP =====================
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
