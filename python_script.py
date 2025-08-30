from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/esp32-data', methods=['POST'])
def receive_data():
    data = request.get_json()
    print("📩 Data received from ESP32:", data)
    return jsonify({"status": "success", "data": data}), 200

@app.route('/')
def home():
    return "https://renewable-energy-monitoring-system-for.onrender.com/esp32-data "

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
