extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/pixfmt.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/frame.h>
    #include <libavutil/opt.h>
    #include <libavutil/dict.h>
}
#define DEFAULT_NB_SAMPLE 2048

#include <iostream>

using namespace std;

int main()
{
    char *file_path = "D:\\Amthem.mp4";

    AVFormatContext *pFormatCtx;

    AVCodecContext *pAudioCodecCtx;
    AVCodec *pAudioCodec;
    AVFrame *pAudioFrame;
    AVPacket *audiopackt;
    uint8_t **out_Audiobuffer;

    int audioStream;
    int sample_rate;
    static struct SwrContext *audio_convert_ctx;
    bool convert_spe = false; //audioを変換する必要ありますか？
    bool isGetInfoOnly; //audioの情報を読み込みます。タイトル、歌手、アルバム、,作詞,歌詞,,,
    int LineSize;
    int readSize;

    int64_t Sample_size,MAX_Sample_size;
    uint64_t out_audioChanels, audioChanel_out;

    int got_Frame = 0;
    int msleep = 0;
    bool sendStartPlay = false;

    int ret;
    sample_rate = 44100;
    audioChanel_out = AV_CH_LAYOUT_STEREO;//

    av_register_all(); //FFMPEGは、これを使用してアプリケーションを初期化します。
    cout << "Hello FFmpeg!" << endl;
    unsigned version = avcodec_version();
    cout << "version is:" << version << endl;

    //AVFormatContextを割り当てます.
    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx,file_path,NULL,NULL) != 0)
    {
        cout << "Don't read file" << endl;
        return -1;
    }

    cout << "Read file" << endl;

    if(avformat_find_stream_info(pFormatCtx,NULL) < 0)
    {
        cout << "Can't find stream infomation" << endl;
        return -1;
    }

    cout << "find stream infomation" << endl;

    audioStream = -1;



    //ビデオのストリームを見つかるまで探索します。
    //videoStreamに保存されます
    //ここでは、ビデオストリームだけを扱います.
    for(int i=0;i < pFormatCtx->nb_streams;i++)
    {

        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;
        }
    }

    cout << "audio Stream is " << audioStream << endl;

    if(audioStream == -1)
    {
        cout << "Don't find audio stream " << endl;
        return -1;
    }

    cout << "find stream " << endl;

    pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;
    pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);

    if(pAudioCodec == NULL)
    {
        cout << "not find Audio codec " << endl;
        return -1;
    }

    cout << "Audio codec find " << endl;

    //オーディオデコーダーを開く
    if(avcodec_open2(pAudioCodecCtx,pAudioCodec,NULL)<0)
    {
        cout << "not open Audio codec " << endl;
        return -1;
    }
    cout << "Audio codec open " << endl;


    audiopackt = new AVPacket;
    av_init_packet(audiopackt);
    pAudioFrame = av_frame_alloc();

    //audioの変換コンテキスト作成
    if(pAudioCodecCtx->sample_fmt != AV_SAMPLE_FMT_S16){
        audio_convert_ctx = swr_alloc_set_opts(NULL,audioChanel_out,
                                               AV_SAMPLE_FMT_S16,sample_rate,
                                               pAudioCodecCtx->channel_layout,pAudioCodecCtx->sample_fmt,
                                               pAudioCodecCtx->sample_rate,0,NULL);
        if(swr_init(audio_convert_ctx)<0){
            cout << "swr init Failed " << endl;
        }
        cout << "swr init  " << endl;

        convert_spe =true;
        out_audioChanels = av_get_channel_layout_nb_channels(pAudioCodecCtx->channel_layout);
        Sample_size = av_rescale_rnd(DEFAULT_NB_SAMPLE,sample_rate,
                                     pAudioCodecCtx->sample_rate,
                                     AV_ROUND_UP);
        MAX_Sample_size = Sample_size;
        ret = av_samples_alloc_array_and_samples(&out_Audiobuffer,&LineSize,out_audioChanels,
                                                 DEFAULT_NB_SAMPLE,AV_SAMPLE_FMT_S16,1);
        if(ret < 0){
            cout << "audio buffer not create" << endl;
        }
        cout << "audio buffer create" << endl;
    }
    else
    {
        convert_spe = false;
    }



    FILE *CacheFile;
    char *CacheFilename = "CacheFile.raw";
    CacheFile = fopen(CacheFilename,"wb");

    if(CacheFile==NULL){
        cout << "Can't open Cache File" << endl;
        return -1;
    }
    cout << "open Cache File" << endl;
    cout << "start" << endl;
    cout << "______________________________" << endl;
    while (1) {

        if(av_read_frame(pFormatCtx,audiopackt) < 0){
            break;
        }

        if(audiopackt->stream_index ==audioStream)
        {

                readSize = avcodec_decode_audio4(pAudioCodecCtx,pAudioFrame,&got_Frame,audiopackt);
                if(readSize < 0){
                    cout << "length <0 .skip this frame" << endl;
                    break;
                }
                if(got_Frame){
                    if(convert_spe){
                        Sample_size = av_rescale_rnd(pAudioFrame->nb_samples,
                                    sample_rate,pAudioFrame->sample_rate,AV_ROUND_UP);
                        if(Sample_size > MAX_Sample_size){
                            //out_Audiobuffer初期化
                            av_freep(&out_Audiobuffer[0]);
                            ret = av_samples_alloc(out_Audiobuffer,&LineSize,out_audioChanels
                                                   ,Sample_size,AV_SAMPLE_FMT_S16,1);
                            if(ret < 0){
                                cout << "faild allocate" << endl;
                                break;
                            }
                            MAX_Sample_size = Sample_size;
                        }
                        //変換する
                        ret = swr_convert(audio_convert_ctx,out_Audiobuffer,Sample_size
                                          ,(const uint8_t **)pAudioFrame->data,pAudioFrame->nb_samples);

                        if(ret < 0){
                            cout << "faild allocate" << endl;
                            break;
                        }
                        int buf_size = av_samples_get_buffer_size(&LineSize,out_audioChanels,
                                                                  Sample_size,AV_SAMPLE_FMT_S16,1);

                        if(buf_size < 0){
                            cout << "Error while converting audio" << endl;
                            break;
                        }
                        fwrite(out_Audiobuffer[0],1,buf_size,CacheFile);
                    }

                    //変換しない
                    else{
                        fwrite(pAudioFrame->data[0],1,pAudioFrame->linesize[0],CacheFile);
                    }
                av_free_packet(audiopackt);
            }
        }
    }

    cout << "Finish" << endl;
    fclose(CacheFile);
    av_free(out_Audiobuffer);
    //av_free(pVideoFrameRGB);
    avcodec_close(pAudioCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}
