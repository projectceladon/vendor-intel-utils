#!/vendor/bin/sh

symlink=$(readlink -f /sys/bus/i2c/devices/*INT34C3*)
symlink_dir=`dirname "$symlink"`
i2c_base=`basename "$symlink_dir"`

NUMBER=${i2c_base#i2c-}
hex_i2c_num=$(printf "%x" $NUMBER)
chown audioserver:audio /dev/i2c-$NUMBER
chmod 660 /dev/i2c-$NUMBER

# Set primary channel to idle
dirana_cfg 20 02
# Wait 30ms
usleep 30000

# Set volume on main channel
dirana_cfg F4 40 84 07 FF 00 80
# Wait 30ms
usleep 30000

# Set volume on secondary channel 1
dirana_cfg F4 40 B6 07 FF 00 80
# Wait 30ms
usleep 30000

# Set volume on secondary channel 2
dirana_cfg F4 40 D7 07 FF 00 80
# Wait 30ms
usleep 30000

# Unmute primary channel DSP
dirana_cfg F4 40 9B 07 FF
# Wait 30ms
usleep 30000

# Unmute secondary channel 1 DSP
dirana_cfg F4 40 B8 07 FF
# Wait 30ms
usleep 30000

# Unmute secondary channel 2 DSP
dirana_cfg F4 40 D9 07 FF
# Wait 30ms
usleep 30000

# Enable SRC0
dirana_cfg F3 03 82 80 00 00
# Wait 30ms
usleep 30000

# Enable SRC2
dirana_cfg F3 03 86 80 00 00
# Wait 30ms
usleep 30000

# Enable ADCs
dirana_cfg A9 28 00
# Wait 30ms
usleep 30000

# Configure AD inputs
dirana_cfg A9 00 03
# Wait 30ms
usleep 30000

dirana_cfg A9 01 03
# Wait 30ms
usleep 30000

# Enable Front DACs
dirana_cfg A9 32 00
# Wait 30ms
usleep 30000

# Enable Rear DACs
dirana_cfg A9 33 00
# Wait 30ms
usleep 30000

# Enable Center/Subwoofer DACs
dirana_cfg A9 34 00
# Wait 30ms
usleep 30000

# Configure IIS_IN9 input
dirana_cfg A9 0E 00
# Wait 30ms
usleep 30000

# Overwrite default pin settings
dirana_cfg FF 40 00 21 E0 00 00 03 04
# Wait 30ms
usleep 30000

dirana_cfg FF 40 00 21 F0 00 00 03 04
# Wait 30ms
usleep 30000

dirana_cfg FF 40 00 22 00 00 00 03 04
# Wait 30ms
usleep 30000

# Configure IIS_IN6 (Saturn) input
dirana_cfg A9 1B 00
# Wait 30ms
usleep 30000

# Enable IIS_OUT3 output
dirana_cfg A9 0D 00
# Wait 30ms
usleep 30000

# IIS_OUT3 as output
dirana_cfg A9 38 01
# Wait 30ms
usleep 30000

# Configure IIS_OUT0 output
dirana_cfg A9 16 00
# Wait 30ms
usleep 30000

# Configure SecOut to Async output output
dirana_cfg F4 02 E2 00 01 6F
# Wait 30ms
usleep 30000

dirana_cfg F4 02 AE 00 00 9E
# Wait 30ms
usleep 30000

# Set initial no headroom for tone control, controlled dynamically
dirana_cfg F4 40 A2 07 FF
# Wait 30ms
usleep 30000

# Ma imum headroom GEQ and tone control 32dB ma , compensate upscal and channel gain
dirana_cfg F4 00 50 03 37 18
# Wait 30ms
usleep 30000

dirana_cfg F4 40 88 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 89 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 8A 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 8B 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 8C 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 8D 07 FF
# Wait 30ms
usleep 30000

dirana_cfg F4 40 73 05 0C
# Wait 30ms
usleep 30000

dirana_cfg F4 40 74 05 0C
# Wait 30ms
usleep 30000
dirana_cfg F4 40 75 05 0C
# Wait 30ms
usleep 30000

dirana_cfg F4 40 76 05 0C
# Wait 30ms
usleep 30000

# Add e tra 12db boost
dirana_cfg F4 00 4D 00 00 01
# Wait 30ms
usleep 30000

# Enable ALE, but disable leveler for now, set reference spectrum flat
dirana_cfg F4 0E BD 00 00 00
# Wait 30ms
usleep 30000

dirana_cfg F4 0E BC 00 00 01
# Wait 30ms
usleep 30000

dirana_cfg F4 0E C2 40 00 00
# Wait 30ms
usleep 30000

dirana_cfg F4 0E C3 40 00 00
# Wait 30ms
usleep 30000

dirana_cfg F4 0E C4 40 00 00
# Wait 30ms
usleep 30000

dirana_cfg F4 40 12 00 80
# Wait 30ms
usleep 30000

dirana_cfg F4 40 13 00 80
# Wait 30ms
usleep 30000

dirana_cfg F4 40 14 00 80
# Wait 30ms
usleep 30000

# Audio Power Enable & 48.0kHz
dirana_cfg 3F 0D
# Wait 30ms
usleep 30000

# Set Sine Coefficients based on a frequency(f) of 1kHz and Sample frequency(fs) of 48kHz
# Formula for HE conversion is in Section 1.2 of the SAF775 _UM_Audio v7.2 r1.20 Oct 2014.
# Formula for Sine is in Section 4.27.4
# theta = 2 * Pi * f / fs.
# cos (theta) = 0.991445
# In Python to get HE 24 bit value. he (min(round(0.991445 * 8388608), 8388607)) = 0 7ee7ab
# Split into higher and lower 12 bit values.
# High remains the same = 0 7ee
# Lower is right shifted 1 place = 0 7ab -> RSH 1 -> 0 3d5
# Y:SinGen_aHbak = 0 7ee
# Y:SinGen_aLbak = 0 3d5
dirana_cfg F4 46 14 7 ee
# Wait 30ms
usleep 30000

dirana_cfg F4 46 15 3 d5
# Wait 30ms
usleep 30000

# set output buffers
dirana_cfg C9 09 01
# Wait 30ms
usleep 30000

dirana_cfg C9 05 02
# Wait 30ms
usleep 30000

# enabling TDM's interfaces - Resets TDM output pointer configuration
dirana_cfg A9 50 00
# Wait 30ms
usleep 30000

dirana_cfg A9 54 00
# Wait 30ms
usleep 30000

dirana_cfg A9 55 00
# Wait 30ms
usleep 30000

dirana_cfg A9 56 00
# Wait 30ms
usleep 30000

# configure TDM for 24 bits
dirana_cfg A9 40 00
# Wait 30ms
usleep 30000

dirana_cfg A9 46 00
# Wait 30ms
usleep 30000

dirana_cfg A9 47 00
# Wait 30ms
usleep 30000

# configure TDM rising edge framesync
dirana_cfg A9 2A 02
# Wait 30ms
usleep 30000

dirana_cfg A9 2B 02
# Wait 30ms
usleep 30000

# select number of TDM channels 8
dirana_cfg A9 2C 02
# Wait 30ms
usleep 30000

# set Dirana to be Slave to BCLK of TDM
dirana_cfg A8 05
# Wait 30ms
usleep 30000

# Configure TDM input sync to be Synchronous
dirana_cfg A9 37 01
# Wait 30ms
usleep 30000

# Configure TDMs output pointers
# FrontOutL mapped to TDM0 channels 1&2
dirana_cfg F4 02 B2 00 01 67
# Wait 30ms
usleep 30000

# Au In L&R mapped to TDM0 channels 3&4. Assuming stereo copy
dirana_cfg F4 02 B3 00 00 AE
# Wait 30ms
usleep 30000

# Microphone mapped to TDM0 channel 5
dirana_cfg F4 02 B4 00 00 B0
# Wait 30ms
usleep 30000

# Radio mapped to TDM0 channels 3&4
dirana_cfg F4 02 B2 00 01 70
# Wait 30ms
usleep 30000

# FrontOutL duplicated on TDM1 & TDM2 channels 1&2
dirana_cfg F4 02 BA 00 01 67
# Wait 30ms
usleep 30000

dirana_cfg F4 02 C2 00 01 67
# Wait 30ms
usleep 30000

# Somehow disabling the DAB initialisation has undone this setting
dirana_cfg A9 32 00
# Wait 30ms
usleep 30000

# Enable ADCs
dirana_cfg A9 28 00
# Wait 30ms
usleep 30000

# Set AIN 2 to High common mode 1 Vrms
dirana_cfg A9 02 03
