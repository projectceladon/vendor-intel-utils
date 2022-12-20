Control Protocol Specification
==============================

.. contents::

High Level Spec
---------------

Control protocol messages are received in a form of ASCII string messages
(``std::string`` or ``char*``). On the high level message might contain one
of the following::

  "start"|json-object

``"start"`` message is received in the very beginning of streaming once
client got connected. Upon receiving this message webrtc server is supposed
to publish outgoing streams and be ready to receive new incoming control
messages.

``json-object`` represents a control message which can be parameterized.
Template for such json messages is the following::

  {
    "type": "control",
    "data": {
      "event": EVENT_ID,
      "parameters": {       # parameters are optional
        "PARAM-1": VALUE-1,
        "PARAM-2": VALUE-2,
        ...
      }
    }
  }

These control messages contain description of different client events such
as mouse or touch screen clicks, mouse movements, keyboard button presses,
etc. In the following sections we describe each event and its parameters.

Supported Control Events
------------------------

The following table lists supported control events depending on reference
stack implementation:

* Intel Cloud Streaming System Software on Android OS (aka ICS3A)
* Cloud Gaming Reference Stack for Windows (aka WinCGRS)

+-----------------------+---------------+---------------+
| EVENT_ID              | WinCGRS       | ICS3A         |
+=======================+===============+===============+
| ``touch``             | not supported | supported     |
+-----------------------+---------------+---------------+
| ``joystick``          | not supported | supported     |
+-----------------------+---------------+---------------+
| ``gps``               | not supported | supported     |
+-----------------------+---------------+---------------+
| ``mousemove``         | supported     | supported     |
+-----------------------+---------------+---------------+
| ``mouseup``           | supported     | supported     |
+-----------------------+---------------+---------------+
| ``mousedown``         | supported     | supported     |
+-----------------------+---------------+---------------+
| ``wheel``             | supported     | not supported |
+-----------------------+---------------+---------------+
| ``keydown``           | supported     | not supported |
+-----------------------+---------------+---------------+
| ``keyup``             | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoykeydown``       | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoykeyup``         | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoylstick``        | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoyrstick``        | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoyltrigger``      | supported     | not supported |
+-----------------------+---------------+---------------+
| ``vjoyrtrigger``      | supported     | not supported |
+-----------------------+---------------+---------------+
| ``sizechange``        | supported     | supported     |
+-----------------------+---------------+---------------+
| ``pointerlockchange`` | supported     | not supported |
+-----------------------+---------------+---------------+
| ``framestats``        | supported     | supported     |
+-----------------------+---------------+---------------+
| ``sensorcheck``       | not supported | supported     |
+-----------------------+---------------+---------------+
| ``sensordata``        | not supported | supported     |
+-----------------------+---------------+---------------+
| ``cmdchannel``        | not supported | supported     |
+-----------------------+---------------+---------------+
| ``camerainfo``        | not supported | supported     |
+-----------------------+---------------+---------------+

Below we give json template for each event followed by discussion of
paramers whenever needed. These templates are not a strict json examples.
Instead of real parameter values we give C/C++ types to which parameter
value will be casted.

GPS
~~~

This event sends GPS data to the remote device. GPS Sensor first needs to be
enabled which is done upon receiving "gps-start" message from remote device.
See ``GPS Control Commands`` for details.

::

  {
    "type": "control"
    "data": {
      "event": "gps",
      "parameters": {
        "data": std::string
      }
    }
  }

``"data"`` in ``"parameters"`` is the GPS data. Currently, it only supports Global Positioning System Fix Data (GPGGA).

For more detail, please reference http://aprs.gids.nl/nmea/#gga.

Touch Events
~~~~~~~~~~~~

::

  {
    "type": "control"
    "data": {
      "event": "touch",
      "parameters": {
        "data": char*,
      }
    }
  }

``data`` in ``parameters`` represents single or multiple touch commands.
Multiple commands are delimited by ``\n``. See https://github.com/openstf/minitouch
for commands protocol description. Command examples:

* "d <contact> <x> <y> <pressure>" - touch down command.
* "m <contact> <x> <y> <pressure>" - move command.
* "u <contact>" - touch up command.
* "c" - commit command.
* "r" - reset command.
* "w <ms>" - wait command.


Joystick
~~~~~~~~

::

  {
    "type": "control"
    "data": {
      "event": "joystick",
      "parameters": {
        "data": char*,
        "jID": unsigned int
      }
    }
  }

``"jID"`` is the ID of the joystick.

``"data"`` in ``"parameters"`` represents single or multiple commands for the Joystick.
Multiple commands are delimited by ``\n``. Unknown commands should be ignored by remote
device. Supported commands are:

`c`
  Commits the current set of joystick events causing them to play out on the
  screen. Note that nothing visible will happen until you commit.

`k <code> <value>`
  Joystick button message.

  ``<code>`` is expected to be one of the following Android "Unified Scan Codes"
  defined in `Generic.kl <https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/data/keyboards/Generic.kl>`_:

  +------+---------------+
  | Code | Button        |
  +======+===============+
  | 304  | BUTTON_A      |
  +------+---------------+
  | 305  | BUTTON_B      |
  +------+---------------+
  | 307  | BUTTON_X      |
  +------+---------------+
  | 308  | BUTTON_Y      |
  +------+---------------+
  | 310  | BUTTON_L1     |
  +------+---------------+
  | 311  | BUTTON_R1     |
  +------+---------------+
  | 312  | BUTTON_L2     |
  +------+---------------+
  | 313  | BUTTON_R2     |
  +------+---------------+
  | 314  | BUTTON_SELECT |
  +------+---------------+
  | 315  | BUTTON_START  |
  +------+---------------+
  | 316  | BUTTON_MODE   |
  +------+---------------+
  | 317  | BUTTON_THUMBL |
  +------+---------------+
  | 318  | BUTTON_THUMBR |
  +------+---------------+

  A client upon receiving a Joystick real scancode (different Joysticks might
  have different scancodes) should convert it to Android "Unified Scan Code".

  ``<value>=0|1``, where 0 - key is down, 1 - key is up.

`m <code> <value>`
  Joystick miscellaneous message. Reserved.

`a <code> <value>`
  Joystick axis message. ``<code>`` is expected to be one of:

  * One of the following Android "Unified Scan Code" for "axis" defined in `Generic.kl <https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/data/keyboards/Generic.kl>`_:

  +----------+---------------------+----------+
  | Code     | ``<value>`` range   | Notes    |
  +==========+=====================+==========+
  | ``0x10`` | -1~1                | HAT_X    |
  +----------+---------------------+----------+
  | ``0x11`` | -1~1                | HAT_Y    |
  +----------+---------------------+----------+
  | ``0x00`` | -128~127            | X        |
  +----------+---------------------+----------+
  | ``0x01`` | -128~127            | Y        |
  +----------+---------------------+----------+
  | ``0x02`` | -128~127            | Z        |
  +----------+---------------------+----------+
  | ``0x05`` | -128~127            | RZ       |
  +----------+---------------------+----------+

  * Addional custom codes: ``0x3e`` (LTRIGGER) and ``0x3f`` (RTRIGGER)

  +----------+---------------------+----------+
  | Code     | ``<value>`` range   | Notes    |
  +==========+=====================+==========+
  | ``0x3e`` | 0~255               | LTRIGGER |
  +----------+---------------------+----------+
  | ``0x3f`` | 0~255               | RTRIGGER |
  +----------+---------------------+----------+

  Similar to Joystick button messages user needs to convert real Joystick scan
  codes to the supported code values described above (Android Unified Scan
  Code or additional custom code).

`i`
  Joystick was plugged-in.

`p`
  Joystick disconnected.

Mouse Events
~~~~~~~~~~~~

mousemove
^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "mousemove",
      "parameters": {
        "eventTimeSec": struct timeval::tv_sec,  # WinCGRS only
        "eventTimeSec": struct timeval::tv_usec, # WinCGRS only
        "x": long,
        "y": long,
        "movementX", # WinCGRS only, if relative position enabled
        "movementY", # WinCGRS only, if relative position enabled
      }
    }
  }

mouseup
^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "mouseup",
      "parameters": { # WinCGRS only
        "x": long,
        "y": long,
        "which": unsigned int,
      }
    }
  }

``"which"`` is one of:

* 1 - left
* 2 - middle
* 3 - right

mousedown
^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "mousedown",
      "parameters": {
        "x": long,
        "y": long,
        "which": unsigned int, # WinCGRS only
      }
    }
  }

``"which"`` is one of:

* 1 - left
* 2 - middle
* 3 - right

wheel
^^^^^

Supported on WinCGRS. Not supported on ICS3A. ::

  {
    "type": "control"
    "data": {
      "event": "wheel",
      "parameters": {
        "deltaX": float,
        "deltaY": float,
      }
    }
  }

Keyboard Events
~~~~~~~~~~~~~~~

Supported on WinCGRS. Not supported on ICS3A.

keydown and keyup
^^^^^^^^^^^^^^^^^

Keyboard events are supported only by WinCGRS. ::

  {
    "type": "control"
    "data": {
      "event": "keydown"|"keyup",
      "parameters": {
        "which": unsigned int,
      }
    }
  }

``"which"`` - TBD list of key codes

Joystick Events
~~~~~~~~~~~~~~~

Supported on WinCGRS. Not supported on ICS3A.

vjoykeydown and vjoykeyup
^^^^^^^^^^^^^^^^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "vjoykeydown"|"vjoykeyup",
      "parameters": {
        "which": unsigned int,
      }
    }
  }

``"which"`` - TBD list of key codes

vjoylstick
^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "vjoylstick",
      "parameters": {
        "lx": unsigned int,
        "ly": unsigned int,
      }
    }
  }

vjoyrstick
^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "vjoyrstick",
      "parameters": {
        "rx": unsigned int,
        "ry": unsigned int,
      }
    }
  }

vjoyltrigger and vjoyrtrigger
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "vjoyltrigger"|"vjoyrtrigger",
      "parameters": {
        "trigger": double,
      }
    }
  }

Sensors Events
~~~~~~~~~~~~~~

sensorcheck
^^^^^^^^^^^

"sensorcheck" is a query event to know which sensors are to be configured in client.
In response to "sensorcheck" event, streamer sends each sensor's enable/disable command to client app.
Commands are explained at ``Sensor control commands``.

Supported on ICS3A ::

  {
    "type": "control"
    "data": {
      "event": "sensorcheck",
    }
  }


sensordata
^^^^^^^^^^

"sensordata" is a data event containing sensor's type and data values.
Data events are received whenever sensor data is available at client.

Supported on ICS3A ::

  {
    "type": "control"
    "data": {
      "event": "sensordata",
      "parameters": {
        "type": int,
        "data": [float, float, ...],
      }
    }
  }

``"type"`` is an android sensor type defined in
https://android.googlesource.com/platform/hardware/libhardware/+/master/include/hardware/sensors-base.h

``"data"`` is an array of sensor data values.

Command Channel
~~~~~~~~~~~~~~~

cmdchannel
^^^^^^^^^^

"cmdchannel" is a data event containing package name to start on remote system and/or shell command to execute on remote system. Streaming server should send back ``activity-switch`` message to the client containing status of the executed package or command.
For remote Android system, "pkg" is package name of android apk, "cmd" currently supports am, pm, input, dumpsys, setprop and monkey, in order to support multi-user scenario, we have customized a "get" command to query android user id and android display id according to the streamer user id, command format: get self [--user-id <STREAMER_USER_ID>].

Supported on ICS3A ::

  {
    "type": "control"
    "data": {
      "event": "cmdchannel",
      "parameters": {
        "pkg": char*,
        "cmd": char*,
      }
    }
  }

Miscellaneous
~~~~~~~~~~~~~

videoalpha
^^^^^^^^^^

Event requests server to send data with alpha channel. ``action`` with the value 1  enables alpha
channel, with value 0 - disables.

::

  {
    "type": "control"
    "data": {
      "event": "videoalpha",
      "parameters": {
        "action": uint32_t
      }
    }
  }

sizechange
^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "sizechange",
      "parameters": {
        "rendererSize": {
          "width": unsigned int,
          "height": unsigned int,
        },
        "mode": "stretch", # fit mode if not specified
      }
    }
  }

pointerlockchange
^^^^^^^^^^^^^^^^^

Supported on WinCGRS. Not supported on ICS3A. ::

  {
    "type": "control"
    "data": {
      "event": "pointerlockchange",
      "parameters": {
        "locked": bool,
      }
    }
  }

framestats
^^^^^^^^^^

::

  {
    "type": "control"
    "data": {
      "event": "framestats",
      "parameters": {
        "framets": long,
        "framesize": long,
        "framedelay": int,
        "framestartdelay": long,
        "packetloss": long,
      }
    }
  }

Camera Event
~~~~~~~~~~~~

camerainfo
^^^^^^^^^^

"camerainfo" is an event containing remote client camera(s) capability
info such as number of camera(s) available, resolution, orientation,
camera facing etc.
This event is received immediately when client is connected.

Supported on ICS3A ::

  {
    "type": "control"
    "data": {
      "event": "camerainfo",
      "parameters": {
        "numOfCameras": int,
        "camOrientation": [cam1_orientation, cam2_orientation, ...],
        "camFacing": [cam1_facing, cam2_facing, ...],
        "maxCameraRes": [cam1_maxRes, cam2_maxRes, ...],
      }
    }
  }

::

  camOrientation[i]=NULL|0|90|180|270

"camOrientation" is the image sensor orientation of each camera separately
as an array of strings.

::

  camFacing[i]=NULL|back|front

"camFacing" is the facing info of each camera separately as an array of
strings.

::

  maxCameraRes[i]=NULL|480p|720p|1080p|2160p|4320p

"maxCameraRes" is the maximum supported resolution of each camera separately
as an array of strings.

`NULL` string has special meaning marking missed camera information.

Control Commands to Client
-------------------------

The control commands are the messages sent to client upon request from AIC.
Command messages are in json-message format with the following template::

  {
    "key": "Control Command"
    "PARAM1": VALUE1,
    "PARAM2": VALUE2,
    ...
  }

Supported Commands
~~~~~~~~~~~~~~~~~~

+--------------------------+---------------+---------------+
| Control Command          | WinCGRS       | ICS3A         |
+==========================+===============+===============+
| ``gps-start``            | not supported | supported     |
+--------------------------+---------------+---------------+
| ``gps-stop``             | not supported | supported     |
+--------------------------+---------------+---------------+
| ``gps-quit``             | not supported | supported     |
+--------------------------+---------------+---------------+
| ``sensor-start``         | not supported | supported     |
+--------------------------+---------------+---------------+
| ``sensor-stop``          | not supported | supported     |
+--------------------------+---------------+---------------+
| ``activity-switch``      | not supported | supported     |
+--------------------------+---------------+---------------+
| ``user-id``              | not supported | supported     |
+--------------------------+---------------+---------------+
| ``cmd-output``           | not supported | supported     |
+--------------------------+---------------+---------------+
| ``start-camera-preview`` | not supported | supported     |
+--------------------------+---------------+---------------+
| ``stop-camera-preview``  | not supported | supported     |
+--------------------------+---------------+---------------+
| ``cameraRes``            | not supported | supported     |
+--------------------------+---------------+---------------+
| ``cameraId``             | not supported | supported     |
+--------------------------+---------------+---------------+
| ``start-audio-rec``      | not supported | supported     |
+--------------------------+---------------+---------------+
| ``stop-audio-rec``       | not supported | supported     |
+--------------------------+---------------+---------------+
| ``start-audio-play``     | not supported | supported     |
+--------------------------+---------------+---------------+
| ``stop-audio-play``      | not supported | supported     |
+--------------------------+---------------+---------------+
| ``video-alpha-success``  | not supported | supported     |
+--------------------------+---------------+---------------+

GPS Control Commands
~~~~~~~~~~~~~~~~~~~~

gps-start
^^^^^^^^^

Client should enable GPS sensor if it is not enabled and start
sending GPS data.

::

  {
    "key": "gps-start"
  }

gps-stop
^^^^^^^^

Client should stop sending GPS data. GPS sensor however should not
be disabled. Instead it should be kept ready to start sending
GPS data again (upon receiving "gps-start" message).

::

  {
    "key": "gps-stop"
  }

gps-quit
^^^^^^^^

Client should disable GPS sensor.

::

  {
    "key": "gps-stop"
  }

Sensor Control Commands
~~~~~~~~~~~~~~~~~~~~~~~

Sensor control commands are sent to client to enable/disable sensors.

The process is started by remote gaming device (for example, Android in
Container) which requests streamer to enable/disable client sensors.
Initially all control requests sent by AIC will be under queue until
``sensorcheck`` event is received from the client. Later, sensor control
requests are directly forwarded to the client.

sensor-start
^^^^^^^^^^^^

This command is used to enable client sensor.

::

  {
    "key": "sensor-start"
    "type": int
    "samplingPeriod_ms": int
  }

``"type"`` is android sensor type defined in https://android.googlesource.com/platform/hardware/libhardware/+/master/include/hardware/sensors-base.h
``"samplingPeriod_ms"`` is the delay value in milliseconds at which android sensor's data samples are collected.

sensor-stop
^^^^^^^^^^^

This command is used to disable client sensor.

::

  {
    "key": "sensor-stop"
    "type": int
  }

``"type"`` is android sensor type defined in https://android.googlesource.com/platform/hardware/libhardware/+/master/include/hardware/sensors-base.h

Command Channel Control Commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

activity-switch
^^^^^^^^^^^^^^^

This message provides information to the client on the activity happening on the remote system. Format of the incoming information is implementation specific. Message can be sent on starting or closing applications, mouse or touch moves, etc.

::

  {
    "key": "activity-switch"
    "val": char*
  }

For remote Android client ``val`` has the ``Type:Field1:Field2`` pattern, where:

* ``Type=0`` - activity switched normally
* ``Type=1`` - activity switched abnormally
* ``Type=2`` - general app crash
* ``Type=3`` - app early not responding
* ``Type=4`` - app not responding

For example::

  {
    "key" : "activity-switch",
    "val" : "0:com.android.launcher3/com.android.launcher3.uioverrides.QuickstepLauncher:com.android.gallery3d/com.android.gallery3d.app.GalleryActivity"
  }

user-id
^^^^^^^

This message provides streamer user id to client, which can be used to query android user id and android display id.

::

  {
    "key": "user-id"
    "val": char*
  }

``"val"`` is streamer user id value.

cmd-output
^^^^^^^^^^

This message provides output to the client about the result of the command execution.

::

  {
    "key": "cmd-output"
    "val": {
      "cmd": char*,
      "output": char*
    }
  }

``"cmd"`` is command to be executed.
``"output"`` is output of the command.

Camera Control Commands
~~~~~~~~~~~~~~~~~~~~~~~

start-camera-preview
^^^^^^^^^^^^^^^^^^^^

To start the camera stream from client when server requests
openCamera.

::

  {
    "key": "start-camera-preview"
  }

stop-camera-preview
^^^^^^^^^^^^^^^^^^^^

To stop the camera stream from client when server requests
closeCamera.

::

  {
    "key": "stop-camera-preview"
  }

cameraRes
^^^^^^^^^

To pass the corresponding user requested resolution info
when open the camera stream.

::

  {
    "key": "cameraRes"
  }


cameraId
^^^^^^^^

To pass the corresponding user requested camera Id info
when open the camera stream. This is useful when client
have multiple cameras. This info is used to identify
between those cameras.

::

  {
    "key": "cameraId"
  }

Audio Control Commands
~~~~~~~~~~~~~~~~~~~~~~~

start-audio-rec
^^^^^^^^^^^^^^^

Command to start the audio input(record) stream from client.

::

  {
    "key": "start-audio-rec"
  }

stop-audio-rec
^^^^^^^^^^^^^^

Command to stop the audio input(record) stream from client.

::

  {
    "key": "stop-audio-rec"
  }

start-audio-play
^^^^^^^^^^^^^^^^

Command to notify the start of audio output(playback) to client.

::

  {
    "key": "start-audio-play"
  }

stop-audio-play
^^^^^^^^^^^^^^^

Command to notify the stop of audio output(playback) to client.

::

  {
    "key": "stop-audio-play"
  }

Video Control Command
~~~~~~~~~~~~~~~~~~~~~

video-alpha-success
^^^^^^^^^^^^^^^^^^^

Server acknowledges that it supports alpha video request by sending "video-alpha-success".
Client should expect incoming video bitstream to contain AV_FRAME_DATA_SEI_UNREGISTERED messages
which should be parsed before decoding to check if incoming bitstream has alpha video encoded frames.

::

  {
    "key": "video-alpha-success"
  }

