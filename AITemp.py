import serial
import time

# settings
serial_port = "COM5"      # adjust if needed
baud_rate = 9600
update_delay = 2.5

mock_temps = [70, 72, 75, 78, 81, 84, 87, 90, 87, 84, 78]
mock_index = 0
history = []

# connect to arduino
try:
    arduino = serial.Serial(serial_port, baud_rate, timeout=1)
    time.sleep(2)
    print("Connected to Arduino on", serial_port)
except Exception as e:
    print("ERROR: Could not connect:", e)
    exit()

# very simple prediction model
def predict_next(history):
    if len(history) < 2:
        return history[-1]

    change = history[-1] - history[-2]
    return history[-1] + change * 1.2

# safe write wrapper
def send_to_arduino(msg):
    try:
        arduino.write((msg + "\n").encode())
    except serial.SerialException:
        print("Warning: write failed, retrying...")
        time.sleep(0.3)

# main loop
while True:

    send_to_arduino("REQ_MODE")
    try:
        mode_data = arduino.readline().decode().strip()
    except serial.SerialException:
        print("Serial connection lost.")
        break

    if not mode_data.startswith("MODE:"):
        print("Waiting for mode...")
        time.sleep(update_delay)
        continue

    try:
        mode = int(mode_data.split(":")[1])
    except ValueError:
        print("Bad mode data:", mode_data)
        time.sleep(update_delay)
        continue

    if mode not in (3, 4):
        print("AI not active (Mode =", mode, ")")
        time.sleep(update_delay)
        continue

    # read temperature
    if mode == 3:
        send_to_arduino("REQ_TEMP")
        try:
            temp_line = arduino.readline().decode().strip()
        except serial.SerialException:
            print("Lost connection reading temperature.")
            break

        try:
            tempF = float(temp_line)
        except:
            print("Invalid sensor data:", temp_line)
            time.sleep(update_delay)
            continue

    else:  # AI + mock
        tempF = mock_temps[mock_index]
        mock_index = (mock_index + 1) % len(mock_temps)

    # update history & predict
    history.append(tempF)
    if len(history) > 10:
        history.pop(0)

    predicted = predict_next(history)

    # choose fan command
    if predicted < 75:
        command = "OFF"
    elif predicted <= 85:
        command = "LOW"
    else:
        command = "HIGH"

    send_to_arduino(command)

    # output
    print("-----------------------------")
    print("Actual Temp:     ", tempF)
    print("Predicted Temp:  ", round(predicted, 2))
    print("Fan Command:     ", command)
    print("Mode:            ", mode)
    print("-----------------------------\n")

    time.sleep(update_delay)
