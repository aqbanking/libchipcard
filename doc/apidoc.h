
/** @defgroup chipcard_libs Libchipcard2 Libraries
 * @short Libraries provided by Libchipcard2
 *
 * This group contains the libraries provided by Libchipcard2.
 */


/** @defgroup chipcard_client Libchipcard2 Client Library
 * @ingroup chipcard_libs
 * @short Client Library of Libchipcard2
 *
 */
/** @defgroup chipcardc_client_app Client Interface for Applications
 * @ingroup chipcard_client
 */

/** @defgroup chipcardc_client_cd Client Interface for Card Implementations
 * @ingroup chipcard_client
 */

/** @defgroup chipcardc_client_sv Client Interface for Service Implementations
 * @ingroup chipcard_client
 */

/** @defgroup chipcardc_card_basic Basic Chipcard Functions
 * @ingroup chipcard_client
 */

/** @defgroup chipcardc_cards Supported Cards
 * @ingroup chipcard_client
 */

/** @defgroup chipcardc_client_ct Chipcard CryptTokens
 * @ingroup chipcard_client
 */


/** @defgroup chipcardc_mon Monitoring Server Activities
 * @ingroup chipcard_client
 */


/** @defgroup MOD_TUTORIALS Tutorials
 */


/** @mainpage Libchipcard2 Documentation Main Page

@section sec_quick Quick Start

Please see
<a href="modules.html">Libchipcard2 Modules</a>
for the API documentation.

@section sec_intro Introduction

Libchipcard2 is a library for generic access to chipcard readers. It contains
a complete ressource manager and uses the hardware drivers provided by
manufacturers of card readers. The number of readers to be used in parallel
is unbound.

Libchipcard2 has some advantages over existing card reader ressource managers:
 - autodetection of readers at multiple buses
 - readers are only allocated upon request by a client (while no client needs
   access to a given reader this reader is not accessed by Libchipcard2)
 - it provides a fake CTAPI which can be used by any CTAPI-aware program
   to use Libchipcard2 via CTAPI

Libchipcard2 autodetects and automatically configures readers at the following
buses:
 - raw USB
 - USB serial
 - PCI
 - PCMCIA



@section sec_needed Needed Components

Libchipcard2 needs the following packages:
- Gwenhywfar http://gwenhywfar.sf.net/          [required]
- LibUSB     http://libusb.sf.net/              [strongly recommended]
- LibSysFS   http://linux-diag.sf.net/          [recommended]
- OpenSC     http://www.opensc.org/             [optional]
- Kernel sources (needed for PCMCIA)            [optional]

If LibUSB is available at compile time it will be used to scan the USB bus
for new devices. Fortunately LibUSB is ported to the major *nix-alike 
systems ;-)
If LibUSB is missing autoconfiguration of USB devices is not possible.

For devices which use a /dev/ttyUSBx device the procfs file 
"/proc/tty/driver/usb-serial" or "/proc/tty/driver/usbserial" is used.
However, revent kernels (2.6.x) only allow root to view these files, so
for these systems LibSysFS (part of sysfsutils) is needed.

If OpenSC is installed then the OpenSC-driver for Libchipcard2 is built and
installed. This driver allows OpenSC to use Libchipcard.

If the kernel sources are installed at compile time then the PCMCIA scanner
code is built. This allows to detect PCMCIA-based card readers.


@section sec_server_setup Server Setup

The chipcard daemon uses the configuration file 
    <b>$PREFIX/etc/chipcard2-server/chipcardd2.conf</b>

This file contains a description of which drivers to load and a list of
configured readers.

You can copy one of the example files installed to 
"$PREFIX/etc/chipcard2-server/". For USB-only readers the minimal example
can be used. For serial devices the file "chipcardd2.conf.example" can be used
as a starting point.


@subsection sec_security Security Mode

The underlying IPC (interprocess communication) model used allows a variety
of security modes:
 
 a) @b local
    This mode uses Unix Domain Sockets. These sockets are only available on
    POSIX systems (not on WIN32 platforms), they can only be connected to from
    the very same machine.
    This is the recommended mode for local-only usage.
    
 b) @b public
    This mode uses simple TCP sockets for IPC. You should not use this mode
    since it does not provide any encryption.
 
 c) @b private
    This mode uses SSL secured sockets. This is the recommended mode on
    systems where there are no Unix Domain Sockets (i.e. the "local" mode is
    not available).
    
 d) @b secure
    This mode is the same as "private", but it requires the client to present
    a valid certificate. This certificate is looked up in a directory on the
    server and if not found access will be denied.
    You can use this mode if you want to be very safe. It also allows the
    server to distinguish between multiple users accessing the server, so that
    administration accounts can be realized (which will only allow special
    users to execute administrative commands).

For security modes "private" and "secure" some additional setup is to be done.
You can do most of that additional setup with the following command:

@code
#>chipcardd2 init
@endcode

This will create all files necessary for "private" or "secure" mode (such as
Diffie-Hellman-parameters, a self-signed certificate etc).

You have to create the server configuration file first (e.g. by just copying
one of the example files provided by this package).

Please refer to the file doc/CERTIFICATES for details.


@subsection sec_adding_readers Adding Readers


@code
#>chipcardd2 addreader --rname ARG --dtype ARG --rtype ARG --rport ARG
@endcode

Adds a reader to the configuration. Please see "chipcardd2 --help".



@section sec_start_daemon Starting The Daemon

<i>chipcardd2 --help</i> lists all possible command line arguments.

However, in most cases the following does suffice:

@code
#>chipcardd2 --pidfile PIDFILENAME
@endcode

where PIDFILENAME is the name of the PID file (used to store the process id
of the server which can be used to send signals to it).


For debugging purposes the following is more usefull:

@code
#>chipcardd2 --pidfile PIDFILENAME -f --logtype console --loglevel notice
@endcode

For "loglevel" you can use "--loglevel info" to increase the verbosity even
more. 
The option "-f" makes the daemon stay in the foreground. In this case you can
stop it using CTRL-C.


Please note that the server doesn't start if there is no line in the
configuration file saying "enabled=1".



@section sec_server_env  Server Environment Variables


@subsection sec_server_env1 LCDM_DRIVER_LOGLEVEL

Loglevel to be used for drivers.
Version before 1.9.16alpha used LC_DRIVER_LOGLEVEL" instead.


@subsection sec_server_env2 LCSV_SERVICE_LOGLEVEL

Loglevel to be used for services.
Version before 1.9.16alpha used "LC_SERVICE_LOGLEVEL" instead.


@subsection sec_server_env2 LC_CTAPI_LOGLEVEL
Loglevel to be used by the fake CTAPI.


@subsection sec_server_env2 OPENSC_LOGLEVEL
Loglevel to be used by the OpenSC reader driver.



@section sec_client_setup Client Setup

Clients for libchipcard2 use the configuration file 
    <b>$PREFIX/etc/chipcard2-client/chipcardc2.conf</b>

This file contains a description of the chipcard2 servers to connect to.



@section sec_own_projects Using Libchipcard2 in your own Projects

Please have a look at the file in tutorials/. They pretty much explain how
Libchipcard2 can be used.



@section sec_remote_drivers Remote Drivers

As of version 1.9.10 Libchipcard2 supports remote drivers. These drivers are
used on thin clients (as requested by GnuMed).

For this to work you need to enable remote drivers in the server configuration
file (<b>$PREFIX/etc/chipcard2-server/chipcardd2.conf</b>).

Also, you will have to add a "server" section in the server configuration file
which is not "local" (because otherwise the remote driver could not connect
to the server).

The next step is to create a configuration file on the thin client which is
used by the remote driver daemon 
     $PREFIX/etc/chipcard2-server/chipcardrd.conf

An example file is provided. The driver sections in such a file are nearly the
same as in the server configuration file.



@section sec_opensc Using OpenSC with Libchipcard2

You can enable this driver with OpenSC by adding the name "chipcard2" to
the OpenSC configuration file variable "app/reader_drivers".
You will also have to add a driver section to that configuration file below
"app":

@code
  reader_driver chipcard2 {
    module = /usr/lib/reader-libchipcard2;
  }
@endcode
  
Such a section allows OpenSC to dynamically load the driver module.



@section sec_macosx Using Libchipcard2 With MacOS

The following commands will compile Libchipcard2 on MacOS:

@code
./configure --prefix=/sw LDFLAGS=-L/sw/lib CFLAGS=-I/sw/include CPPFLAGS=-I/sw/include
make
sudo make install
@endcode

The configuration files are then expected in /sw/chipcard2-server.



@section sec_ccid Using the OpenSource Generic CCID Driver

Libchipcard2 1.9.12beta and later has improved support for this GPL licensed
driver. However, since Libchipcard2 uses this driver directly (i.e. without
using PC/SC) you will have to compile the driver yourself.
I suggest using version ccid-0.9.3 or better. To *compile* the driver you need
libpcsclite (but don't install the pcscd if you want to work with 
Libchipcard2). After compiling the driver you can safely uninstall
libpcsclite and still use the driver with libchipcard2.


@code
./configure (all in one line)
  --disable-pcsclite 
  --enable-usbdropdir=/usr/lib/chipcard2-server/lowlevel/ifd
  --enable-ccidtwindir=/usr/lib/chipcard2-server/lowlevel/ifd
make
make install
@endcode

This allows parallel installation of the non-PCSC version of the driver and
the PCSC version. Libchipcard2 always searches for the CCID driver in its
own lowlevel/ifd folder.

Please note that recent versions of this driver need libpcsclite installed
at build-time, but not at runtime. So please do this:
- install libcpsclite (and devel-packages, if any)
- build the driver as described above
- deinstall libpcsclite (because it might want to start the PC/SC daemon which
  unfortunately tries to completely take over control over all card readers it
  can lay its hands on)



@section sec_change_1916 Changes in Server Configuration File in 1.9.16alpha

For 1.9.16alpha the server engine has been completely rewritten. It is now
much easier to extend and it works much cleaner.

However, the new modular design of the server made it necessary to change the
structure of the configuration file a little.

The current version of Libchipcard2 should be able to read existing files,
but new files should be created according to the new format (see example
files in doc/). Existing files should be modified.

The changes are really minor:
- "Driver" sections are now below the new section "DeviceManager" instead of
  root
- "Service" sections have moved from toplevel to below "ServiceManager"
- "Reader" sections now use the variable "busType" instead of "com" to specify
  how the reader is connected


The new structure of the configuration file is this:


@code
  Server {
    # server settings, haven't changed since previous versions, neither in
    # location nor in content
  }
  
  DeviceManager {
    # This is a new section, it contains the drivers which have been at the
    # topmost level of the configuration file in previous versions.
    
    Driver {
      # Driver section, the content itself is unchanged
      
      Reader {
        # The driver section contains a single change, which doesn't pose much
        # of a problem since a driver and reader section was mostly used with
        # serial devices.
        # The variable "com" (or sometimes "comType") was replaced by the
        # variable "busType" (this is due to the internal reorganization).
        # The following table shows how to convert existing settings:
        #
        #   Previously (com="")  I    New (busType="")
        #   ---------------------+--------------------
        #   com="serial"         I    busType="serial"
        #   com="usb"            I    busType="UsbRaw"
        #   com="usbserial"      I    busType="UsbTty"
        #       ---              I    busType="Pci"
        #       ---              I    busType="Pcmcia"
        #   -------------------------------------------
        # This allowed me to implement additional bus types (like PCMCIA).
        
      } # Reader
    } # Driver
  } # DeviceManager

  CardManager {
    # This is a new optional section which contains some card-related
    # settings (see example in doc/)
  } # CardManager
  
  ServiceManager {
    # This section now contains the "Service" sections which were found in the
    # topmost level in previous files.
    
    Service {
      # This is a service section as in previous versions. The only change is
      # that this section is now below "ServiceManager".
    } # Service
  }
@endcode



@section sec_other_projects Projects Using Libchipcard2

The following is a list of projects which use Libchipcard2. This list
is rather incomplete, please contact the author of you want your project
listed here as well.

 - AqBanking
 - QBankManager (via AqBanking)
 - Gnucash (via AqBanking)
 - KMyMoney (via AqBanking)
 - Grisbi (via AqBanking)
 - Gnumed
 *
 */


