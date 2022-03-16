/*
 * libiio - Library for interfacing industrial I/O (IIO) devices
 *
 * Copyright (C) 2014 Analog Devices, Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * */

#ifndef IIO_CONFIG_H
#define IIO_CONFIG_H

#define LIBIIO_VERSION_MAJOR    0
#define LIBIIO_VERSION_MINOR    18
#define LIBIIO_VERSION_GIT    "c0012d0"

#define LOG_LEVEL Info_L

#define WITH_XML_BACKEND
#define WITH_NETWORK_BACKEND

/* #undef WITH_NETWORK_GET_BUFFER */
#define WITH_NETWORK_EVENTFD
#define HAS_PIPE2
#define HAS_STRDUP
#define HAS_STRERROR_R
#define HAS_NEWLOCALE
#define HAS_PTHREAD_SETNAME_NP

#endif /* IIO_CONFIG_H */
