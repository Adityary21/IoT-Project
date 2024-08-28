import streamlit as st
import mysql.connector
import pandas as pd
from PIL import Image
import os

# Koneksi ke MySQL database `belajar`
conn = mysql.connector.connect(
    host='localhost',
    user='root',  # Sesuaikan dengan username MySQL Anda
    password='',  # Kosongkan jika tidak ada password
    database='belajar'  # Nama database yang digunakan
)
cursor = conn.cursor(dictionary=True)

# Fungsi untuk memuat data dari tabel face_recognition
def load_data():
    cursor.execute("SELECT * FROM face_recognition")
    records = cursor.fetchall()
    return pd.DataFrame(records)

# Muat data dari MySQL
data = load_data()

# Tampilkan tabel data di Streamlit
st.write("Data dari tabel face_recognition:")
st.write(data)

# Tampilkan gambar dengan informasi dalam layout kolom
st.write("Gambar yang disimpan:")

num_cols = 3  # Tentukan jumlah kolom
for i in range(0, len(data), num_cols):
    cols = st.columns(num_cols)
    for j, col in enumerate(cols):
        if i + j < len(data):
            row = data.iloc[i + j]
            image_path = row['image_path']
            if os.path.exists(image_path):
                image = Image.open(image_path)
                image = image.resize((150, 150))  # Resize gambar ke 150x150 pixel
                col.image(image, caption=f"{row['name']} ({row['timestamp']})", use_column_width=True)
                col.write(f"ID: {row['id']}")
                col.write(f"Nama: {row['name']}")
                col.write(f"Timestamp: {row['timestamp']}")
            else:
                col.write(f"Gambar tidak ditemukan di {image_path}")

# Tutup koneksi MySQL
cursor.close()
conn.close()
