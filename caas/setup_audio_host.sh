#!/bin/bash
ALSA_CONF="/etc/modprobe.d/alsa-base.conf"
function getCpuInfo {
        echo `lscpu | grep $1 | cut -d ":" -f2 | xargs`
}

cpu_family=$(getCpuInfo 'family:')
cpu_model=$(getCpuInfo 'Model:')
#Additional param for CML NUC
if [[ ($cpu_family = 6) && ($cpu_model = 166) ]]; then
        echo "CML NUC detected"
        if [[ -z `grep 'dell-headset-multi' $ALSA_CONF` ]];then
                echo "will use dell-headset-multi model for snd-hda-intel"
                echo "options snd-hda-intel model=dell-headset-multi" >> $ALSA_CONF
        fi
fi

