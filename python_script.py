from flask import Flask, request, jsonify
from apscheduler.schedulers.background import BackgroundScheduler
import requests

app = Flask(__name__)

# ===================== ESP32 DATA STORAGE =====================
box_temp, frequency, power_factor, voltage, current, power, energy, battery_voltage = (
    None,
    None,
    None,
    None,
    None,
    None,
    None,
    None,
)
solar_voltage, solar_current, solar_power, battery_percentage, light_intensity = (
    None,
    None,
    None,
    None,
    None,
)

# ThingsBoard Access Token (CHANGE THIS)
THINGSBOARD_TOKEN = "rozHpF7JqlEw6XnPluAF"
THINGSBOARD_URL = f"http://demo.thingsboard.io/api/v1/{THINGSBOARD_TOKEN}/telemetry"


@app.route("/esp32-data", methods=["POST"])
def receive_data():
    global box_temp, frequency, power_factor, voltage, current, power, energy
    global solar_voltage, solar_current, solar_power, battery_percentage
    global light_intensity, battery_voltage

    try:
        data = request.get_json()

        # Update global ESP32 values
        box_temp = data.get("BoxTemperature")
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

        return jsonify({"status": "success", "message": "ESP32 data received"}), 200

    except Exception as e:
        print("Error:", str(e))
        return jsonify({"status": "error", "message": str(e)}), 400


# ===================== WEATHER DATA FETCH =====================
LAT = 28.6139
LON = 77.2090
temperature = None
cloudcover = None
windspeed = None
precipitation = None

def fetch_weather():
    global temperature, cloudcover, windspeed, precipitation
    try:
        url = (
            f"https://api.open-meteo.com/v1/forecast?"
            f"latitude={LAT}&longitude={LON}&hourly=temperature_2m,cloudcover,windspeed_10m,precipitation"
        )
        response = requests.get(url, timeout=10)
        data = response.json()
        temperature = data["hourly"]["temperature_2m"][0]
        cloudcover = data["hourly"]["cloudcover"][0]
        windspeed = data["hourly"]["windspeed_10m"][0]
        precipitation = data["hourly"]["precipitation"][0]

    except Exception as e:
        print("Error fetching weather:", str(e))


# ===================== THINGSBOARD SENDER =====================
def send_to_thingsboard():
    payload = {k: v for k, v in {
        "BoxTemperature": box_temp,
        "Frequency": frequency,
        "PowerFactor": power_factor,
        "Voltage": voltage,
        "Current": current,
        "Power": power,
        "Energy": energy,
        "SolarVoltage": solar_voltage,
        "SolarCurrent": solar_current,
        "SolarPower": solar_power,
        "batteryPercentage": battery_percentage,
        "lightIntensity": light_intensity,
        "batteryVoltage": battery_voltage,
        "Temperature": temperature,
        "Cloud%": cloudcover,
        "WindSpeed": windspeed,
        "RainInMM": precipitation,
    }.items() if v is not None}

    try:
        response = requests.post(THINGSBOARD_URL, json=payload, timeout=5)
        print("Sent to ThingsBoard:", payload, "Status:", response.status_code)
    except Exception as e:
        print("Error sending to ThingsBoard:", str(e))


# ===================== MAIN APP =====================
if __name__ == "__main__":
    scheduler = BackgroundScheduler()
    scheduler.add_job(fetch_weather, "interval", hours=1)   # Weather update har 1 ghante
    scheduler.add_job(send_to_thingsboard, "interval", seconds=30)  # Data send har 30 sec

    try:
        scheduler.start()
    except Exception as e:
        print("Scheduler already running or failed to start:", str(e))

    fetch_weather()  # Startup me ek baar weather fetch
    app.run(host="0.0.0.0", port=5000, debug=True)

