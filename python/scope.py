import sys
import serial

# COM17 <qus>

ser = None
line = None

def eat():
	global ser, lastline
	eaten = ''
	while True:
		bytes = ser.readline()
		if bytes == b'':
			break
		line = bytes.decode('utf-8').strip()
		print(line)
		if (line != 'done'):
			lastline = line

def connect():
	global ser, lastline
	import serial
	ser = serial.Serial(sys.argv[1], timeout=.1)
	ser.write(b'\x03'); eat()
	ser.write(b'echo off\r'); eat()
	ser.write(b'prompt off\r'); eat()

connect()

line = 'scope ' + sys.argv[2] + ' ~512\r'
print(line)
bytes = line.encode('utf-8')
ser.write(bytes)
ser.timeout = None

while True:
	bytes = ser.readline()
	line = bytes.decode('utf-8').strip()
	if line == 'done':
		break
	print(line)
