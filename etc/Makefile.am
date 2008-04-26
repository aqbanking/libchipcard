
clientconfdir=@lc_client_config_dir@
clientconf_DATA=\
  chipcardc.conf.example \
  chipcardc.conf.default

serverconfdir=@lc_server_config_dir@
serverconf_DATA=\
 chipcardd.conf.example \
 chipcardd.conf.default \
 chipcardd.conf.slave


initscriptdir=@init_script_dir@
initscript_DATA=chipcardd

EXTRA_DIST=$(clientconf_DATA) $(serverconf_DATA) $(initscript_DATA)
