#!/bin/sh

if [ $# -ne 2 ]
then
echo usage: $0 tostr/fromstr filename
exit
fi

if [ -s $2 ]
then

if [ $1 = "tostr" ]
then
cat $2 | sed 's/[ \t]*$//;s/\t/    /' | gawk '{print "\"" $0 "\\n\"" }'
else
cat $2 | sed 's/^"//;s/\\n"$//'
fi

else
echo $2 is not a valid file
exit
fi

