import pandas as pd
import numpy as np
df=pd.read_csv(
    "telemetry_raw.csv",
    dtype={
        "algorithm":"category",
        "input_size":"int64",
        "execution_time_us":"float64",
        "memory_bytes":"float64",
    },
)
print(df.shape)
print(df.dtypes)
print(df.head)
print(df.isna().sum())