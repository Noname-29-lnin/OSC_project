import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import threading
import time

# Khởi tạo các biến toàn cục
running = False
serial_port = None
data_x = []
data_y = []

# Hàm lấy danh sách cổng COM
def get_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

# Hàm kết nối cổng COM
def connect_serial():
    global serial_port, running, data_x, data_y
    com_port = com_var.get()
    baudrate = int(baud_var.get())
    
    try:
        serial_port = serial.Serial(com_port, baudrate, timeout=1)
        messagebox.showinfo("Thành Công", f"Kết nối thành công với {com_port}")
        print(f"Đã kết nối với {com_port} - Baudrate: {baudrate}")
        running = True
        data_x.clear()
        data_y.clear()
        threading.Thread(target=read_data, daemon=True).start()
    except Exception as e:
        messagebox.showerror("Lỗi", f"Không thể kết nối: {str(e)}")

# Hàm ngắt kết nối cổng COM
def disconnect_serial():
    global running, serial_port
    running = False
    if serial_port and serial_port.is_open:
        serial_port.close()
        messagebox.showinfo("Ngắt Kết Nối", "Đã ngắt kết nối thành công")
        print("Đã ngắt kết nối với cổng COM")

# Hàm đọc dữ liệu từ STM32
def read_data():
    global running, serial_port, data_x, data_y
    start_time = time.time()
    while running:
        try:
            line = serial_port.readline().decode('utf-8').strip()
            if line:
                print(f"Dữ liệu nhận được: {line}")  # Kiểm tra dữ liệu nhận được
                if line.isdigit():  # Nếu dữ liệu có thể chuyển thành số
                    value = int(line)
                    data_y.append(value)
                    data_x.append(time.time() - start_time)

                    # Cập nhật đồ thị
                    ax.clear()
                    ax.plot(data_x, data_y, color="blue")
                    ax.set_title("Dữ Liệu Từ STM32")
                    ax.set_xlabel("Thời Gian (s)")
                    ax.set_ylabel("Giá Trị")
                    canvas.draw()
        except Exception as e:
            print(f"Lỗi đọc dữ liệu: {e}")
            running = False
            break

# Giao diện Tkinter
root = tk.Tk()
root.title("Ứng Dụng Đọc Dữ Liệu Serial STM32")
root.geometry("1000x600")

# Phần cấu hình kết nối
frame_config = ttk.LabelFrame(root, text="Cấu Hình Kết Nối")
frame_config.pack(pady=10, padx=10, fill="x")

# Chọn cổng COM
ttk.Label(frame_config, text="Cổng COM:").grid(row=0, column=0, padx=5, pady=5)
com_var = tk.StringVar()
com_ports = get_ports()
com_var.set(com_ports[0] if com_ports else "")
com_dropdown = ttk.Combobox(frame_config, textvariable=com_var, values=com_ports)
com_dropdown.grid(row=0, column=1, padx=5, pady=5)

# Chọn baudrate
ttk.Label(frame_config, text="Baudrate:").grid(row=1, column=0, padx=5, pady=5)
baud_var = tk.StringVar(value="9600")
baud_dropdown = ttk.Combobox(frame_config, textvariable=baud_var, values=["9600", "115200", "57600"])
baud_dropdown.grid(row=1, column=1, padx=5, pady=5)

# Nút kết nối / ngắt kết nối
btn_connect = ttk.Button(frame_config, text="Kết Nối", command=connect_serial)
btn_connect.grid(row=2, column=0, padx=5, pady=5)

btn_disconnect = ttk.Button(frame_config, text="Ngắt Kết Nối", command=disconnect_serial)
btn_disconnect.grid(row=2, column=1, padx=5, pady=5)

# Khung hiển thị đồ thị
frame_graph = ttk.LabelFrame(root, text="Đồ Thị Dữ Liệu")
frame_graph.pack(pady=10, padx=10, fill="both", expand=True)

# Thiết lập đồ thị
figure = plt.Figure(figsize=(10, 5), dpi=100)
ax = figure.add_subplot(111)
canvas = FigureCanvasTkAgg(figure, master=frame_graph)
canvas.get_tk_widget().pack(fill="both", expand=True)

# Vòng lặp chính của Tkinter
root.mainloop()
