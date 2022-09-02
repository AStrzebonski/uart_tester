import serial
import atexit
import time


def exit_handler(ser: serial.serialwin32.Serial):
    ser.close()


def serial_init(rate: int, port: str):
    ser = serial.Serial()
    ser.baudrate = rate
    ser.port = port
    ser.timeout = 10
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.bytesize = serial.EIGHTBITS
    ser.open()
    ser.reset_input_buffer()

    atexit.register(exit_handler, ser=ser)
    return ser


def serial_write(ser: serial.serialwin32.Serial, bytes):
    term = '\n'
    ser.write(bytes)
    ser.write(term.encode())



def read_data(ser: serial.serialwin32.Serial, len: int):
    return ser.read(len)


def compare_data(data1, data2):
    if data1 == data2:
        print("Testcase SUCCESS")
        return True
    else:
        print("Testcase FAIL")
        return False

def change_baudrate_serial_write(ser: serial.serialwin32.Serial, speed):
    msg = "change baudrate to {}".format(speed).encode()
    serial_write(ser, msg)


def main():
    port = 'COM10'
    speed = 115200
    ser = serial_init(speed, port)

    bytes_send = "dupa".encode()
    serial_write(ser, bytes_send)

    bytes_recv = read_data(ser, len(bytes_send))
    if compare_data(bytes_send, bytes_recv) == False:
        return -1

    speed = 230400
    change_baudrate_serial_write(ser, speed)
    ser.close()

    time.sleep(5)

    ser = serial_init(speed, port)

    bytes_send = "banan".encode()
    serial_write(ser, bytes_send)

    bytes_recv = read_data(ser, len(bytes_send))
    if compare_data(bytes_send, bytes_recv) == False:
        return -1


if __name__ == "__main__":
    main()
