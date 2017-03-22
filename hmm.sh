#! /bin/bash

export LEAP_DEV_ADDR=5
PCAP_FILE=./leap-init.pcapng

tshark \
  -r ${PCAP_FILE} \
  -Y "usb.device_address == ${LEAP_DEV_ADDR} and usb.transfer_type == 0x02 and (usb.setup.bRequest == 0 or usb.setup.bRequest == 192) and frame.number < 1" \
  -X lua_script:./usb_c.lua


