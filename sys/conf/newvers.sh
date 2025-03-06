#!/bin/sh

USERNAME=${USER-root}
HOSTNAME=`hostname`
DATE=`LC_TIME="" date`

echo "char ostype[] = \"OSIX\";" > /tmp/vers.c
echo "char osrelease[] = \"OSIX-0.1\";" >> /tmp/vers.c
echo "char version[] = \"OSIX-0.1: ${DATE}\\n    ${USERNAME}@${HOSTNAME}:${PWD}\";" >> /tmp/vers.c
