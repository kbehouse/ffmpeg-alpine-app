clean:
	@rm -rf ./app/*

build_app: clean
	docker run -w /build --rm -it  -v `pwd`:/build ffmpeg-builder \
	  gcc stream2img.c \
		-lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
		-o app/stream2img
	  
app: build_app
	docker run -w /app --rm -it -v `pwd`/app:/app \
	-e IMG_FOLDER=/app/img \
	-e STREAM_URI=rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov \
	ffmpeg-builder /app/stream2img 
