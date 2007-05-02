
clientconfdir=@lc_client_config_dir@
clientconf_DATA=\
  chipcardc3.conf.example \
  chipcardc3.conf.default

serverconfdir=@lc_server_config_dir@
serverconf_DATA=\
 chipcardd3 \
 chipcardd3.conf.example \
 chipcardd3.conf.default


initscriptdir=/etc/init.d
initscript_DATA=chipcardd3

EXTRA_DIST=$(clientconf_DATA) $(serverconf_DATA) $(initscript_DATA)
