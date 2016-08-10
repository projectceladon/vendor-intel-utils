function do_init_alsa_mixer()
{
	tinymix 12 1
	tinymix 11 100
}

hw_sh=/vendor/etc/init.android_ia.sh
[ -e $hw_sh ] && source $hw_sh

case "$1" in
	init_alsa_mixer)
		do_init_alsa_mixer
		;;
esac

return 0
