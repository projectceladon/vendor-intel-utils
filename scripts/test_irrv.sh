#!/bin/bash

# Usage:
#  test_irrv.sh /path/to/aic/workdir streaming:latest icr:latest

SCRIPT_PATH=$(dirname $(which $0))

AIC_WORKDIR=$(realpath $1)
ESC=$2
ICR=$3

set -x

res=0

DEVICE=${DEVICE:-/dev/dri/renderD128}
DEVICE_GRP=$(stat --format %g $DEVICE)

# helper docker to generate content to use as input to aic-emu
FFMPEG_DOCKER="docker run --rm -u $(id -u):$(id -g) \
  -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
  -v $AIC_WORKDIR:/opt/workdir \
  --name ffmpeg \
  $ICR"

AIC_DOCKER="docker create -u $(id -u):$(id -g) \
  -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
  --network host -v $AIC_WORKDIR:/opt/workdir \
  --name emu0 \
  $ICR \
    aic-emu --cmd /opt/workdir/720p.yml \
      --content /opt/workdir/test.bgra \
      --hwc-sock /opt/workdir/hwc-sock \
      --device $DEVICE"

ESC_DOCKER="docker run --rm \
  -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
  --network host \
  --name esc0 \
  $ESC"

ICR_DOCKER="docker create -u $(id -u):$(id -g) \
  -e DEVICE=$DEVICE -e VAAPI_DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
  --network host -v $AIC_WORKDIR:/opt/workdir \
  -e render_server_port=23432 \
  --name icr0 \
  $ICR"

VAINFO_DOCKER="docker run --rm \
  -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
  --name vainfo0 \
  $ICR"

ffmpeg_cmd="ffmpeg -f lavfi -i testsrc=size=1280x720:rate=30 -pix_fmt bgra -frames:v 150 -f rawvideo -y /opt/workdir/test.bgra"

icr_cases=(
  # h264
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:264 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:264 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:264 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:264 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  # h265
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:265 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:265 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:265 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:265 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  # av1
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:av1 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 30 -url irrv:av1 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:av1 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 0 -hwc_sock /opt/workdir/hwc-sock"
  "icr_encoder 0 -streaming -res 1280x720 -fr 60 -url irrv:av1 -plugin qsv -lowpower -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M -tcae 1 -hwc_sock /opt/workdir/hwc-sock"
  )

function get_framerate() {
  echo "$1" | sed -E 's/.*fr ([0-9\.]*).*/\1/'
}

function is_av1_test() {
  if echo $1 | grep -q av1; then
    return 0
  fi
  return 1
}

function check_av1() {
  if $VAINFO_DOCKER vainfo --display drm --device $DEVICE 2>/dev/null | grep VAEntrypointEncSliceLP | grep -q VAProfileAV1; then
    return 0
  fi
  return 1
}

$FFMPEG_DOCKER $ffmpeg_cmd

count=0

for c in "${icr_cases[@]}"; do
  echo ">>> Testing with:"
  echo "$c"
  echo "<<<"

  if is_av1_test "$c" && ! check_av1; then
    echo "Skipping test since av1 encoding is not supported"
    continue
  fi

  if ! $AIC_DOCKER; then
    res=1
  fi
  if ! $ICR_DOCKER $c; then
    res=1
  fi
  docker start emu0
  docker start icr0
  if ! $ESC_DOCKER irrv-client-test --framerate $(get_framerate "$c"); then
    res=1
  fi
  docker logs icr0 > $AIC_WORKDIR/logs_$count.icr0.txt 2>&1
  docker logs emu0 > $AIC_WORKDIR/logs_$count.emu0.txt 2>&1
  docker stop icr0 emu0
  docker rm icr0 emu0

  count=$((++count))
done

exit $res

