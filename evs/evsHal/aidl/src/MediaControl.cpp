/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (C) 2015-2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MediaControl.h"
#include "VideoCapture.h"

#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>

#include <stack>
#include <string>
#include <dirent.h>
#include <dlfcn.h>
#include "SysCall.h"

using std::string;
using std::vector;

#define MAX_SYS_NAME 64
#define MAX_TARGET_NAME 256
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

struct MediaLink {
    MediaPad* source;
    MediaPad* sink;
    MediaLink* twin;
    uint32_t flags;
    uint32_t padding[3];
};

struct MediaPad {
    MediaEntity* entity;
    uint32_t index;
    uint32_t flags;
    uint32_t padding[3];
};

struct MediaEntity {
    media_entity_desc info;
    MediaPad* pads;
    MediaLink* links;
    unsigned int maxLinks;
    unsigned int numLinks;

    char devname[32];
};

MediaControl* MediaControl::sInstance = nullptr;
std::mutex MediaControl::sLock;

int MediaControl::createLink() {
    int ret = 0;
    McFormat formats[] = {
        {.entityName = "Intel IPU6 CSI-2 1",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI-2 1",
         .pad = 1,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "isx031 a",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "isx031 b",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "isx031 c",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "isx031 d",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "TI960 a",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "TI960 a",
         .pad = 1,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "TI960 a",
         .pad = 2,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "TI960 a",
         .pad = 3,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "TI960 a",
         .pad = 4,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI2 BE SOC 0",
         .pad = 0,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI2 BE SOC 0",
         .pad = 1,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI2 BE SOC 0",
         .pad = 2,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI2 BE SOC 0",
         .pad = 3,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
        {.entityName = "Intel IPU6 CSI2 BE SOC 0",
         .pad = 4,
         .pixelCode = V4L2_MBUS_FMT_UYVY8_1X16,
         .width = WIDTH,
         .height = HEIGHT},
    };

    McLink links[] = {
        {.srcEntityName = "isx031 a", .srcPad = 0, .sinkEntityName = "TI960 a", .sinkPad = 0},
        {.srcEntityName = "isx031 b", .srcPad = 0, .sinkEntityName = "TI960 a", .sinkPad = 1},
        {.srcEntityName = "isx031 c", .srcPad = 0, .sinkEntityName = "TI960 a", .sinkPad = 2},
        {.srcEntityName = "isx031 d", .srcPad = 0, .sinkEntityName = "TI960 a", .sinkPad = 3},
        {.srcEntityName = "TI960 a",
         .srcPad = 4,
         .sinkEntityName = "Intel IPU6 CSI-2 1",
         .sinkPad = 0},
        {.srcEntityName = "Intel IPU6 CSI-2 1",
         .srcPad = 1,
         .sinkEntityName = "Intel IPU6 CSI2 BE SOC 0",
         .sinkPad = 0},
        {.srcEntityName = "Intel IPU6 CSI2 BE SOC 0",
         .srcPad = 1,
         .sinkEntityName = "Intel IPU6 BE SOC capture 0",
         .sinkPad = 0},
        {.srcEntityName = "Intel IPU6 CSI2 BE SOC 0",
         .srcPad = 2,
         .sinkEntityName = "Intel IPU6 BE SOC capture 1",
         .sinkPad = 0},
        {.srcEntityName = "Intel IPU6 CSI2 BE SOC 0",
         .srcPad = 3,
         .sinkEntityName = "Intel IPU6 BE SOC capture 2",
         .sinkPad = 0},
        {.srcEntityName = "Intel IPU6 CSI2 BE SOC 0",
         .srcPad = 4,
         .sinkEntityName = "Intel IPU6 BE SOC capture 3",
         .sinkPad = 0},
    };

    for (McFormat& format : formats) {
        MediaEntity* entity = getEntityByName(format.entityName.c_str());
        int fd = -1;
        if (entity) fd = ::open(entity->devname, O_RDWR);

        ALOGD("@%s, set format for %s  pad: %d", __func__, format.entityName.c_str(), format.pad);
        if (fd >= 0) {
            v4l2_mbus_framefmt mbusfmt;
            memset(&mbusfmt, 0, sizeof(mbusfmt));
            mbusfmt.width = format.width;
            mbusfmt.height = format.height;
            mbusfmt.code = format.pixelCode;

            struct v4l2_subdev_format fmt = {};
            fmt.pad = format.pad;
            fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
            fmt.format = mbusfmt;
            fmt.stream = format.stream;
            ret = ::ioctl(fd, VIDIOC_SUBDEV_S_FMT, &fmt);
            close(fd);
            if (ret < 0) {
                ALOGE("@%s, Fail set format for %s  pad: %d", __func__, format.entityName.c_str(),
                      format.pad);
            }
        } else {
            // continue link others when fails
            ALOGE("@%s, Fail set open device %s", __func__, format.entityName.c_str());
        }
    }

    for (McLink& link : links) {
        ALOGD("@%s, set link : %s --> %s", __func__, link.srcEntityName.c_str(),
              link.sinkEntityName.c_str());
        ret = -1;
        int srcEntity = getEntityIdByName(link.srcEntityName.c_str());
        int sinkEntity = getEntityIdByName(link.sinkEntityName.c_str());
        if (srcEntity >= 0 && sinkEntity >= 0) {
            ret = setupLink(srcEntity, link.srcPad, sinkEntity, link.sinkPad, true);
        }
        if (ret < 0) {
            ALOGE("@%s, Fail set link : %s --> %s", __func__, link.srcEntityName.c_str(),
                  link.sinkEntityName.c_str());
        }
    }
    return ret;
}

void MediaControl::getDeviceName(const char* entityName, string& deviceNodeName, bool isSubDev) {
    const char* filePrefix = "video";
    const char* dirPath = "/sys/class/video4linux/";
    if (isSubDev) filePrefix = "v4l-subdev";

    DIR* dp = opendir(dirPath);
    if (dp == nullptr) {
        ALOGE("@%s, Fail open : %s", __func__, dirPath);
    }

    struct dirent* dirp = nullptr;
    while ((dirp = readdir(dp)) != nullptr) {
        if ((dirp->d_type == DT_LNK) &&
            (strncmp(dirp->d_name, filePrefix, strlen(filePrefix)) == 0)) {
            string subDeviceName = dirPath;
            subDeviceName += dirp->d_name;
            subDeviceName += "/name";
            int fd = open(subDeviceName.c_str(), O_RDONLY);
            if (fd < 0) {
                ALOGE("@%s, Fail open : %s", __func__, subDeviceName.c_str());
            }

            char buf[128] = {'\0'};
            int len = read(fd, buf, sizeof(buf));
            close(fd);
            len--;  // remove "\n"
            if (len == (int)strlen(entityName) && memcmp(buf, entityName, len) == 0) {
                deviceNodeName = "/dev/";
                deviceNodeName += dirp->d_name;
                break;
            }
        }
    }
    closedir(dp);
}

MediaControl* MediaControl::getMediaControlInstance() {
    MediaControl* mediaControlInstance = nullptr;

    for (int i = 0; i < MEDIA_DEVICE_MAX_NUM; i++) {
        std::string fileName = MEDIA_CTL_DEV_NAME;
        fileName.append(std::to_string(i));

        struct stat fileStat = {};
        int ret = stat(fileName.c_str(), &fileStat);
        if (ret != 0) {
            ALOGE("%s: There is no file %s", __func__, fileName.c_str());
            continue;
        }

        SysCall* sc = SysCall::getInstance();
        int fd = sc->open(fileName.c_str(), O_RDWR);
        if (fd < 0) {
            ALOGE("%s, Open media device(%s) failed: %s", __func__, fileName.c_str(),
                  strerror(errno));
            break;
        }

        media_device_info info;
        ret = sc->ioctl(fd, MEDIA_IOC_DEVICE_INFO, &info);
        if ((ret != -1) &&
            (0 == strncmp(info.driver, MEDIA_DRIVER_NAME, strlen(MEDIA_DRIVER_NAME)))) {
            mediaControlInstance = new MediaControl(fileName.c_str());
        }

        if (sc->close(fd) < 0) {
            ALOGE("Failed to close media device %s:%s", fileName.c_str(), strerror(errno));
        }

        if (mediaControlInstance) {
            ALOGE("%s: media device name:%s", __func__, fileName.c_str());
            break;
        }
    }

    return mediaControlInstance;
}

MediaControl* MediaControl::getInstance() {
    std::unique_lock<std::mutex> lock(sLock);
    if (!sInstance) {
        sInstance = getMediaControlInstance();
    }
    return sInstance;
}

void MediaControl::releaseInstance() {
    std::unique_lock<std::mutex> lock(sLock);

    if (sInstance) {
        delete sInstance;
        sInstance = nullptr;
    }
}

MediaControl::MediaControl(const char* devName) : mDevName(devName) {}

MediaControl::~MediaControl() {}

int MediaControl::initEntities() {
    mEntities.reserve(100);

    int ret = enumInfo();
    if (ret != 0) {
        ALOGE("Enum Info failed.\n");
        return -1;
    }

    return 0;
}

void MediaControl::clearEntities() {
    auto entity = mEntities.begin();
    while (entity != mEntities.end()) {
        delete[] entity->pads;
        entity->pads = nullptr;
        delete[] entity->links;
        entity->links = nullptr;
        entity = mEntities.erase(entity);
    }
}

MediaEntity* MediaControl::getEntityByName(const char* name) {
    for (auto& entity : mEntities) {
        if (strcmp(name, entity.info.name) == 0) {
            return &entity;
        }
    }

    return nullptr;
}

int MediaControl::getEntityIdByName(const char* name) {
    MediaEntity* entity = getEntityByName(name);
    if (!entity) {
        return -1;
    }

    return entity->info.id;
}

int MediaControl::resetAllLinks() {
    for (auto& entity : mEntities) {
        for (uint32_t j = 0; j < entity.numLinks; j++) {
            MediaLink* link = &entity.links[j];

            if (link->flags & MEDIA_LNK_FL_IMMUTABLE ||
                link->source->entity->info.id != entity.info.id) {
                continue;
            }
            int ret = setupLink(link->source, link->sink, link->flags & ~MEDIA_LNK_FL_ENABLED);

            if (ret < 0) return ret;
        }
    }

    return 0;
}

int MediaControl::SetRouting(int fd, v4l2_subdev_route* routes, uint32_t numRoutes) {
    if (!routes) {
        ALOGE("%s: Device node %d routes is nullptr", __func__, fd);
        return -EINVAL;
    }

    v4l2_subdev_routing r = {routes, numRoutes, {0}};

    int ret = ::ioctl(fd, VIDIOC_SUBDEV_S_ROUTING, &r);
    if (ret < 0) {
        ALOGE("%s: Device node %d IOCTL VIDIOC_SUBDEV_S_ROUTING error: %s", __func__, fd,
              strerror(errno));
        return ret;
    }

    return ret;
}

int MediaControl::GetRouting(int fd, v4l2_subdev_route* routes, uint32_t* numRoutes) {
    if (!routes || !numRoutes) {
        ALOGE("%s: Device node %d routes or numRoutes is nullptr", __func__, fd);
        return -EINVAL;
    }

    v4l2_subdev_routing r = {routes, *numRoutes, {0}};

    int ret = ::ioctl(fd, VIDIOC_SUBDEV_G_ROUTING, &r);
    if (ret < 0) {
        ALOGE("%s: Device node %d IOCTL VIDIOC_SUBDEV_G_ROUTING error: %s", __func__, fd,
              strerror(errno));
        return ret;
    }

    *numRoutes = r.num_routes;

    return ret;
}

int MediaControl::resetAllRoutes() {
    for (MediaEntity& entity : mEntities) {
        struct v4l2_subdev_route routes[entity.info.pads];
        uint32_t numRoutes = entity.info.pads;

        string subDeviceNodeName;
        subDeviceNodeName.clear();
        getDeviceName(entity.info.name, subDeviceNodeName, true);
        if (subDeviceNodeName.find("/dev/") == std::string::npos) {
            continue;
        }

        int fd = ::open(subDeviceNodeName.c_str(), O_RDWR);
        int ret = GetRouting(fd, routes, &numRoutes);
        if (ret != 0) {
            close(fd);
            continue;
        }

        for (uint32_t j = 0; j < numRoutes; j++) {
            routes[j].flags &= ~V4L2_SUBDEV_ROUTE_FL_ACTIVE;
        }

        ret = SetRouting(fd, routes, numRoutes);
        if (ret < 0) {
            ALOGW("@%s, setRouting ret:%d", __func__, ret);
        }
        close(fd);
    }

    return 0;
}

int MediaControl::setupLink(MediaPad* source, MediaPad* sink, uint32_t flags) {
    MediaLink* link = nullptr;
    media_link_desc ulink;
    uint32_t i;
    int ret = 0;

    SysCall* sc = SysCall::getInstance();

    int fd = openDevice();
    if (fd < 0) goto done;

    for (i = 0; i < source->entity->numLinks; i++) {
        link = &source->entity->links[i];

        if (link->source->entity == source->entity && link->source->index == source->index &&
            link->sink->entity == sink->entity && link->sink->index == sink->index)
            break;
    }

    if (i == source->entity->numLinks) {
        ALOGE("%s: Link not found", __func__);
        ret = -ENOENT;
        goto done;
    }

    /* source pad */
    memset(&ulink, 0, sizeof(media_link_desc));
    ulink.source.entity = source->entity->info.id;
    ulink.source.index = source->index;
    ulink.source.flags = MEDIA_PAD_FL_SOURCE;

    /* sink pad */
    ulink.sink.entity = sink->entity->info.id;
    ulink.sink.index = sink->index;
    ulink.sink.flags = MEDIA_PAD_FL_SINK;

    if (link) ulink.flags = flags | (link->flags & MEDIA_LNK_FL_IMMUTABLE);

    ret = sc->ioctl(fd, MEDIA_IOC_SETUP_LINK, &ulink);
    if (ret == -1) {
        ret = -errno;
        ALOGE("Unable to setup link (%s)", strerror(errno));
        goto done;
    }

    if (link) {
        link->flags = ulink.flags;
        link->twin->flags = ulink.flags;
    }

    ret = 0;

done:
    closeDevice(fd);
    return ret;
}

int MediaControl::setupLink(uint32_t srcEntity, uint32_t srcPad, uint32_t sinkEntity,
                            uint32_t sinkPad, bool enable) {
    ALOGD("@%s srcEntity %d srcPad %d sinkEntity %d sinkPad %d enable %d", __func__, srcEntity,
          srcPad, sinkEntity, sinkPad, enable);

    for (auto& entity : mEntities) {
        for (uint32_t j = 0; j < entity.numLinks; j++) {
            MediaLink* link = &entity.links[j];

            if ((link->source->entity->info.id == srcEntity) && (link->source->index == srcPad) &&
                (link->sink->entity->info.id == sinkEntity) && (link->sink->index == sinkPad)) {
                if (enable)
                    link->flags |= MEDIA_LNK_FL_ENABLED;
                else
                    link->flags &= ~MEDIA_LNK_FL_ENABLED;

                return setupLink(link->source, link->sink, link->flags);
            }
        }
    }

    return -1;
}

int MediaControl::openDevice() {
    int fd;
    SysCall* sc = SysCall::getInstance();

    fd = sc->open(mDevName.c_str(), O_RDWR);
    if (fd < 0) {
        ALOGE("Failed to open media device %s: %s", mDevName.c_str(), strerror(errno));
        return -1;
    }

    return fd;
}

void MediaControl::closeDevice(int fd) {
    if (fd < 0) return;

    SysCall* sc = SysCall::getInstance();

    if (sc->close(fd) < 0) {
        ALOGE("Failed to close media device %s: %s", mDevName.c_str(), strerror(errno));
    }
}

int MediaControl::enumInfo() {
    SysCall* sc = SysCall::getInstance();

    if (mEntities.size() > 0) return 0;

    int fd = openDevice();
    if (fd < 0) {
        ALOGE("Open device failed.");
        return fd;
    }

    media_device_info info;
    int ret = sc->ioctl(fd, MEDIA_IOC_DEVICE_INFO, &info);
    if (ret < 0) {
        ALOGE("Unable to retrieve media device information for device %s (%s)", mDevName.c_str(),
              strerror(errno));
        goto done;
    }

    ret = enumEntities(fd);
    if (ret < 0) {
        ALOGE("Unable to enumerate entities for device %s", mDevName.c_str());
        goto done;
    }

    ALOGD("Found %lu entities, enumerating pads and links", mEntities.size());

    ret = enumLinks(fd);
    if (ret < 0) {
        ALOGE("Unable to enumerate pads and linksfor device %s", mDevName.c_str());
        goto done;
    }

    ret = 0;

done:
    closeDevice(fd);
    return ret;
}

int MediaControl::enumEntities(int fd) {
    MediaEntity entity;
    uint32_t id;
    int ret;

    SysCall* sc = SysCall::getInstance();

    for (id = 0, ret = 0;; id = entity.info.id) {
        memset(&entity, 0, sizeof(MediaEntity));
        entity.info.id = id | MEDIA_ENT_ID_FLAG_NEXT;

        ret = sc->ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity.info);
        if (ret < 0) {
            ret = errno != EINVAL ? -errno : 0;
            break;
        }

        /* Number of links (for outbound links) plus number of pads (for
         * inbound links) is a good safe initial estimate of the total
         * number of links.
         */
        entity.maxLinks = entity.info.pads + entity.info.links;

        entity.pads = new MediaPad[entity.info.pads];
        entity.links = new MediaLink[entity.maxLinks];
        getDevnameFromSysfs(&entity);
        mEntities.push_back(entity);

        /* Note: carefully to move the follow setting. It must be behind of
         * push_back to mEntities:
         * 1. if entity is not pushed back to mEntities, getEntityById will
         * return NULL.
         * 2. we can't set entity.pads[i].entity to &entity direct. Because,
         * entity is stack variable, its scope is just this function.
         */
        for (uint32_t i = 0; i < entity.info.pads; ++i) {
            entity.pads[i].entity = getEntityById(entity.info.id);
        }
    }

    return ret;
}

int MediaControl::getDevnameFromSysfs(MediaEntity* entity) {
    char sysName[MAX_SYS_NAME] = {'\0'};
    char target[MAX_TARGET_NAME] = {'\0'};
    int ret;

    if (!entity) {
        ALOGE("entity is null.");
        return -EINVAL;
    }

    ret = snprintf(sysName, MAX_SYS_NAME, "/sys/dev/char/%u:%u", entity->info.v4l.major,
                   entity->info.v4l.minor);
    if (ret <= 0) {
        ALOGE("create sysName failed ret %d.", ret);
        return -EINVAL;
    }

    ret = readlink(sysName, target, MAX_TARGET_NAME);
    if (ret <= 0) {
        ALOGE("readlink sysName %s failed ret %d.", sysName, ret);
        return -EINVAL;
    }

    char* d = strrchr(target, '/');
    if (!d) {
        ALOGE("target is invalid %s.", target);
        return -EINVAL;
    }
    d++; /* skip '/' */

    char* t = strstr(d, "dvb");
    if (t && t == d) {
        t = strchr(t, '.');
        if (!t) {
            ALOGE("target is invalid %s.", target);
            return -EINVAL;
        }
        *t = '/';
        d += 3; /* skip "dvb" */
        snprintf(entity->devname, sizeof(entity->devname), "/dev/dvb/adapter%s", d);
    } else {
        snprintf(entity->devname, sizeof(entity->devname), "/dev/%s", d);
    }

    return 0;
}

int MediaControl::enumLinks(int fd) {
    int ret = 0;

    SysCall* sc = SysCall::getInstance();

    for (auto& entity : mEntities) {
        media_links_enum links;
        uint32_t i;

        links.entity = entity.info.id;
        links.pads = new media_pad_desc[entity.info.pads];
        links.links = new media_link_desc[entity.info.links];

        if (sc->ioctl(fd, MEDIA_IOC_ENUM_LINKS, &links) < 0) {
            ret = -errno;
            ALOGE("Unable to enumerate pads and links (%s).", strerror(errno));
            delete[] links.pads;
            delete[] links.links;
            return ret;
        }

        for (i = 0; i < entity.info.pads; ++i) {
            entity.pads[i].entity = getEntityById(entity.info.id);
            entity.pads[i].index = links.pads[i].index;
            entity.pads[i].flags = links.pads[i].flags;
        }

        for (i = 0; i < entity.info.links; ++i) {
            media_link_desc* link = &links.links[i];
            MediaLink* fwdlink;
            MediaLink* backlink;
            MediaEntity* source;
            MediaEntity* sink;

            source = getEntityById(link->source.entity);
            sink = getEntityById(link->sink.entity);

            if (source == nullptr || sink == nullptr) {
                ALOGE("WARNING entity %u link %u src %u/%u to %u/%u is invalid!", entity.info.id, i,
                      link->source.entity, link->source.index, link->sink.entity, link->sink.index);
                ret = -EINVAL;
            } else {
                fwdlink = entityAddLink(source);
                if (fwdlink) {
                    fwdlink->source = &source->pads[link->source.index];
                    fwdlink->sink = &sink->pads[link->sink.index];
                    fwdlink->flags = link->flags;
                }

                backlink = entityAddLink(sink);
                if (backlink) {
                    backlink->source = &source->pads[link->source.index];
                    backlink->sink = &sink->pads[link->sink.index];
                    backlink->flags = link->flags;
                }

                if (fwdlink) fwdlink->twin = backlink;
                if (backlink) backlink->twin = fwdlink;
            }
        }

        delete[] links.pads;
        delete[] links.links;
    }

    return ret;
}

MediaLink* MediaControl::entityAddLink(MediaEntity* entity) {
    if (entity->numLinks >= entity->maxLinks) {
        uint32_t maxLinks = entity->maxLinks * 2;
        MediaLink* links = new MediaLink[maxLinks];

        memcpy(links, entity->links, sizeof(MediaLink) * entity->maxLinks);
        delete[] entity->links;

        for (uint32_t i = 0; i < entity->numLinks; ++i) {
            links[i].twin->twin = &links[i];
        }

        entity->maxLinks = maxLinks;
        entity->links = links;
    }

    return &entity->links[entity->numLinks++];
}

MediaEntity* MediaControl::getEntityById(uint32_t id) {
    bool next = id & MEDIA_ENT_ID_FLAG_NEXT;

    id &= ~MEDIA_ENT_ID_FLAG_NEXT;

    for (uint32_t i = 0; i < mEntities.size(); i++) {
        if ((mEntities[i].info.id == id && !next) || (mEntities[0].info.id > id && next)) {
            return &mEntities[i];
        }
    }

    return nullptr;
}
