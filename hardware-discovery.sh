#!/bin/sh

if [ "$1" = "UART-loopback" ]; then
	echo '{'
	echo ' "reconfigure": "hardware-reconfigure.sh",'
	echo ' "hwconfs": ['
	echo '  {'
	echo '   "uid": "ttyUSB0-ttyUSB0-01",'
	echo '   "tx": "/dev/ttyUSB0",'
	echo '   "rx": "/dev/ttyUSB0",'
	echo '   "hwflow": 0,'
	echo '   "baud_rates": ['
	echo '    4800,'
	echo '    9600,'
	echo '    19200'
	echo '   ]'
	echo '  },'
	echo '  {'
	echo '   "uid": "ttyUSB0-ttyUSB0-02",'
	echo '   "tx": "/dev/ttyUSB0",'
	echo '   "rx": "/dev/ttyUSB0",'
	echo '   "hwflow": 1,'
	echo '   "baud_rates": ['
	echo '    4800,'
	echo '    9600,'
	echo '    19200'
	echo '   ]'
	echo '  }'
	echo ' ]'
	echo '}'

	exit 0
fi

echo '{}'
exit 0
