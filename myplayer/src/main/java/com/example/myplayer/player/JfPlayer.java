package com.example.myplayer.player;

import android.text.TextUtils;

import com.example.myplayer.listener.JfOnLoadingListener;
import com.example.myplayer.listener.JfOnPauseResumeListener;
import com.example.myplayer.listener.JfOnPreparedListener;
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

	private String source;
	private JfOnPreparedListener jfOnPreparedListener;
	private JfOnLoadingListener jfOnLoadingListener;
	private JfOnPauseResumeListener jfOnPauseResumeListener;
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

	public void prepared(){
		if (TextUtils.isEmpty(source)){
			JfLog.w("SOURCE IS EMPTY");
			return;
		}
		onCallLoading(true);//加载状态

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
	 * C++层n_prepare()完成后要调用JfOnPreparedListener
	 */
	public void onCallPrepared(){
		if (jfOnPreparedListener != null)
		{
			jfOnPreparedListener.onPrepared();
		}
	}

	/**
	 * 				   C++调用Java层
	 * @param loading
	 */
	public void onCallLoading(boolean loading){
		if (jfOnLoadingListener != null) {
			jfOnLoadingListener.onLoading(loading);
		}
	}
	private native void n_prepared(String source);
	private native void n_start();
	private native void n_pause();
	private native void n_resume();
}
