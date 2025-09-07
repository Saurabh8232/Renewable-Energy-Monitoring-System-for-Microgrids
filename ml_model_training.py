import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, r2_score
import joblib

# 1. Load data (CSV export from your database or integrated pipeline)
data = pd.read_csv('combined_sensor_weather_data.csv', parse_dates=['timestamp'])

# 2. Feature Engineering: Create useful features from timestamp
data['hour'] = data['timestamp'].dt.hour
data['day_of_week'] = data['timestamp'].dt.dayofweek
data['month'] = data['timestamp'].dt.month

# 3. Select features and target variable
feature_cols = [
    'Voltage', 'Current', 'PowerFactor', 'Frequency', 'BoxTemperature',
    'solarVoltage', 'solarCurrent', 'solarPower', 'batteryPercentage', 'batteryVoltage',
    'lightIntensity', 'Temperature', 'CloudPercent', 'WindSpeed', 'RainInMM',
    'hour', 'day_of_week', 'month'
]
target_col = 'Energy'  # Or your target metric for prediction

X = data[feature_cols]
y = data[target_col]

# 4. Train-test split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 5. Initialize and train model
model = RandomForestRegressor(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

# 6. Evaluate
y_pred = model.predict(X_test)

print(f'Mean Absolute Error: {mean_absolute_error(y_test, y_pred):.3f}')
print(f'R2 Score: {r2_score(y_test, y_pred):.3f}')

# 7. Save model and preprocessor/scaler (if any)
joblib.dump(model, 'energy_forecast_rf_model.pkl')
print("Model saved as 'energy_forecast_rf_model.pkl'")
