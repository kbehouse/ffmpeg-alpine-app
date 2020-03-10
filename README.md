# ffmpeg APP using alpine  

Thanks for inspired repo: https://github.com/leandromoreira/ffmpeg-libav-tutorial

Target: Build an app catch mp4 (or rtsp) save to JPG 

## Build & Run Docker
Build
```
docker build -t ffmpeg-alpine-app .
```

Run with mp4
```

docker run -it -v $PWD:/v  \
-e IMG_FOLDER=/v/img \
-e STREAM_URI=/v/test.mp4  \
ffmpeg-alpine-app 
```

Run with rtsp (IPCam)
```
docker run -it -v $PWD:/v  \
-e IMG_FOLDER=/v/img \
-e STREAM_URI=rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov  \
ffmpeg-alpine-app 
```

## Test app easily

Build for build image
```
docker build -t ffmpeg-builder -f Dockfile-build .
```

```
make app
```
