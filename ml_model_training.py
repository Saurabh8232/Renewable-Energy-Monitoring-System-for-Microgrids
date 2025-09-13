

# =========================
# 1) Imports & config
# =========================
import pandas as pd
import numpy as np
import lightgbm as lgb
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score

RANDOM_STATE = 42

# 2) Load CSV
# =========================
csv_path = 'your_data.csv'  
df = pd.read_csv(csv_path)

# Ensure expected columns exist
assert 'timestamp' in df.columns, "timestamp column missing"
assert 'Energy' in df.columns, "Energy column missing"

# 3) Basic cleaning
# =========================
def basic_clean(df):
    df = df.copy()
    df['timestamp'] = pd.to_datetime(df['timestamp'], errors='coerce')
    df = df.dropna(subset=['timestamp'])
    df = df.sort_values('timestamp')
    df = df.drop_duplicates(subset=['timestamp'])
    # Coerce non-time columns to numeric
    for c in df.columns:
        if c != 'timestamp':
            df[c] = pd.to_numeric(df[c], errors='coerce')
    return df

df = basic_clean(df)

# 4) Infer frequency and resample
# =========================
def infer_and_resample(df, target_col='Energy'):
    df = df.set_index('timestamp').sort_index()
    deltas = df.index.to_series().diff().dropna()
    median_delta = deltas.median()
    if pd.isna(median_delta):
        median_delta = pd.Timedelta(minutes=15)
    full_index = pd.date_range(df.index.min(), df.index.max(), freq=median_delta)
    df = df.reindex(full_index)

    # Interpolate exogenous features; conservative ffill for Energy
    for c in df.columns:
        if c != target_col:
            df[c] = df[c].interpolate(limit=4, limit_direction='both')
    df[target_col] = df[target_col].fillna(method='ffill', limit=2)

    df = df.rename_axis('timestamp').reset_index()
    return df, median_delta

df, freq_delta = infer_and_resample(df, target_col='Energy')

# 5) Feature engineering
# =========================
def add_time_features(df):
    d = pd.to_datetime(df['timestamp'])
    df['hour'] = d.dt.hour
    df['dayofweek'] = d.dt.dayofweek
    df['month'] = d.dt.month
    df['is_weekend'] = (df['dayofweek'] >= 5).astype(int)
    return df

def add_lag_features(df, target_col='Energy', lags=None, roll_windows=None):
    df = df.copy()
    if lags is None:
        lags = [1, 2, 3, 4, 6, 12, 24, 48, 96]
    if roll_windows is None:
        roll_windows = [4, 12, 24, 48]
    for lag in lags:
        df[f'{target_col}_lag_{lag}'] = df[target_col].shift(lag)
    for w in roll_windows:
        df[f'{target_col}_roll_mean_{w}'] = df[target_col].shift(1).rolling(w, min_periods=max(2, w//2)).mean()
        df[f'{target_col}_roll_std_{w}']  = df[target_col].shift(1).rolling(w, min_periods=max(2, w//2)).std()
        df[f'{target_col}_roll_min_{w}']  = df[target_col].shift(1).rolling(w, min_periods=max(2, w//2)).min()
        df[f'{target_col}_roll_max_{w}']  = df[target_col].shift(1).rolling(w, min_periods=max(2, w//2)).max()
    return df

df = add_time_features(df)
df = add_lag_features(df, target_col='Energy')

# =========================
# 6) Build horizon targets (+1 hour, +1 day)
# =========================
def build_horizon_targets(df, freq_delta, target_col='Energy'):
    df = df.copy()
    step_minutes = int(freq_delta.total_seconds() // 60)
    if step_minutes == 0:
        step_minutes = 15
    steps_per_hour = max(1, int(round(60 / step_minutes)))
    steps_per_day = steps_per_hour * 24
    df['Energy_t_plus_1h'] = df[target_col].shift(-steps_per_hour)
    df['Energy_t_plus_1d'] = df[target_col].shift(-steps_per_day)
    return df, steps_per_hour, steps_per_day

df, steps_per_hour, steps_per_day = build_horizon_targets(df, freq_delta, target_col='Energy')

# Drop NaNs from engineered features and targets
df_model = df.dropna().copy()

# 7) Time-based split
# =========================
def time_split(df, test_ratio=0.15, val_ratio=0.15):
    n = len(df)
    test_start = int(n * (1 - test_ratio))
    val_start = int(n * (1 - test_ratio - val_ratio))
    train = df.iloc[:val_start].copy()
    val = df.iloc[val_start:test_start].copy()
    test = df.iloc[test_start:].copy()
    return train, val, test

train_df, val_df, test_df = time_split(df_model, test_ratio=0.15, val_ratio=0.15)

# =========================
# 8) Prepare X, y
# =========================
drop_cols_common = {
    'timestamp',
    'Energy',
    'Energy_t_plus_1h',
    'Energy_t_plus_1d'
}

def make_xy(df, target):
    features = [c for c in df.columns if c not in drop_cols_common]
    X = df[features]
    y = df[target]
    return X, y, features

Xtr_1h, ytr_1h, feats_1h = make_xy(train_df, 'Energy_t_plus_1h')
Xva_1h, yva_1h, _ = make_xy(val_df, 'Energy_t_plus_1h')
Xte_1h, yte_1h, _ = make_xy(test_df, 'Energy_t_plus_1h')

Xtr_1d, ytr_1d, feats_1d = make_xy(train_df, 'Energy_t_plus_1d')
Xva_1d, yva_1d, _ = make_xy(val_df, 'Energy_t_plus_1d')
Xte_1d, yte_1d, _ = make_xy(test_df, 'Energy_t_plus_1d')

# =========================
# 9) Train LightGBM with early stopping
# =========================
def train_lgbm(X_train, y_train, X_val, y_val, params=None):
    if params is None:
        params = {
            'objective': 'regression',
            'metric': ['l1', 'l2'],
            'learning_rate': 0.05,
            'num_leaves': 64,
            'feature_fraction': 0.9,
            'bagging_fraction': 0.8,
            'bagging_freq': 1,
            'min_data_in_leaf': 40,
            'seed': RANDOM_STATE,
            'verbose': -1
        }
    dtrain = lgb.Dataset(X_train, label=y_train, free_raw_data=False)
    dval = lgb.Dataset(X_val, label=y_val, reference=dtrain, free_raw_data=False)
    model = lgb.train(
        params,
        dtrain,
        valid_sets=[dtrain, dval],
        valid_names=['train', 'val'],
        num_boost_round=5000,
        early_stopping_rounds=200,
        verbose_eval=200
    )
    return model

model_1h = train_lgbm(Xtr_1h, ytr_1h, Xva_1h, yva_1h)
model_1d = train_lgbm(Xtr_1d, ytr_1d, Xva_1d, yva_1d)

# =========================
# 10) Evaluate
# =========================
def evaluate(y_true, y_pred, tag=''):
    mae = mean_absolute_error(y_true, y_pred)
    rmse = mean_squared_error(y_true, y_pred, squared=False)
    r2 = r2_score(y_true, y_pred)
    print(f'{tag} MAE:  {mae:.6f}')
    print(f'{tag} RMSE: {rmse:.6f}')
    print(f'{tag} R2:   {r2:.6f}')
    return {'MAE': mae, 'RMSE': rmse, 'R2': r2}

pred_va_1h = model_1h.predict(Xva_1h, num_iteration=model_1h.best_iteration)
pred_te_1h = model_1h.predict(Xte_1h, num_iteration=model_1h.best_iteration)

pred_va_1d = model_1d.predict(Xva_1d, num_iteration=model_1d.best_iteration)
pred_te_1d = model_1d.predict(Xte_1d, num_iteration=model_1d.best_iteration)

print('\\nValidation (+1h):')
metrics_1h_val = evaluate(yva_1h, pred_va_1h, tag='Val +1h')

print('\\nTest (+1h):')
metrics_1h_test = evaluate(yte_1h, pred_te_1h, tag='Test +1h')

print('\\nValidation (+1d):')
metrics_1d_val = evaluate(yva_1d, pred_va_1d, tag='Val +1d')

print('\\nTest (+1d):')
metrics_1d_test = evaluate(yte_1d, pred_te_1d, tag='Test +1d')

# =========================
# 11) Inference for next timestamps
# =========================
# To predict the next 1 hour and next 1 day from the most recent row that has all features:
def get_latest_feature_row(df_full, features):
    last_valid = df_full.dropna().iloc[-1]
    return last_valid[features].to_frame().T

latest_1h_X = get_latest_feature_row(df_model, feats_1h)
latest_1d_X = get_latest_feature_row(df_model, feats_1d)

next_1h_energy = model_1h.predict(latest_1h_X, num_iteration=model_1h.best_iteration)
next_1d_energy = model_1d.predict(latest_1d_X, num_iteration=model_1d.best_iteration)

print('\\nForecasts from last available time:')
print('Next 1 hour Energy:', next_1h_energy)
print('Next 1 day Energy :', next_1d_energy)

# =========================
# 12) (Optional) Save models
# =========================
# model_1h.save_model('lgbm_energy_plus1h.txt')
# model_1d.save_model('lgbm_energy_plus1d.txt')
