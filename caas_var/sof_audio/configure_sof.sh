#!/bin/bash
SOFBIN_TPLGS="sof-bin/lib/firmware/intel/sof-tplg-v1.4.2/"
SOFBIN_FWS="sof-bin/lib/firmware/intel/sof/v1.4.2/"
LIB_TPLG="/lib/firmware/intel/sof-tplg/"
LIB_FW="/lib/firmware/intel/sof/"
BLACKLIST_HDA_CONF="/etc/modprobe.d/blacklist-dsp.conf"
SOF_WORK_DIR="$2/sof_audio"
USAGE="Usage : configure_sof.sh install/uninstall WorkingDir"

function setupSof {
    cd $SOF_WORK_DIR
    if [ -d "$LIB_FW" ]; then
        if [ ! -d "sof_bkp" ]; then
            sudo mv $LIB_FW "sof_bkp"
        fi
    fi
    if [ -d "$LIB_TPLG" ]; then
        if [ ! -d "sof-tplg_bkp" ]; then
            sudo mv $LIB_TPLG "sof-tplg_bkp"
        fi
    fi
    if [ -f "$BLACKLIST_HDA_CONF" ]; then
        if [ ! -f "blacklist_hda_bkp" ]; then
            sudo mv $BLACKLIST_HDA_CONF "blacklist_hda_bkp"
        fi
    fi
    sudo mkdir $LIB_FW
    sudo mkdir $LIB_TPLG

    rm -rf sof-bin
    git clone https://github.com/thesofproject/sof-bin -b stable-v1.4.2
    if [ ! -d "sof-bin" ]; then
        echo "Failed to download https://github.com/thesofproject/sof-bin/tree/stable-v1.4.2"
        exit -1
    fi
    sudo cp "${SOFBIN_TPLGS}sof-hda-generic-4ch.tplg" $LIB_TPLG
    sudo cp "${SOFBIN_TPLGS}sof-hda-generic-2ch.tplg" $LIB_TPLG
    sudo cp "${SOFBIN_TPLGS}sof-hda-generic.tplg" $LIB_TPLG
    sudo cp "${SOFBIN_FWS}intel-signed/sof-cnl-v1.4.2.ri" "${LIB_FW}sof-cml.ri"
    sudo cp "${SOFBIN_FWS}intel-signed/sof-cnl-v1.4.2.ri" "${LIB_FW}sof-cnl.ri"
    #sudo cp "blacklist-dsp.conf" $BLACKLIST_HDA_CONF
}

function restore {
    cd $SOF_WORK_DIR
    sudo rm -rf $LIB_FW
    sudo rm -rf $LIB_TPLG
    sudo rm $BLACKLIST_HDA_CONF
    if [ -d "sof_bkp" ]; then
        sudo mv "sof_bkp" $LIB_FW
    fi
    if [ -d "sof-tplg_bkp" ]; then
        sudo mv "sof-tplg_bkp" $LIB_TPLG
    fi
    if [ -f "blacklist_hda_bkp" ]; then
        sudo mv "blacklist_hda_bkp" $BLACKLIST_HDA_CONF
    fi
}

if [[ ! -d $SOF_WORK_DIR ]]; then
    echo "Error : $SOF_WORK_DIR does not exist"
    echo $USAGE
    exit -1
fi

echo "SOF_WORK_DIR=$SOF_WORK_DIR"

if [[ $1 == "install" ]]; then
    setupSof
else
    if [[ $1 == "uninstall" ]]; then
        restore
    else
        echo $USAGE
        exit -1
    fi
fi

