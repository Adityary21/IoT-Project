import streamlit as st
import mysql.connector
import pandas as pd
from PIL import Image
import os
from datetime import datetime

conn = mysql.connector.connect(
    host='localhost',
    user='root',
    password='',
    database='belajar'
)
cursor = conn.cursor(dictionary=True)

def load_data():
    cursor.execute("SELECT * FROM face_recognition WHERE DATE(timestamp) = CURDATE()")
    records = cursor.fetchall()
    return pd.DataFrame(records)

siswa_list = ['Aditya', 'Kemal']

data = load_data()

st.write("Data dari tabel face_recognition:")
st.write(data)

st.write("Kehadiran siswa hari ini:")

attendance_table = pd.DataFrame(columns=["Nama Siswa", "Kehadiran", "Gambar"])

for siswa in siswa_list:
    siswa_data = data[data['name'].str.lower() == siswa.lower()]
    
    if not siswa_data.empty:
        new_row = pd.DataFrame({
            "Nama Siswa": [siswa],
            "Kehadiran": ["✔️"],
            "Gambar": [siswa_data.iloc[-1]['image_path']]
        })
    else:
        new_row = pd.DataFrame({
            "Nama Siswa": [siswa],
            "Kehadiran": ["❌"],
            "Gambar": [None]
        })
    
    attendance_table = pd.concat([attendance_table, new_row], ignore_index=True)

num_cols = 3
for i in range(0, len(attendance_table), num_cols):
    cols = st.columns(num_cols)
    for j, col in enumerate(cols):
        if i + j < len(attendance_table):
            row = attendance_table.iloc[i + j]
            col.write(f"Nama: {row['Nama Siswa']}")
            col.write(f"Kehadiran: {row['Kehadiran']}")
            if row['Gambar']:
                image_path = row['Gambar']
                if os.path.exists(image_path):
                    image = Image.open(image_path)
                    image = image.resize((150, 150))
                    col.image(image, caption=f"{row['Nama Siswa']} ({row['Kehadiran']})", use_column_width=True)
            else:
                col.write("Tidak ada gambar tersedia.")

cursor.close()
conn.close()
