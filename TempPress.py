import serial
import csv
import time

# Configure the serial port and parameters
SERIAL_PORT = '/dev/ttyACM0'  # Change to your serial port (e.g., 'COM3', '/dev/ttyUSB0')
BAUD_RATE = 9600
TIMEOUT = 1

# Open the serial port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)

# Open the CSV file for writing
with open('Combined_Temp_Pressure4.csv', mode='w', newline='') as csv_file:
    csv_writer = csv.writer(csv_file)

    # Write the header row if needed
    csv_writer.writerow(['Timestamp', 'Temperature (in Celsius)', 'Pressure (in MPa)'])
    print("working")
    try:
        # print("working2")
        # Get the starting time
        # start_time = time.time()
        # t = 0
        while True:
            # elapsed_time = time.time() - start_time
            # print("working 3")

            
            # Read a line from the serial port
            line = ser.readline().decode('utf-8').strip().split(" ")
            # print(line)
            


            if len(line) == 2:
                # Print the data to the console (optional)
                print(line)
                # print(t)
                timestamp = time.strftime('%H:%M:%S')
                # Write the data to the CSV file
                if(len(line) >= 2):
                    T = line[0]
                    # P = 0
                    P = line[len(line) - 1]
                    csv_writer.writerow([timestamp,T, P])

                # Sleep for 500 milliseconds
                # time.sleep(1)
            #     t = t + 0.5
            # if t > 2:
            #     break
    except KeyboardInterrupt:
        print("Program interrupted by user.")

    finally:
        # Close the serial port
        ser.close()