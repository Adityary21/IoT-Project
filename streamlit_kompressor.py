import pandas as pd
import joblib
import plotly.graph_objs as go
import numpy as np
import gspread
from oauth2client.service_account import ServiceAccountCredentials
import time
import streamlit as st
from datetime import datetime
from streamlit_option_menu import option_menu
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import StandardScaler

# Atur Google Sheets API
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
creds = ServiceAccountCredentials.from_json_keyfile_name('monitoring-426117-1a045c8861ad.json', scope)
client = gspread.authorize(creds)

# Memuat model dan scaler yang sudah disimpan
rf_temp = joblib.load('C:\\Users\\ari20\\model\\tempmodel1.joblib')
rf_pressure = joblib.load('C:\\Users\\ari20\\model\\pressuremodel1.joblib')
scaler = joblib.load('C:\\Users\\ari20\\model\\scaler_rf1.joblib')

def prediksi_status(suhu, tekanan, model):
    data_input = pd.DataFrame([[suhu, tekanan]], columns=['Temperature', 'Pressure'])
    data_input_scaled = scaler.transform(data_input)
    status_terprediksi = model.predict(data_input_scaled)
    return status_terprediksi[0]

def bersihkan_data(data):
    data = data.dropna()
    data['Temperature'] = pd.to_numeric(data['Temperature'], errors='coerce')
    data['Pressure'] = pd.to_numeric(data['Pressure'], errors='coerce')
    data['Timestamp'] = pd.to_datetime(data['Timestamp'])
    data = data.drop_duplicates()
    data = data.dropna(subset=['Temperature', 'Pressure'])
    
    # Pastikan hanya kolom numerik yang digunakan untuk perhitungan IQR
    numeric_cols = ['Temperature', 'Pressure']
    Q1 = data[numeric_cols].quantile(0.25)
    Q3 = data[numeric_cols].quantile(0.75)
    IQR = Q3 - Q1
    data = data[~((data[numeric_cols] < (Q1 - 1.5 * IQR)) | (data[numeric_cols] > (Q3 + 1.5 * IQR))).any(axis=1)]
    return data


def proses_spreadsheet(data):
    suhu_list = []
    tekanan_list = []
    temp_status_list = []
    pressure_status_list = []
    time_list = []
    data = bersihkan_data(data)
    if len(data) > 50:
        data = data.iloc[-50:]
    for index, row in data.iterrows():
        suhu = row['Temperature']
        tekanan = row['Pressure']
        time = row['Timestamp']
        # Ubah timestamp agar sesuai dengan waktu sekarang
        time = datetime.now().replace(hour=time.hour, minute=time.minute, second=time.second, microsecond=0)
        temp_status = prediksi_status(suhu, tekanan, rf_temp)
        pressure_status = prediksi_status(suhu, tekanan, rf_pressure)
        suhu_list.append(suhu)
        tekanan_list.append(tekanan)
        temp_status_list.append(temp_status)
        pressure_status_list.append(pressure_status)
        time_list.append(time)
        print(f"Waktu: {time}, Suhu: {suhu}, Tekanan: {tekanan}, TempStatus Terprediksi: {temp_status}, PressureStatus Terprediksi: {pressure_status}")
    return suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list

def plot_grafik(suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list, chart_placeholder):
    chart_placeholder.empty()
    
    with chart_placeholder.container():
        # Plot Suhu
        fig_suhu = go.Figure()
        fig_suhu.add_trace(go.Scatter(
            x=time_list,
            y=suhu_list,
            mode='lines+markers',
            name='Suhu',
            line=dict(color='blue', shape='spline'),
            marker=dict(size=8, color='blue')
        ))

        fig_suhu.update_layout(
            title='Grafik Suhu',
            xaxis_title='Waktu',
            yaxis_title='Nilai Suhu',
            legend=dict(
                title=dict(text='Parameter', font=dict(size=12, color='white')),
                font=dict(
                    size=12,
                    color='white'
                )
            ),
            hovermode='x unified',
            plot_bgcolor='black',
            paper_bgcolor='black',
            xaxis=dict(
                showgrid=True,
                gridcolor='grey',
                tickfont=dict(color='white'),
                titlefont=dict(color='white')
            ),
            yaxis=dict(
                showgrid=True,
                gridcolor='grey',
                tickfont=dict(color='white'),
                titlefont=dict(color='white')
            ),
            font=dict(
                family="Arial, sans-serif",
                size=12,
                color="white"
            )
        )
        
        st.plotly_chart(fig_suhu)

        # Plot Tekanan
        fig_tekanan = go.Figure()
        fig_tekanan.add_trace(go.Scatter(
            x=time_list,
            y=tekanan_list,
            mode='lines+markers',
            name='Tekanan',
            line=dict(color='red', shape='spline'),
            marker=dict(size=8, color='red')
        ))

        fig_tekanan.update_layout(
            title='Grafik Tekanan',
            xaxis_title='Waktu',
            yaxis_title='Nilai Tekanan',
            legend=dict(
                title=dict(text='Parameter', font=dict(size=12, color='white')),
                font=dict(
                    size=12,
                    color='white'
                )
            ),
            hovermode='x unified',
            plot_bgcolor='black',
            paper_bgcolor='black',
            xaxis=dict(
                showgrid=True,
                gridcolor='grey',
                tickfont=dict(color='white'),
                titlefont=dict(color='white')
            ),
            yaxis=dict(
                showgrid=True,
                gridcolor='grey',
                tickfont=dict(color='white'),
                titlefont=dict(color='white')
            ),
            font=dict(
                family="Arial, sans-serif",
                size=12,
                color="white"
            )
        )
        
        st.plotly_chart(fig_tekanan)

        # Peringatan TempStatus rendah atau tinggi hanya untuk data terbaru
        if temp_status_list[-1] == 1:
            st.warning(f"Warning: TempStatus is LOW at {time_list[-1]}")
        elif temp_status_list[-1] == 3:
            st.error(f"Alert: TempStatus is HIGH at {time_list[-1]}")
        else:
            st.success(f"TempStatus is NORMAL at {time_list[-1]}")

        # Peringatan PressureStatus rendah atau tinggi hanya untuk data terbaru
        if pressure_status_list[-1] == 1:
            st.warning(f"Warning: PressureStatus is LOW at {time_list[-1]}")
        elif pressure_status_list[-1] == 3:
            st.error(f"Alert: PressureStatus is HIGH at {time_list[-1]}")
        else:
            st.success(f"PressureStatus is NORMAL at {time_list[-1]}")

def perbarui_visualisasi(sheet, chart_placeholder, last_data):
    # Tentukan header yang diharapkan
    expected_headers = ['Timestamp', 'Temperature', 'Pressure']

    data = pd.DataFrame(sheet.get_all_records(expected_headers=expected_headers))
    if not data.equals(last_data):
        suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list = proses_spreadsheet(data)
        plot_grafik(suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list, chart_placeholder)
    else:
        # Update grafik meskipun tidak ada perubahan data
        suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list = proses_spreadsheet(data)
        plot_grafik(suhu_list, tekanan_list, temp_status_list, pressure_status_list, time_list, chart_placeholder)
    return data

# Fungsi utama untuk aplikasi Streamlit
def main():
    st.set_page_config(page_title="Aplikasi Monitoring Suhu dan Tekanan", layout="wide", initial_sidebar_state="expanded", page_icon="🤖")
    
    # Menggunakan streamlit-option-menu untuk navbar yang lebih interaktif
    with st.sidebar:
        choice = option_menu(
            "Menu",
            ["Home", "Monitoring", "Tentang"],
            icons=["house", "activity", "info-circle", "phone"],
            menu_icon="cast",
            default_index=0,
            styles={
                "container": {"padding": "5px"},
                "icon": {"color": "#FAFAFA", "font-size": "25px"}, 
                "nav-link": {
                    "font-size": "16px", 
                    "text-align": "left", 
                    "margin": "0px", 
                    "--hover-color": "#575757",
                },
                "nav-link-selected": {"background-color": "#02ab21"},
            }
        )
    
    if choice == "Home":
        st.write("<h1 style='text-align: center; color: white;'>Aplikasi Monitoring Suhu dan Tekanan</h1>", unsafe_allow_html=True)
        st.write("<div style='text-align: center; color: white;'>Aplikasi Ini Memungkinkan Anda Untuk memonitoring temperature dan pressure secara real-time.</div>", unsafe_allow_html=True)
        st.markdown("<div style='text-align: center; color: white;'>Status yang ditampilkan: 1 untuk low, 2 untuk normal, dan 3 untuk high.</div>", unsafe_allow_html=True)
        st.image("https://www.saventech.com/wp-content/uploads/2021/08/blog-4-1.jpg", use_column_width=True)
    
    elif choice == "Monitoring":
        st.write("<h1 style='text-align: center; color: white;'>Monitoring Suhu dan Tekanan</h1>", unsafe_allow_html=True)
        sheet_url = 'https://docs.google.com/spreadsheets/d/19CgmT8a92Xq8VjAS6XFNR7jcEha0NCxit14V6wp0PE4/edit#gid=0'
        sheet = client.open_by_url(sheet_url).sheet1
        chart_placeholder = st.empty()
        last_data = pd.DataFrame()

        # Placeholder untuk memulai/menghentikan pembaruan otomatis
        auto_update = st.checkbox('Mulai Pembaruan Otomatis', value=True)
        
        while auto_update:
            with chart_placeholder.container():
                last_data = perbarui_visualisasi(sheet, chart_placeholder, last_data)
            time.sleep(15)  # Check for updates every 15 seconds
        
        st.write("Pembaruan otomatis dihentikan.")
    
    elif choice == "Tentang":
        st.write("<h1 style='text-align: center; color: white;'>Aplikasi Monitoring Suhu dan Tekanan</h1>", unsafe_allow_html=True)
        st.write("""
        ### Tentang Aplikasi Monitoring
        Aplikasi ini dirancang untuk memonitor suhu dan tekanan secara real-time menggunakan data yang diambil dari sensor dan disimpan di Google Sheets.
        
        Fitur Utama:
        - Monitoring suhu dan tekanan secara real-time.
        - Visualisasi data dalam bentuk grafik yang mudah dipahami.
        - Pemberitahuan otomatis jika ada kondisi abnormal pada suhu atau tekanan.
        
        Teknologi yang Digunakan:
        - Google Sheets API untuk penyimpanan data.
        - Model Machine Learning untuk prediksi status suhu dan tekanan.
        - Streamlit untuk tampilan antarmuka pengguna yang interaktif.
        """)

if __name__ == "__main__":
    main()