import serial
import atexit
import time
import random
import string

def get_random_string(length):
    # choose from all letters
    letters = string.ascii_letters
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str

def exit_handler(ser: serial.serialwin32.Serial):
    ser.close()

def serial_init(rate: int, port: str):
    ser = serial.Serial()
    ser.baudrate = rate
    ser.port = port
    ser.timeout = 3
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
    print("wrote: " + str(bytes))
    ser.write(term.encode())

def serial_read(ser: serial.serialwin32.Serial, length):
    ret = ser.read(length)
    print("received: " + str(ret))
    return ret

def compare_data(data1, data2):
    lost_frames = len(data1) - len(data2)
    bad_frames = 0
    for _a, _b in zip(data1, data2):
        if _a != _b:
            bad_frames += 1
    print("lost frames:", lost_frames)
    print("bad frames:", bad_frames)

    if data1 == data2:
        print("Testcase SUCCESS")
        return True
    else:
        print("Testcase FAIL")
        return False

def change_baudrate_serial_write(ser: serial.serialwin32.Serial, speed):
    ACK_MSG = "ACK"

    msg = str(speed).encode()
    serial_write(ser, msg)

    bytes_recv = serial_read(ser, len(ACK_MSG))
    if bytes_recv.decode() != ACK_MSG:
        print("FAIL: unable to change baudrate on device")

def testcase_run(ser: serial.serialwin32.Serial, length):
    ser.reset_input_buffer()
    bytes_send = get_random_string(length).encode()

    serial_write(ser, bytes_send)

    bytes_recv = serial_read(ser, length)

    return compare_data(bytes_send, bytes_recv)


def main():
    # Teststand input:
    params = list(map(str, input().split(',')))
    port = str(params[0])
    print("Port:", port)
    speed = [115200, int(params[1])]
    print("Speed:", speed)
    length = int(params[2])
    print("Length:", length)

    # No teststand input
    # port = 'COM5'
    # speed = [115200, 9600, 57600]
    # length = 32

    for i in range(len(speed)):
        ser = serial_init(speed[i], port)

        if i < len(speed) - 1:
            change_baudrate_serial_write(ser, speed[i + 1])

        testcase_run(ser, length)

        ser.close()

        time.sleep(3)


if __name__ == "__main__":
    main()
