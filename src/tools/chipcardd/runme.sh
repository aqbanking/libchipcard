#!/bin/sh
export LCDM_DRIVER_LOGLEVEL="info"
export GWEN_LOGLEVEL="notice"
#./chipcardd4 --logtype console --loglevel notice -f --pidfile chipcardd3.pid --store-all-certs
./chipcardd4 --logtype console --loglevel info -f --pidfile chipcardd3.pid
