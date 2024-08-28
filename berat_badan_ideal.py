import streamlit as st
import pandas as pd
import numpy as np
import joblib
import gspread
from oauth2client.service_account import ServiceAccountCredentials
import json
import requests # type: ignore
import urllib.parse
from gtts import gTTS # type: ignore
import base64
import os 

# Konfigurasi API Gemini
GEMINI_API_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent"
GEMINI_API_KEY = "AIzaSyA8Fci2mbiJiYUSGy31VzBWVgDEKzuhC2Q"
MAX_TOKENS = 200

# Fungsi untuk mengirim permintaan ke API Gemini
def ask_gemini(question):
    headers = {
        "Content-Type": "application/json"
    }
    payload = {
        "contents": [{
            "parts": [{"text": question}]
        }],
        "generationConfig": {
            "maxOutputTokens": MAX_TOKENS
        }
    }
    response = requests.post(f"{GEMINI_API_URL}?key={GEMINI_API_KEY}", headers=headers, data=json.dumps(payload))
    if response.status_code == 200:
        return response.json()["candidates"][0]["content"]["parts"][0]["text"]
    else:
        return f"Error: {response.status_code}, {response.text}"

# Memuat model RandomForest yang telah dilatih
model = joblib.load('random_forest_bmi_model.pkl')

# Fungsi untuk mengkategorikan BMI
def categorize_bmi(bmi):
    if bmi < 13.9:
        return "Sangat Kurus"
    elif 13.9 <= bmi < 23.0:
        return "Kurus"
    elif 23.0 <= bmi < 29.2:
        return "Normal"
    elif 29.2 <= bmi < 35.8:
        return "Gendut"
    elif 35.8 <= bmi < 51.7:
        return "Obisitas"
    else:
        return "Sangat Obisitas"

# Fungsi untuk melakukan prediksi BMI
def predict_bmi(height, weight, gender):
    gender_value = 1 if gender == 'Laki-Laki' else 0
    features = np.array([[height, weight, gender_value]])
    prediction = model.predict(features)
    return prediction[0]

# Fungsi untuk memuat data dari Google Sheets
def load_data_from_sheets(sheet_url, sheet_name):
    scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
    creds = ServiceAccountCredentials.from_json_keyfile_name("C:\\Users\\handi\\infra-volt-428716-f6-2b1f4cd5094d.json", scope)
    client = gspread.authorize(creds)
    sheet = client.open_by_url(sheet_url).worksheet(sheet_name)
    data = sheet.get_all_records()
    return pd.DataFrame(data)

# Fungsi untuk menghasilkan audio player untuk Streamlit
def get_audio_player(file_path):
    audio_file = open(file_path, "rb")
    audio_bytes = audio_file.read()
    audio_base64 = base64.b64encode(audio_bytes).decode()
    audio_html = f"""
    <audio controls>
        <source src="data:audio/mp3;base64,{audio_base64}" type="audio/mp3">
        Your browser does not support the audio element.
    </audio>
    """
    return audio_html

# Antarmuka pengguna Streamlit
st.title("Prediksi BMI Menggunakan RandomForest")
if st.button('mulai'):
    # Memuat data dari Google Sheets
    sheet_url = "https://docs.google.com/spreadsheets/d/1GVL65V0eSMytN6GfuxII2daXCrkxtPVmb2KaQBKNeFU/edit?gid=0#gid=0"
    sheet_name = "Sheet1"
    data = load_data_from_sheets(sheet_url, sheet_name)

    # Menampilkan data yang dimuat untuk debugging
    st.write("Data from Google Sheets:")
    st.write(data)

    # Memproses data yang dimuat 
    if not data.empty:
        latest_data = data.iloc[-1]  # Menggunakan baris terakhir dari dataframe
        height = latest_data["Height"]
        weight = latest_data["Weight"]
        
        # Memastikan tinggi dan berat berada dalam rentang yang di inginkan
        if height > 250.0:
            height = 250.0
        elif height < 0.0:
            height = 0.0

        if weight > 300.0:
            weight = 300.0
        elif weight < 0.0:
            weight = 0.0
    else:
        height = 0.0
        weight = 0.0
    
    total_height_cm = 179.0  # Tinggi tetap dari sensor ultrasonik ke tanah dalam cm
    actual_height_cm = total_height_cm - height  # Mengurangi pembacaan dari tinggi total
    actual_height_m = actual_height_cm  / 100 # Konversi ke meter

    # Menampilkan nilai tinggi dan berat untuk debugging
    st.write(height)
    st.write(f"Height from Google Sheets: {height}")
    st.write(f"Weight from Google Sheets: {weight}")

    # Field input untuk pengguna melihat tinggi, berat, dan gender
    tinggi = st.number_input("Masukkan Tinggi Badan (dalam meter):", value=float(height))
    berat = st.number_input("Masukkan Berat Badan (dalam kg):", value=float(weight))
    gender = st.radio("Pilih Jenis Kelamin:", ('Laki-Laki', 'Wanita'))

    bmi_prediction = predict_bmi(tinggi, berat, gender)
    bmi_category = categorize_bmi(bmi_prediction)
    st.write(f"Prediksi BMI Anda adalah: {bmi_prediction:.2f}")
    st.write(f"Kategori BMI Anda adalah: {bmi_category}")
    
    pertanyaan = f"Saya adalah {gender} memiliki Tinggi badan {height}cm, dan Berat badan {berat}, dan AI memprediksi Nilai BMI saya {bmi_prediction:.2f} dengan kategori {bmi_category}, apa yang harus dilakukan? dan apa saran dari anda? jelaskan dengan 1 paragraf secara singkat tidak usah menggunakan poin poin"
    jawaban_gemini = ask_gemini(pertanyaan)
    st.write("*Saran AI:*")
    st.write(jawaban_gemini)

    # Mengonversi jawaban Gemini ke audio
    tts = gTTS(text=jawaban_gemini, lang='id')
    audio_file_path = r'F:/Users/Windows10/Downloads/response.mp3'
    tts.save(audio_file_path)

    # Menampilkan audio player di Streamlit
    st.audio(audio_file_path)

    # Membersihkan file audio setelah diputar
    os.remove(audio_file_path)
