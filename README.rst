Cloud Streaming
===============

.. contents::

Overview
--------

This repository provides coherent software, drivers and utilities
to setup streaming and video encoding services in the cloud to enable
end-users with remote access to Android cloud instances.

Included here are software, scripts and Docker configurations that
provide:

* **Streaming software**

  * `OWT <https://github.com/open-webrtc-toolkit>`_ based streaming
    service
  * Docker configuration for the easy setup

* **Video Encoding software**

  * `FFmpeg <https://ffmpeg.org/>`_ based video encoding service to
    assist streaming counterpart
  * Docker configuration for the easy setup

* **Helper and test scripts and docker configurations**

  * P2P service docker configuration
  * Chrome browser test client docker configuration

Prerequisites
-------------

* Video encoding service requires `Intel® Data Center GPU Flex Series
  <https://ark.intel.com/content/www/us/en/ark/products/series/230021/intel-data-center-gpu-flex-series.html>`_

* Docker configurations require at least 100GB free disk space available for docker

* Streaming and Video Encoding services are assumed to be running under same bare metal or virtual host

* Android in Container is assumed to be properly installed and running under the same bare metal
  or virtual host as Streaming and Video Encoding services

Installation
------------

We provide dockerfiles to setup streaming and encoding services. These dockerfiles need to be
built into docker containers. Dockerfiles can also be used as a reference instruction
to install ingredients on bare metal.

To build any of the configurations, first clone Cloud Streaming repository::

  git clone https://github.com/projectceladon/cloud-streaming.git && cd cloud-streaming

To setup running environment Android in Container (AIC) instance(s) should be started first.
For the instruction below we will assume that:

* At least 1 AIC is running and ``AIC_INSTANCE`` environment variable is exported to
  point out to AIC workdir::

    export AIC_INSTANCE=/path/to/aic/workdir

* AIC environment has configured Docker network labelled ``android``

* AIC environment has configured P2P server and its IP address under ``android`` network
  was exposed via ``P2P_IP`` environment variable::

    P2P_IP=$(docker inspect -f '{{.NetworkSettings.Networks.android.IPAddress}}' p2p)

Setup Docker
~~~~~~~~~~~~

Docker is required to build and to run Cloud Streaming containers. If you run Ubuntu 22.04
or later you can install it as follows::

  apt-get install docker.io

You might need to further configure docker as follows:

* If you run behind a proxy, configure proxies for for docker daemon. Refer to
  https://docs.docker.com/config/daemon/systemd/.

* Make sure that docker has at least 100GB of hard disk space to use. To check available
  space run (in the example below 390GB are available)::

    $ df -h $(docker info -f '{{ .DockerRootDir}}')
    Filesystem      Size  Used Avail Use% Mounted on
    /dev/sda1       740G  320G  390G  46% /

  If disk space is not enough (for example, default ``/var/lib/docker`` is mounted to
  a small size partition which might be a case for ``/var``), consider reconfiguring
  docker storage location as follows::

    # Below assumes unaltered default docker installation when
    # /etc/docker/daemon.json does not exist
    echo "{\"data-root\": \"/mnt/newlocation\"}" | sudo tee /etc/docker/daemon.json
    sudo systemctl daemon-reload
    sudo systemctl restart docker

Setup Video Encoding Service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Build Video Encoding Service as follows::

  docker build \
    $(env | grep -E '(_proxy=|_PROXY)' | sed 's/^/--build-arg /') \
    --file docker/encoder/ubuntu22.04/selfbuild-prodkmd/Dockerfile \
    --tag cloud-encoder \
    .

Start the service configuring docker container. You can modify video encoding settings
to fine tune the encoding quality, select H.264 or HEVC encoders::

  DEVICE=${DEVICE:-/dev/dri/renderD128}
  DEVICE_GRP=$(stat --format %g $DEVICE)

  # Encode AIC video with H.264 encoder
  ICR_CMDLINE="icr_encoder 0 -streaming \
    -res 1920x1080 -url irrv:264 -plugin qsv -lowpower -tcae 1 \
    -fr 30 -quality 4 -ratectrl VBR -b 3.3M -maxrate 6.6M \
    -hwc_sock /opt/workdir/ipc/hwc-sock"

  docker create \
    -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
    -e render_server_port=23432 \
    --network android \
    -v $AIC_INSTANCE/workdir:/opt/workdir \
    --name icr0 \
    cloud-encoder $ICR_CMDLINE

  docker start video0

Setup Streaming Service
~~~~~~~~~~~~~~~~~~~~~~~

Build Streaming Service as follows::

  docker build \
    $(env | grep -E '(_proxy=|_PROXY)' | sed 's/^/--build-arg /') \
    --file docker/streamer/ubuntu22.04/Dockerfile \
    --tag cloud-streamer \
    .

Start the service configuring docker container::

  DEVICE=${DEVICE:-/dev/dri/renderD128}
  DEVICE_GRP=$(stat --format %g $DEVICE)

  ICR0_IP=$(docker inspect -f '{{.NetworkSettings.Networks.android.IPAddress}}' icr0)

  STREAMER_CMDLINE="ga-server-periodic -n 0 -s ${P2P_IP} \
    --server-peer-id s0 --client-peer-id c0 \
    --icr-ip ${ICR0_IP} \
    --loglevel debug /opt/etc/ga/server.desktop.webrtc.conf"

  docker create \
    -e DEVICE=$DEVICE --device $DEVICE --group-add $DEVICE_GRP \
    --network android \
    -v $AIC_INSTANCE/workdir:/opt/workdir \
    --name streamer0 \
    $IMAGE $STREAMER_CMDLINE

  docker start streamer0

Running the Software
--------------------

Upon successful setup connections to the streaming service should be possible having
appropriate client.


