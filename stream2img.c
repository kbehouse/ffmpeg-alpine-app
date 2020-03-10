/*
 * http://ffmpeg.org/doxygen/trunk/index.html
 *
 * Main components
 *
 * Format (Container) - a wrapper, providing sync, metadata and muxing for the streams.
 * Stream - a continuous stream (audio or video) of data over time.
 * Codec - defines how data are enCOded (from Frame to Packet)
 *        and DECoded (from Packet to Frame).
 * Packet - are the data (kind of slices of the stream data) to be decoded as raw frames.
 * Frame - a decoded raw frame (to be encoded or filtered).
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h> 
#include "bmp.h"
#include "util.h"

// print out the steps and errors
static void logging(const char *fmt, ...);
// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// #define PIX_FMT_RGB24 AV_PIX_FMT_RGB24
// #define avcodec_alloc_frame av_frame_alloc
// #define PIX_FMT_RGB24 AV_PIX_FMT_RGB24
// #define PIX_FMT_YUV422P AV_PIX_FMT_YUV422P



char jpgFilename[256];
char bmpFilename[256];
int fpsCount = 0;
struct timeval tvStart;

struct SwsContext *pSwsCtx;
AVFrame *pFrameBGR;

// read from env
char* env_img_folder = "cam";
char* env_stream_uri = NULL;

void parse_env(){
	env_stream_uri = getenv("STREAM_URI");
	env_img_folder = getenv("IMG_FOLDER");
}

void bmp_init( AVCodecContext *pCodecContext){
    pFrameBGR = av_frame_alloc();
	if (!pFrameBGR){
		logging("failed to allocated memory for pFrameBGR");
		return ;
	}
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, pCodecContext->width, pCodecContext->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(pFrameBGR->data, pFrameBGR->linesize, buffer, AV_PIX_FMT_BGR24,
        pCodecContext->width, pCodecContext->height, 1);

    pSwsCtx = sws_getContext(pCodecContext->width, pCodecContext->height,
        pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, NULL, NULL, NULL);
}



int main(int argc, const char *argv[])
{
	parse_env();
	createFolderIfNotExist(env_img_folder);

	logging("stream uri:%s, img folder:%s\n", env_stream_uri, env_img_folder);

	av_log_set_level(AV_LOG_ERROR);
	logging("initializing all the containers, codecs and protocols.");

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	AVFormatContext *pFormatContext = avformat_alloc_context();
	if (!pFormatContext) {
		logging("ERROR could not allocate memory for Format Context");
		return -1;
	}

	logging("opening the input file (%s) and loading format (container) header", argv[1]);
	// Open the file and read its header. The codecs are not opened.
	// The function arguments are:
	// AVFormatContext (the component we allocated memory for),
	// url (filename),
	// AVInputFormat (if you pass NULL it'll do the auto detect)
	// and AVDictionary (which are options to the demuxer)
	// http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
	if (avformat_open_input(&pFormatContext, env_stream_uri, NULL, NULL) != 0) {
		logging("ERROR could not open the file");
		return -1;
	}

	// now we have access to some information about our file
	// since we read its header we can say what format (container) it's
	// and some other information related to the format itself.
	logging("format %s, duration %lld us, bit_rate %lld", pFormatContext->iformat->name, pFormatContext->duration, pFormatContext->bit_rate);

	logging("finding stream info from format");
	// read Packets from the Format to get stream information
	// this function populates pFormatContext->streams
	// (of size equals to pFormatContext->nb_streams)
	// the arguments are:
	// the AVFormatContext
	// and options contains options for codec corresponding to i-th stream.
	// On return each dictionary will be filled with options that were not found.
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
	if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
		logging("ERROR could not get the stream info");
		return -1;
	}

	// the component that knows how to enCOde and DECode the stream
	// it's the codec (audio or video)
	// http://ffmpeg.org/doxygen/trunk/structAVCodec.html
	AVCodec *pCodec = NULL;
	// this component describes the properties of a codec used by the stream i
	// https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
	AVCodecParameters *pCodecParameters =  NULL;
	int video_stream_index = -1;

	logging("pFormatContext->nb_streams = %d", pFormatContext->nb_streams);
	// loop though all the streams and print its main information
	for (int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters *pLocalCodecParameters =  NULL;
		pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
		logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
		logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
		logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
		logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

		logging("finding the proper decoder (CODEC)");

		AVCodec *pLocalCodec = NULL;

		// finds the registered decoder for a codec ID
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
		logging("pLocalCodecParameters->codec_id = %d", pLocalCodecParameters->codec_id);
		pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

		if (pLocalCodec==NULL) {
			logging("ERROR unsupported codec!");
			return -1;
		}

		// when the stream is a video we store its index, codec parameters and codec
		if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (video_stream_index == -1) {
				video_stream_index = i;
				pCodec = pLocalCodec;
				pCodecParameters = pLocalCodecParameters;
			}

			logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
		} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
			logging("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
		}

		// print its name, id and bitrate
		logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pCodecParameters->bit_rate);
	}
	// https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
	AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
	if (!pCodecContext)
	{
		logging("failed to allocated memory for AVCodecContext");
		return -1;
	}

	// Fill the codec context based on the values from the supplied codec parameters
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
	if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
	{
		logging("failed to copy codec params to codec context");
		return -1;
	}

	// Initialize the AVCodecContext to use the given AVCodec.
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
	if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
	{
		logging("failed to open codec through avcodec_open2");
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVFrame.html
	AVFrame *pFrame = av_frame_alloc();
	if (!pFrame)
	{
		logging("failed to allocated memory for AVFrame");
		return -1;
	}
	
	// https://ffmpeg.org/doxygen/trunk/structAVPacket.html
	AVPacket *pPacket = av_packet_alloc();
	if (!pPacket)
	{
		logging("failed to allocated memory for AVPacket");
		return -1;
	}

	bmp_init(pCodecContext);
	int response = 0;
	int how_many_packets_to_process = 5 * 200;
	
	gettimeofday(&tvStart, NULL);
	while (av_read_frame(pFormatContext, pPacket) >= 0)
	{
		// if it's the video stream
		if (pPacket->stream_index == video_stream_index) {
			// logging("AVPacket->pts %" PRId64, pPacket->pts);
			// if (how_many_packets_to_process % 10 ==0){
			// 	logging("how_many_packets_to_process =  %d", how_many_packets_to_process);
			// 	// showStatusVm();
			// 	// showPsaux();
			// }
			response = decode_packet(pPacket, pCodecContext, pFrame);
			if (response < 0){
				logging("[Error] response = %d (<0) -> break while", response);
				break;
			}
				
			// stop it, otherwise we'll be saving hundreds of frames
			// if (--how_many_packets_to_process <= 0) break;
		}
		// https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
		av_packet_unref(pPacket);
	}

	logging("releasing all the resources");

	avformat_close_input(&pFormatContext);
	avformat_free_context(pFormatContext);
	av_packet_free(&pPacket);
	av_frame_free(&pFrame);
	avcodec_free_context(&pCodecContext);

	// rmFolder(env_img_folder);

	return 0;
}

static void logging(const char *fmt, ...)
{
		va_list args;
		fprintf( stderr, "LOG: " );
		va_start( args, fmt );
		vfprintf( stderr, fmt, args );
		va_end( args );
		fprintf( stderr, "\n" );
}

int saveJpg(AVFrame *pFrame, char *out_name) {
    
    int width = pFrame->width;
    int height = pFrame->height;
    AVCodecContext *pCodeCtx = NULL;
    
    
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // 設定輸出檔案格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    // 建立並初始化輸出AVIOContext
    if (avio_open(&pFormatCtx->pb, out_name, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Couldn't open output file.");
        return -1;
    }

    // 構建一個新stream
    AVStream *pAVStream = avformat_new_stream(pFormatCtx, 0);
    if (pAVStream == NULL) {
        return -1;
    }

    AVCodecParameters *parameters = pAVStream->codecpar;
    parameters->codec_id = pFormatCtx->oformat->video_codec;
    parameters->codec_type = AVMEDIA_TYPE_VIDEO;
    parameters->format = AV_PIX_FMT_YUVJ420P;
    parameters->width = pFrame->width;
    parameters->height = pFrame->height;

    AVCodec *pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);

    if (!pCodec) {
        printf("Could not find encoder\n");
        return -1;
    }

    pCodeCtx = avcodec_alloc_context3(pCodec);
    if (!pCodeCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return -1;
    }

    pCodeCtx->time_base = (AVRational) {1, 25};

    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.");
        return -1;
    }

    int ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        printf("write_header fail\n");
        return -1;
    }

    int y_size = width * height;

    //Encode
    // 給AVPacket分配足夠大的空間
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);

    // 編碼資料
    ret = avcodec_send_frame(pCodeCtx, pFrame);
    if (ret < 0) {
        printf("Could not avcodec_send_frame.");
        return -1;
    }

    // 得到編碼後資料
    ret = avcodec_receive_packet(pCodeCtx, &pkt);
    if (ret < 0) {
        printf("Could not avcodec_receive_packet");
        return -1;
    }

    ret = av_write_frame(pFormatCtx, &pkt);

    if (ret < 0) {
        printf("Could not av_write_frame");
        return -1;
    }

    av_packet_unref(&pkt);

    //Write Trailer
    av_write_trailer(pFormatCtx);


    avcodec_close(pCodeCtx);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    return 0;
}



void framePostProcess(AVCodecContext *pCodecContext, AVFrame *pFrame){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long int ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	// JPG, ok 
	snprintf(jpgFilename, sizeof(jpgFilename), "%s/%ld.jpg", env_img_folder, ms);
	saveJpg(pFrame, jpgFilename); //儲存為jpg圖片

	// bmp_generator(bmpFilename, pCodecContext->width, pCodecContext->height, pFrameBGR->data[0]);
	
	// save to bmp
	sws_scale(pSwsCtx, pFrame->data,
		pFrame->linesize, 0, pCodecContext->height,
		pFrameBGR->data, pFrameBGR->linesize);
	
	snprintf(bmpFilename, sizeof(bmpFilename),  "%s/%ld.bmp", env_img_folder, ms);
	// logging("Save to %s", bmpFilename);
	// logging("framerate num: %d / den: %d", pCodecContext->framerate.num, pCodecContext->framerate.den);
	// SaveBitmap(bmpFilename, pFrameBGR->data[0],pCodecContext->width, pCodecContext->height);

	fpsCount++;
	if (tv.tv_sec > tvStart.tv_sec) {
		double interval = (tv.tv_sec - tvStart.tv_sec) + (tv.tv_usec - tvStart.tv_usec) / 1000000 ;
		float fps = fpsCount / interval; 
		logging("FPS: %.2lf", fps);
		fpsCount = 0;
		gettimeofday(&tvStart, NULL);
	}
}
	// struct timeval tvStart;
	// gettimeofday(&tvStart, NULL);
	// long int  start = tvStart.tv_sec * 1000 + tvStart.tv_usec / 1000;
	


static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
	 
	// Supply raw packet data as input to a decoder
	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
	int response = avcodec_send_packet(pCodecContext, pPacket);

	if (response < 0) {
		logging("Error while sending a packet to the decoder: %s", av_err2str(response));
		return response;
	}

	while (response >= 0)
	{
		// Return decoded output data (into a frame) from a decoder
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
		response = avcodec_receive_frame(pCodecContext, pFrame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if (response < 0) {
			logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
			return response;
		}

		if (response >= 0) {
			// logging(
			// 		"Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
			// 		pCodecContext->frame_number,
			// 		av_get_picture_type_char(pFrame->pict_type),
			// 		pFrame->pkt_size,
			// 		pFrame->pts,
			// 		pFrame->key_frame,
			// 		pFrame->coded_picture_number
			// );
			framePostProcess(pCodecContext, pFrame);
			
		}
	}
	return 0;
}