import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_absolute_error, r2_score
from sklearn.preprocessing import StandardScaler
import joblib
from datetime import datetime, timedelta
import sqlite3

class MicrogridPredictor:
    def __init__(self):
        self.energy_model = RandomForestRegressor(n_estimators=100, random_state=42)
        self.maintenance_model = RandomForestRegressor(n_estimators=100, random_state=42)
        self.scaler = StandardScaler()
        
    def prepare_features(self, df):
        """Feature engineering for ML models"""
        df = df.copy()
        
        # Time-based features
        df['hour'] = pd.to_datetime(df['timestamp']).dt.hour
        df['day_of_week'] = pd.to_datetime(df['timestamp']).dt.dayofweek
        df['month'] = pd.to_datetime(df['timestamp']).dt.month
        df['is_weekend'] = (df['day_of_week'] >= 5).astype(int)
        
        # Rolling averages
        df['power_ma_1h'] = df['Power'].rolling(window=4).mean()  # 4 samples = 1 hour
        df['voltage_ma_1h'] = df['Voltage'].rolling(window=4).mean()
        df['temp_diff'] = df['BoxTemperature'] - df['Temperature']  # Internal vs external temp
        
        # Efficiency metrics
        df['power_efficiency'] = df['Power'] / (df['Voltage'] * df['Current'] + 0.001)
        df['solar_efficiency'] = df['SolarPower'] / (df['LightIntensity'] + 1)
        
        # Weather impact features
        df['weather_score'] = (100 - df['CloudPercent']) * (1 - df['RainInMM']/10)
        
        return df.dropna()
    
    def train_energy_forecasting_model(self, data_csv_path):
        """Train model to predict next hour energy generation"""
        df = pd.read_csv(data_csv_path)
        df = self.prepare_features(df)
        
        # Features for energy prediction
        feature_cols = [
            'Voltage', 'Current', 'PowerFactor', 'Frequency', 'BoxTemperature',
            'SolarVoltage', 'SolarCurrent', 'BatteryPercentage', 'LightIntensity',
            'Temperature', 'CloudPercent', 'WindSpeed', 'RainInMM',
            'hour', 'day_of_week', 'month', 'is_weekend',
            'power_ma_1h', 'voltage_ma_1h', 'temp_diff', 'solar_efficiency', 'weather_score'
        ]
        
        X = df[feature_cols]
        y = df['Energy']
        
        # Split data
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
        
        # Scale features
        X_train_scaled = self.scaler.fit_transform(X_train)
        X_test_scaled = self.scaler.transform(X_test)
        
        # Train model
        self.energy_model.fit(X_train_scaled, y_train)
        
        # Evaluate
        y_pred = self.energy_model.predict(X_test_scaled)
        mae = mean_absolute_error(y_test, y_pred)
        r2 = r2_score(y_test, y_pred)
        
        print(f"Energy Forecasting Model Performance:")
        print(f"MAE: {mae:.3f}")
        print(f"R² Score: {r2:.3f}")
        
        # Save model
        joblib.dump(self.energy_model, 'ml_models/energy_forecast_model.pkl')
        joblib.dump(self.scaler, 'ml_models/energy_scaler.pkl')
        
        return mae, r2
    
    def train_maintenance_prediction_model(self, data_csv_path):
        """Train model to predict maintenance needs"""
        df = pd.read_csv(data_csv_path)
        df = self.prepare_features(df)
        
        # Create maintenance target (1 = needs maintenance, 0 = normal)
        df['needs_maintenance'] = (
            (df['power_efficiency'] < 0.8) |  # Low efficiency
            (abs(df['Voltage'] - 230) > 20) |  # Voltage deviation
            (df['BoxTemperature'] > 45) |      # Overheating
            (df['BatteryPercentage'] < 20)     # Low battery
        ).astype(int)
        
        feature_cols = [
            'Voltage', 'Current', 'Power', 'BoxTemperature', 'BatteryPercentage',
            'power_efficiency', 'temp_diff', 'voltage_ma_1h'
        ]
        
        X = df[feature_cols]
        y = df['needs_maintenance']
        
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
        
        self.maintenance_model.fit(X_train, y_train)
        
        # Evaluate
        y_pred = self.maintenance_model.predict(X_test)
        accuracy = (y_pred == y_test).mean()
        
        print(f"Maintenance Prediction Model Accuracy: {accuracy:.3f}")
        
        # Save model
        joblib.dump(self.maintenance_model, 'ml_models/maintenance_model.pkl')
        
        return accuracy
    
    def predict_next_hour_energy(self, current_data):
        """Predict energy for next hour using current sensor + weather data"""
        # Load trained model
        energy_model = joblib.load('ml_models/energy_forecast_model.pkl')
        scaler = joblib.load('ml_models/energy_scaler.pkl')
        
        # Prepare features (same as training)
        features = np.array([
            current_data['Voltage'], current_data['Current'], current_data['PowerFactor'],
            current_data['Frequency'], current_data['BoxTemperature'],
            current_data['SolarVoltage'], current_data['SolarCurrent'],
            current_data['BatteryPercentage'], current_data['LightIntensity'],
            current_data['Temperature'], current_data['CloudPercent'],
            current_data['WindSpeed'], current_data['RainInMM'],
            datetime.now().hour, datetime.now().weekday(), datetime.now().month,
            int(datetime.now().weekday() >= 5),
            # Add calculated features here
        ]).reshape(1, -1)
        
        features_scaled = scaler.transform(features)
        prediction = energy_model.predict(features_scaled)[0]
        
        return prediction
    
    def check_maintenance_needs(self, current_data):
        """Check if system needs maintenance"""
        maintenance_model = joblib.load('ml_models/maintenance_model.pkl')
        
        # Calculate efficiency and other maintenance indicators
        power_efficiency = current_data['Power'] / (current_data['Voltage'] * current_data['Current'] + 0.001)
        
        features = np.array([
            current_data['Voltage'], current_data['Current'], current_data['Power'],
            current_data['BoxTemperature'], current_data['BatteryPercentage'],
            power_efficiency, 
            current_data['BoxTemperature'] - current_data['Temperature'],  # temp_diff
            current_data['Voltage']  # voltage_ma_1h (simplified)
        ]).reshape(1, -1)
        
        maintenance_prob = maintenance_model.predict_proba(features)[0][1]
        needs_maintenance = maintenance_prob > 0.7
        
        alerts = []
        if needs_maintenance:
            if power_efficiency < 0.8:
                alerts.append("Low system efficiency detected")
            if abs(current_data['Voltage'] - 230) > 20:
                alerts.append("Voltage deviation beyond safe limits")
            if current_data['BoxTemperature'] > 45:
                alerts.append("System overheating")
            if current_data['BatteryPercentage'] < 20:
                alerts.append("Battery level critically low")
        
        return {
            'needs_maintenance': needs_maintenance,
            'maintenance_probability': maintenance_prob,
            'alerts': alerts
        }

# Usage example
if __name__ == "__main__":
    predictor = MicrogridPredictor()
    
    # Train models (run once with historical data)
    # predictor.train_energy_forecasting_model('data/training_data.csv')
    # predictor.train_maintenance_prediction_model('data/training_data.csv')
    
    # Example prediction with current data
    current_data = {
        'Voltage': 230.5, 'Current': 2.1, 'Power': 480.5,
        'PowerFactor': 0.95, 'Frequency': 50.1, 'BoxTemperature': 32.5,
        'SolarVoltage': 12.5, 'SolarCurrent': 1500, 'BatteryPercentage': 85.3,
        'LightIntensity': 45000, 'Temperature': 28.5, 'CloudPercent': 20,
        'WindSpeed': 5.2, 'RainInMM': 0
    }
    
    # Get predictions
    next_hour_energy = predictor.predict_next_hour_energy(current_data)
    maintenance_check = predictor.check_maintenance_needs(current_data)
    
    print(f"Predicted next hour energy: {next_hour_energy:.2f} kWh")
    print(f"Maintenance needed: {maintenance_check['needs_maintenance']}")
    print(f"Alerts: {maintenance_check['alerts']}")
