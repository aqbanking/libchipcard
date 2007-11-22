#!/bin/sh
export LCDM_DRIVER_LOGLEVEL=info
#./chipcardd4 --logtype console --loglevel info -f --pidfile chipcardd3.pid --store-all-certs --runonce 2
/usr/sbin/chipcardd4 --logtype console --loglevel info -f --pidfile chipcardd4.pid
