#!/bin/bash
ALSA_CONF="/etc/modprobe.d/alsa-base.conf"
pa_cookie="/home/$SUDO_USER/.config/pulse/cookie"
pa_server="$XDG_RUNTIME_DIR/pulse/native"
mic_gain_cmd="pactl set-source-volume alsa_input.pci-0000_00_1f.3.analog-stereo "
audio_volume="pactl set-sink-volume @DEFAULT_SINK@"
function getCpuInfo {
        echo `lscpu | grep $1 | cut -d ":" -f2 | xargs`
}

function enableHeadset {
        if [[ -z `grep 'dell-headset-multi' $ALSA_CONF` ]];then
                echo "will use dell-headset-multi model for snd-hda-intel"
                echo "options snd-hda-intel model=dell-headset-multi" >> $ALSA_CONF
        fi
}

function setMicGain {
        echo "set alsa mic gain to $1%"
        `PULSE_SERVER=$pa_server PULSE_COOKIE=$pa_cookie $mic_gain_cmd $1%`
        `PULSE_SERVER=$pa_server PULSE_COOKIE=$pa_cookie $audio_volume 30%`
}

cpu_family=$(getCpuInfo 'family:')
cpu_model=$(getCpuInfo 'Model:')
#Additional handling for CML and TGL NUC
if [[ ($cpu_family = 6) && (($cpu_model = 166) || ($cpu_model = 140)) ]]; then
        echo "CML/TGL NUC detected"
        if [[ $1 == "setMicGain" ]]; then
                setMicGain 15
        else
                enableHeadset
        fi
fi

