#!/bin/sh
export LCDM_DRIVER_LOGLEVEL="info"
export GWEN_LOGLEVEL="notice"
./chipcardd2 --logtype console --loglevel notice -f --pidfile chipcardd2.pid --store-all-certs
