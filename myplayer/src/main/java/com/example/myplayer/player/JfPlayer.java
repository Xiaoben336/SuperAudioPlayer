package com.example.myplayer.player;

import android.support.v4.app.NavUtils;
import android.text.TextUtils;

import com.example.myplayer.bean.JfTimeInfoBean;
import com.example.myplayer.listener.JfOnCompleteListener;
import com.example.myplayer.listener.JfOnErrorListener;
import com.example.myplayer.listener.JfOnLoadingListener;
import com.example.myplayer.listener.JfOnPauseResumeListener;
import com.example.myplayer.listener.JfOnPreparedListener;
import com.example.myplayer.listener.JfOnTimeInfoListener;
import com.example.myplayer.log.JfLog;

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

	private static JfTimeInfoBean jfTimeInfoBean;
	public JfPlayer(){

	}

	/**
	 * 设置数据源
	 * @param source
	 */
	public void setSource(String source) {
		this.source = source;
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
		//onCallLoading(true);//加载状态

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
		//在C++层有一个while循环，可能会耗时较长
		new Thread(new Runnable() {
			@Override
			public void run() {
				n_stop();
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

	private native void n_prepared(String source);
	private native void n_start();
	private native void n_pause();
	private native void n_resume();
	private native void n_stop();
	private native void n_seek(int sec);
}
