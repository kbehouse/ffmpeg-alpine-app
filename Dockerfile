FROM alpine:3.10 AS build
MAINTAINER Kartik, Chen <kbehouse@gmail.com>

WORKDIR /build
COPY . /build
RUN apk upgrade -U \
 && apk add  ffmpeg-dev build-base  \
 && rm -rf /var/cache/*

RUN gcc stream2img.c \
  -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
  -o stream2img


FROM alpine:3.10
RUN apk upgrade -U \
 && apk add  ffmpeg-libs  \
 && apk add bash bash-completion \
 && rm -rf /var/cache/*
COPY --from=build /build/stream2img /stream2img
CMD ["/stream2img"]
