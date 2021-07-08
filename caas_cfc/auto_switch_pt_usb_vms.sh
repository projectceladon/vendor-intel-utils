#!/bin/bash

findall()
{
	args=""
	input="usb_vm.txt"
	if [ ! -s $input ]
	then
		return
	fi
	while IFS=" " read -r line
	do
		args="$args $line"
	done < "$input"
	i=0
	bcnt=0
	pcnt=0
	bs='\'
	for word in $args
	do
		if [ $(( i % 2 )) == 0 ]
		then
			bid[$bcnt]=$word
			bcnt=$bcnt+1
		else
			pid[$pcnt]=$word
			pcnt=$pcnt+1
		fi
		i=$(( i + 1 ))
		#i=$i+1
	done

	i=0
	for elem in "${bid[@]}"
	do
		echo "-device usb-host,hostbus=${bid[$i]},hostport=${pid[$i]}"
		i=$(( i + 1 ))
	done

}

rm -rf usb_vm.txt
python3 $1
findall
