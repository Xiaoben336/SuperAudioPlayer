package com.example.myplayer.player;


import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.support.annotation.RequiresApi;
import android.text.TextUtils;
import android.view.Surface;

import com.example.myplayer.bean.JfTimeInfoBean;
import com.example.myplayer.listener.JfOnCompleteListener;
import com.example.myplayer.listener.JfOnErrorListener;
import com.example.myplayer.listener.JfOnLoadingListener;
import com.example.myplayer.listener.JfOnPauseResumeListener;
import com.example.myplayer.listener.JfOnPreparedListener;
import com.example.myplayer.listener.JfOnTimeInfoListener;
import com.example.myplayer.log.JfLog;
import com.example.myplayer.opengl.JfGLSurfaceView;
import com.example.myplayer.opengl.JfRender;
import com.example.myplayer.util.JfVideoSupUtil;

import java.io.IOException;
import java.nio.ByteBuffer;


public class JfPlayer {
	static {
		System.loadLibrary("avutil-55");
		System.loadLibrary("avcodec-57");
		System.loadLibrary("avformat-57");
		System.loadLibrary("avdevice-57");
		System.loadLibrary("swresample-2");
		System.loadLibrary("swscale-4");
		System.loadLibrary("postproc-54");
		System.loadLibrary("avfilter-6");
		System.loadLibrary("native-lib");
	}

	private static String source;
	private static boolean playNext = false;

	private JfOnPreparedListener jfOnPreparedListener;
	private JfOnLoadingListener jfOnLoadingListener;
	private JfOnPauseResumeListener jfOnPauseResumeListener;
	private JfOnTimeInfoListener jfOnTimeInfoListener;
	private JfOnErrorListener jfOnErrorListener;
	private JfOnCompleteListener jfOnCompleteListener;
	private JfGLSurfaceView jfGLSurfaceView;

	private int duration;//总时长


	private MediaFormat mediaFormat;
	private MediaCodec mediaCodec;
	private Surface surface;
	private MediaCodec.BufferInfo info;
	private static JfTimeInfoBean jfTimeInfoBean;
	public JfPlayer(){

	}


	public int getDuration() {
		return duration;
	}

	/**
	 * 设置数据源
	 * @param source
	 */
	public void setSource(String source) {
		this.source = source;
	}

	public void setJfGLSurfaceView(JfGLSurfaceView jfGLSurfaceView) {
		this.jfGLSurfaceView = jfGLSurfaceView;

		jfGLSurfaceView.getJfRender().setOnSurfaceCreateListener(new JfRender.OnSurfaceCreateListener() {
			@Override
			public void onSurfaceCreate(Surface s) {
				if (surface == null) {
					surface = s;
					JfLog.d("onSurfaceCreate");
				}
			}
		});
	}

	public void setJfOnPreparedListener(JfOnPreparedListener jfOnPreparedListener) {
		this.jfOnPreparedListener = jfOnPreparedListener;
	}

	public void setJfOnLoadingListener(JfOnLoadingListener jfOnLoadingListener) {
		this.jfOnLoadingListener = jfOnLoadingListener;
	}

	public void setJfOnPauseResumeListener(JfOnPauseResumeListener jfOnPauseResumeListener) {
		this.jfOnPauseResumeListener = jfOnPauseResumeListener;
	}

	public void setJfOnTimeInfoListener(JfOnTimeInfoListener jfOnTimeInfoListener) {
		this.jfOnTimeInfoListener = jfOnTimeInfoListener;
	}

	public void setJfOnErrorListener(JfOnErrorListener jfOnErrorListener) {
		this.jfOnErrorListener = jfOnErrorListener;
	}

	public void setJfOnCompleteListener(JfOnCompleteListener jfOnCompleteListener) {
		this.jfOnCompleteListener = jfOnCompleteListener;
	}

	public void prepared(){
		if (TextUtils.isEmpty(source)){
			JfLog.w("SOURCE IS EMPTY");
			return;
		}

		new Thread(new Runnable() {
			@Override
			public void run() {
				n_prepared(source);
			}
		}).start();
	}


	public void start(){
		if (TextUtils.isEmpty(source)){
			JfLog.w("SOURCE IS EMPTY");
			return;
		}

		new Thread(new Runnable() {
			@Override
			public void run() {
				n_start();
			}
		}).start();
	}


	/**
	 * 暂停播放
	 */
	public void pause(){
		n_pause();
		if (jfOnPauseResumeListener != null) {
			jfOnPauseResumeListener.onPause(true);
		}
	}

	/**
	 * 继续播放
	 */
	public void resume(){
		n_resume();
		if (jfOnPauseResumeListener != null) {
			jfOnPauseResumeListener.onPause(false);
		}
	}

	/**
	 * 停止播放
	 */
	public void stop(){
		jfTimeInfoBean = null;
		duration = 0;
		//在C++层有一个while循环，可能会耗时较长
		new Thread(new Runnable() {
			@Override
			public void run() {
				n_stop();
				releaseMediaCodec();
			}
		}).start();
	}

	/**
	 * 快进快退播放
	 * @param sec
	 */
	public void seek(int sec){
		n_seek(sec);
	}


	public void playNest(String nextUrl){
		source = nextUrl;
		playNext = true;
		stop();
	}



	/**
	 * C++层n_prepare()完成后要调用JfOnPreparedListener
	 */
	public void onCallPrepared(){
		if (jfOnPreparedListener != null)
		{
			jfOnPreparedListener.onPrepared();
		}
	}

	/**
	 *加载回调
	 * @param loading
	 */
	public void onCallLoading(boolean loading){
		if (jfOnLoadingListener != null) {
			jfOnLoadingListener.onLoading(loading);
		}
	}

	/**
	 * 得到音频总时长和当前时间的回调
	 * @param currentTime
	 * @param totalTime
	 */
	public void onCallTimeInfo(int currentTime,int totalTime){
		if (jfOnTimeInfoListener != null) {
			if (jfTimeInfoBean == null) {
				jfTimeInfoBean = new JfTimeInfoBean();
			}
			duration = totalTime;
			jfTimeInfoBean.setCurrentTime(currentTime);
			jfTimeInfoBean.setTotalTime(totalTime);
			jfOnTimeInfoListener.onTimeInfo(jfTimeInfoBean);
		}
	}

	/**
	 * 报错回调
	 * @param code
	 * @param msg
	 */
	public void onCallError(int code,String msg){
		stop();
		if (jfOnErrorListener != null) {
			jfOnErrorListener.onError(code,msg);
		}
	}

	/**
	 * 播放完成时回调
	 */
	public void onCallComplete(){
		stop();
		if (jfOnCompleteListener != null) {
			jfOnCompleteListener.onComplete();
		}
	}


	/**
	 * 播放下一曲回调
	 */
	public void onCallPlayNext(){
		if (playNext){
			playNext = false;
			prepared();
		}
	}


	/**
	 *渲染YUV数据
	 * @param width
	 * @param height
	 * @param y
	 * @param u
	 * @param v
	 */
	public void onCallRenderYUV(int width,int height,byte[] y,byte[] u,byte[] v){
		JfLog.d("获取到视频的数据");
		if (jfGLSurfaceView != null) {
			jfGLSurfaceView.getJfRender().setRenderType(JfRender.RENDER_YUV);
			jfGLSurfaceView.setYUVData(width, height, y, u, v);
		}
	}

	/**
	 * 是否支持硬解码
	 * @param ffcodecname
	 * @return
	 */
	public boolean onCallIsSupCodec(String ffcodecname){
		JfLog.d("ffcodecname == " + ffcodecname);
		return JfVideoSupUtil.isSupCodec(ffcodecname);
	}


	/**
	 * 初始化MediaCodec
	 * @param codecName 解码器名称：h264、h265...
	 * @param width
	 * @param height
	 * @param csd_0
	 * @param csd_1
	 */
	public void onCallInitMediaCodec(String codecName,int width,int height,byte[] csd_0,byte[] csd_1){
		if (surface != null) {
			try {
				jfGLSurfaceView.getJfRender().setRenderType(JfRender.RENDER_MEDIACODEC);
				String mime = JfVideoSupUtil.findVideoCodecName(codecName);
				mediaFormat = MediaFormat.createVideoFormat(mime,width,height);
				mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,width * height);
				mediaFormat.setByteBuffer("csd-0",ByteBuffer.wrap(csd_0));
				mediaFormat.setByteBuffer("csd-1",ByteBuffer.wrap(csd_1));
				JfLog.d("mediaFormat.toString() === " + mediaFormat.toString());

				mediaCodec = MediaCodec.createDecoderByType(mime);
				info = new MediaCodec.BufferInfo();
				mediaCodec.configure(mediaFormat,surface,null,0);
				mediaCodec.start();
			} catch (IOException e) {
				//e.printStackTrace();
			}
		} else {
			if (jfOnErrorListener != null) {
				jfOnErrorListener.onError(401,"surface is null");
			}
		}
	}


	private void releaseMediaCodec(){
		if (mediaCodec != null) {
			try {
				mediaCodec.flush();
				mediaCodec.stop();
				mediaCodec.release();
			} catch (Exception e) {
				//e.printStackTrace();
			}
			mediaCodec = null;
			mediaFormat = null;
			info = null;
		}
	}

	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	public void onCallDecodeAVPacket(int datasize, byte[] data){
		if (surface != null && datasize > 0 && data != null && mediaCodec != null) {
			try{
				int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);
				if (inputBufferIndex >= 0) {//进队
					//可能有好几个队列，按照索引，把队列中的bytebuffer拿出来
					ByteBuffer byteBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
					byteBuffer.clear();
					byteBuffer.put(data);
					mediaCodec.queueInputBuffer(inputBufferIndex,0,datasize,0,0);
				}

				int outputBufferIndex = mediaCodec.dequeueOutputBuffer(info,10);
				while (outputBufferIndex >= 0) {
					mediaCodec.releaseOutputBuffer(outputBufferIndex,true);
					outputBufferIndex = mediaCodec.dequeueOutputBuffer(info,10);
				}
			}catch (Exception e) {
				//e.printStackTrace();
			}
		}
	}

	private native void n_prepared(String source);
	private native void n_start();
	private native void n_pause();
	private native void n_resume();
	private native void n_stop();
	private native void n_seek(int sec);
}
