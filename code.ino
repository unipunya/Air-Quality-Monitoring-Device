import network
import urequests
import json
import time
from machine import ADC, Pin, I2C, UART
import onewire, ds18x20
from lcd_api import LcdApi
from i2c_lcd import I2cLcd
import utime

# I2C settings
I2C_ADDR = 0x27  # The I2C address of your LCD
I2C_NUM_ROWS = 4
I2C_NUM_COLS = 20

# Initialize the I2C interface
i2c = I2C(0, scl=Pin(1), sda=Pin(0), freq=400000)

# Initialize GSM module (assuming you have a SIM800L connected to UART pins)
uart = UART(0, baudrate=9600, tx=Pin(12), rx=Pin(13))  # UART setup

# Initialize the LCD
lcd = I2cLcd(i2c, I2C_ADDR, I2C_NUM_ROWS, I2C_NUM_COLS)

# Display loading message
lcd.clear()
lcd.move_to(0, 0)
lcd.putstr("Loading, please wait...")

# Define pins for the sensors
PRE_PIN = 22
VNOX_PIN = 26  # Assuming VNOX is connected to GPIO 26 (ADC0)
VRED_PIN = 27  # Assuming VRED is connected to GPIO 27 (ADC1)
dust_pin = Pin(16, Pin.IN)  # Pin dust definition
ds_pin = Pin(5)  # Define the GPIO pin where the sensor is connected

led_onboard = machine.Pin("LED", machine.Pin.OUT)

num_samples = 10  # Number of samples to average for stability

# Define preheat time in seconds
PRE_HEAT_SECONDS = 10

# Initialize the preheater pin
pre_pin = Pin(PRE_PIN, Pin.OUT)

# Initialize ADC pins
vnox_pin = ADC(Pin(VNOX_PIN))
vred_pin = ADC(Pin(VRED_PIN))

# Reference voltage of the ADC
VREF = 3.3

# Sensitivity values from the datasheet (example values, replace with actual values)
SENSITIVITY_NO2 = 50  # mV/ppm (example)
SENSITIVITY_CO = 0.15  # mV/ppm (example)

# Preheating process
pre_pin.value(1)
time.sleep(PRE_HEAT_SECONDS)
pre_pin.value(0)

# Set up the one-wire bus
ds_sensor = ds18x20.DS18X20(onewire.OneWire(ds_pin))

# Scan for devices on the bus
roms = ds_sensor.scan()

# Timing variables
sample_time_ms = 5000  # Sample time in milliseconds (5 seconds)
low_pulse_occupancy = 0
start_time = time.ticks_ms()

# Global variable for temperature
current_temp = 0.0

# Constants
VOLTAGE_REF = 3.3  # Reference voltage of the ADC (in volts)
ADC_RESOLUTION = 65535  # 16-bit ADC resolution
OXYGEN_SENSOR_PIN = 28  # ADC pin where the sensor is connected

# Function to calculate the sensor resistance
def calculate_resistance(adc_value, RLOAD):
    Vadc = adc_value * (3.3 / 65535)  # Convert ADC value to voltage
    if Vadc == 0:
        return float('inf')  # Avoid division by zero
    Rs = (3.3 - Vadc) * RLOAD / Vadc  # Calculate sensor resistance
    return Rs

# Function to average multiple ADC readings
def average_adc_readings(adc_pin, num_samples=10):
    total = 0
    for _ in range(num_samples):
        total += adc_pin.read_u16()
        time.sleep(0.1)  # small delay between readings
    return total // num_samples

# Initial calibration to determine R0 (baseline resistance in clean air)
def calibrate_sensor(sensor_pin, RLOAD, num_samples=50):
    total_Rs = 0
    for _ in range(num_samples):
        adc_value = average_adc_readings(sensor_pin)
        Rs = calculate_resistance(adc_value, RLOAD)
        total_Rs += Rs
    return total_Rs / num_samples

# Measure gas concentration based on calibrated R0 and sensitivity factors
def measure_gas_concentration(Rs, R0, factor):
    if R0 == 0:  # Avoid division by zero
        return float('inf')
    concentration = (Rs / R0) / factor
    return concentration

# Allow time for initialization
time.sleep(5)

def is_valid_reading(value, min_value=0, max_value=65535):
    return min_value <= value <= max_value

# Function to measure dust concentration
def pulse_in(pin, value, timeout=1000000):
    start = time.ticks_us()
    while pin.value() != value:
        if time.ticks_diff(time.ticks_us(), start) > timeout:
            return 0
    start = time.ticks_us()
    while pin.value() == value:
        if time.ticks_diff(time.ticks_us(), start) > timeout:
            return 0
    duration = time.ticks_diff(time.ticks_us(), start)
    return duration

# Example calibration coefficients (replace these with actual values from your calibration)
a = 0.0001
b = -0.003
c = 0.5
d = 1.0

def run_temp():
    global current_temp
    # Request temperature readings
    ds_sensor.convert_temp()
    
    for rom in roms:
        temp = ds_sensor.read_temp(rom)
        if temp is not None:
            current_temp = temp
            
            print(f"Temperature read: {temp:.2f}C")  # Debugging statement
            lcd.move_to(10, 0)
            lcd.putstr("T: {:.2f}C".format(temp))
        else:
            print("Error: Could not read temperature data")

# Function to send SMS
def send_sms(message):
    uart.write('AT+CMGS="+94740683564"\r\n')  # Replace with your phone number
    time.sleep(1)
    uart.write(message + "\x1A")  # Send message and terminate with Ctrl+Z
    time.sleep(5)
    response = uart.read()
    print(response)
    if response and b'ERROR' in response:
        print("Failed to send SMS")
    else:
        print("SMS sent successfully")

def init_gsm():
    uart.write("AT\r\n")
    time.sleep(1)
    response = uart.read()
    if response and b"OK" in response:
        print("GSM Module is responding.")
    else:
        print("GSM Module not responding. Check connections.")

    uart.write("AT+CMGF=1\r\n")  # Set SMS mode to text
    time.sleep(1)
    response = uart.read()
    if response and b"OK" in response:
        print("SMS mode set to text.")
    else:
        print("Failed to set SMS mode.")

# Wi-Fi connection setup
def connect_wifi(ssid, password):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)

    while not wlan.isconnected():
        print('Connecting to network...')
        time.sleep(1)

    print('Connected to WiFi')
    print('Network config:', wlan.ifconfig())

def is_wifi_connected():
    wlan = network.WLAN(network.STA_IF)
    return wlan.isconnected()

# Replace with your Wi-Fi credentials
wifi_ssid = 'Darsha'
wifi_password = '12345678'

# Firebase interaction setup
firebase_url = 'https://air-quality-monitoring-5ad17-default-rtdb.asia-southeast1.firebasedatabase.app'
firebase_auth_token = '8fZsO6HR1WFK7v5yOUCAzA6DuRPXwYnGHklo15lc'

def send_data_to_firebase(data):
    try:
        # Define the paths for real-time and history data
        realtime_path = "Gas value"
        history_path = "History"
        
        # URLs for real-time and history data
        realtime_url = f"{firebase_url}/{realtime_path}.json?auth={firebase_auth_token}"
        history_url = f"{firebase_url}/{history_path}.json?auth={firebase_auth_token}"
        
        headers = {'Content-Type': 'application/json'}
        
        # Send real-time data
        print(f"Sending data to {realtime_url} with payload {data}")
        response = urequests.put(realtime_url, data=json.dumps(data), headers=headers)
        if response.status_code != 200:
            print(f"Error {response.status_code}: {response.text}")
        else:
            print("Data sent successfully to Firebase.")
        response.close()
        
        # Send historical data (without timestamp in the path)
        print(f"Sending history data to {history_url} with payload {data}")
        history_response = urequests.post(history_url, data=json.dumps(data), headers=headers)
        if history_response.status_code != 200:
            print(f"Error {history_response.status_code}: {history_response.text}")
        else:
            print("History data sent successfully to Firebase.")
        history_response.close()
        
    except Exception as e:
        print("Exception during data send to Firebase:", e)

def get_data_from_firebase():
    try:
        path = "Gas value"
        url = f"{firebase_url}/{path}.json?auth={firebase_auth_token}"
        print(f"Retrieving data from {url}")
        response = urequests.get(url)
        if response.status_code != 200:
            print(f"Error {response.status_code}: {response.text}")
        else:
            data = response.json()
            print(data)
        response.close()
        return data
    except Exception as e:
        print("Exception during data retrieval from Firebase:", e)
        return None

# Example of sending data to Firebase
data = {
    "value": 55,
    "time": "2024-07-21T12:02:00Z"
}
send_data_to_firebase(data)

# Example of retrieving historical data from Firebase
historical_data = get_data_from_firebase()
print("Historical Data:", historical_data)

# Function to read ADC value and convert it to voltage
def read_adc_voltage(adc_pin):
    adc = machine.ADC(adc_pin)
    adc_value = adc.read_u16()
    voltage = (adc_value / ADC_RESOLUTION) * VOLTAGE_REF
    return voltage

# Function to convert voltage to oxygen level (in percentage)
def voltage_to_oxygen_level(voltage):
    # Conversion logic based on the sensor's datasheet
    # This is a hypothetical example, please refer to your sensor's datasheet for accurate calculations
    # Assume the sensor output is linear between 0V (0% oxygen) to 2.5V (100% oxygen)
    oxygen_level = (voltage / 3.5) * 100
    return oxygen_level

# Initialize GSM
init_gsm()

# Connect to Wi-Fi
connect_wifi(wifi_ssid, wifi_password)

while True:
    try:
        # Display loading message
        lcd.clear()
        lcd.move_to(0, 0)
        lcd.putstr("Loading, please wait...")

        # Read analog values
        vnox_value = vnox_pin.read_u16()
        vred_value = vred_pin.read_u16()

        # Convert ADC values to voltage
        vnox_voltage = vnox_value / 65535 * VREF
        vred_voltage = vred_value / 65535 * VREF

        # Convert voltage to gas concentration
        no2_concentration = vnox_voltage / SENSITIVITY_NO2
        co_concentration = vred_voltage / SENSITIVITY_CO

        # Wait for 1 second
        time.sleep(1)

        # Read oxygen sensor value and calculate concentration
        voltage = read_adc_voltage(OXYGEN_SENSOR_PIN)
        oxygen_level = voltage_to_oxygen_level(voltage)
        oxygen_percentage = oxygen_level / 100

        utime.sleep(1)  # Wait for 1 second before reading again

        # Print the gas concentrations on the LCD
        lcd.clear()
        lcd.move_to(0, 0)
        lcd.putstr(f"CO:{co_concentration:.2f}")
        lcd.move_to(0, 1)
        lcd.putstr(f"NO2:{no2_concentration:.2f}")
        lcd.move_to(0, 2)
        lcd.putstr(f"O2:{oxygen_level:.2f}%")

        # Print the gas concentrations on the console
        print(f"CO concentration: {co_concentration:.2f} ppm")
        print(f"NO2 concentration: {no2_concentration:.2f} ppm")
        print(f"Oxygen concentration: {oxygen_level:.2f} %")

    except Exception as e:
        print("Error reading sensor data:", e)

    # Measure dust concentration
    duration = pulse_in(dust_pin, 0)
    low_pulse_occupancy += duration

    if time.ticks_diff(time.ticks_ms(), start_time) > sample_time_ms:
        ratio = low_pulse_occupancy / (sample_time_ms * 10.0)
        concentration = a * (ratio ** 3) + b * (ratio ** 2) + c * ratio + d

        low_pulse_occupancy = 0
        start_time = time.ticks_ms()

        lcd.move_to(0, 3)
        lcd.putstr(f"Dust:{concentration:.2f}ug/m3")
        print(f"Dust:{concentration:.2f}ug/m3")

        run_temp()

        # Compose the message
        sms_message = f"Temperature: {current_temp:.2f}C, Dust:{concentration:.2f}ug/m3, CO:{co_concentration:.2f}ppm, NO2:{no2_concentration:.2f}ppm, O2:{oxygen_level:.2f}%"
        data_to_send = {
            "Temperature": current_temp,
            "Dust": concentration,
            "CO": co_concentration,
            "NO2": no2_concentration,
            "Oxygen": oxygen_level
        }

        if is_wifi_connected():
            # Send data to Firebase
            send_data_to_firebase(data_to_send)
        else:
            # Send SMS if Wi-Fi is not connected
            send_sms(sms_message)

    # Run temperature reading function
    run_temp()

    # Blink onboard LED
    led_onboard.value(1)
    utime.sleep_ms(1000)
    led_onboard.value(0)
    utime.sleep_ms(1000)

    time.sleep(5)