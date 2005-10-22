#!/bin/sh
export LCDM_DRIVER_LOGLEVEL="info"
./chipcardd2 --logtype console --loglevel info -f --pidfile chipcardd2.pid --store-all-certs
