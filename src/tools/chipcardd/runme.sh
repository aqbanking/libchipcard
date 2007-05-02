#!/bin/sh
export LCDM_DRIVER_LOGLEVEL="info"
export GWEN_LOGLEVEL="notice"
#./chipcardd3 --logtype console --loglevel notice -f --pidfile chipcardd3.pid --store-all-certs
./chipcardd --logtype console --loglevel info -f --pidfile chipcardd3.pid --accept-all-certs
