from flask import Flask, request, jsonify
import requests

app = Flask(__name__)

# ===================== ESP32 + WEATHER STORAGE =====================
THINGSBOARD_TOKEN = None
box_temp = frequency = power_factor = voltage = current = power = energy = None
solar_voltage = solar_current = solar_power = battery_percentage = light_intensity = None
battery_voltage = None
temperature = cloudcover = windspeed = precipitation = None
LAT = LON = IP = None
payload = {}

# ===================== WEATHER DATA =====================
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
        temperature = cloudcover = windspeed = precipitation = None


# ===================== ESP32 DATA =====================
@app.route("/esp32-data", methods=["POST"])
def receive_data():
    global payload, LAT, LON, THINGSBOARD_TOKEN, IP
    global box_temp, frequency, power_factor, voltage, current, power, energy
    global solar_voltage, solar_current, solar_power, battery_percentage
    global light_intensity, battery_voltage

    try:
        data = request.get_json()

        # Update globals
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
        THINGSBOARD_TOKEN = data.get("THINGSBOARD_TOKEN")
        LAT = data.get("latitude")
        LON = data.get("longitude")
        IP = data.get("deviceIP")

        # Update weather
        fetch_weather()

        # Build payload once and store globally
        payload = {
            k: v
            for k, v in {
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
            }.items()
            if v is not None
        }

        # Send to ThingsBoard
        THINGSBOARD_URL = f"http://demo.thingsboard.io/api/v1/{THINGSBOARD_TOKEN}/telemetry"
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
