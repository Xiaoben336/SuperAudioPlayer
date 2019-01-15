package com.example.myplayer.opengl;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.view.Surface;

import com.example.myplayer.R;
import com.example.myplayer.log.JfLog;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class JfRender implements GLSurfaceView.Renderer ,SurfaceTexture.OnFrameAvailableListener {
	public static final int RENDER_YUV = 1;
	public static final int RENDER_MEDIACODEC = 2;

	private Context context;

	private final float[] vertexData ={//顶点坐标

			-1f, -1f,
			1f, -1f,
			-1f, 1f,
			1f, 1f

	};

	private final float[] textureData ={//纹理坐标
			0f,1f,
			1f, 1f,
			0f, 0f,
			1f, 0f
	};

	private FloatBuffer vertexBuffer;
	private FloatBuffer textureBuffer;
	private int renderType;
	//yuv
	private int program_yuv;
	private int avPosition_yuv;
	private int afPosition_yuv;

	private int sampler_y;
	private int sampler_u;
	private int sampler_v;
	private int[] textureId_yuv;

	//渲染用
	private int width_yuv;
	private int height_yuv;
	private ByteBuffer y;
	private ByteBuffer u;
	private ByteBuffer v;


	//mediacodec
	private int program_mediacodec;
	private int avPosition_mediacodec;
	private int afPosition_mediacodec;
	private int samplerOES_mediacodec;
	private int textureId_mediacodec;
	private SurfaceTexture surfaceTexture;
	private Surface surface;
	private OnSurfaceCreateListener onSurfaceCreateListener;
	private OnRenderListener onRenderListener;

	public JfRender(Context context){
		this.context = context;
		//存储顶点坐标数据
		vertexBuffer = ByteBuffer.allocateDirect(vertexData.length * 4)
				.order(ByteOrder.nativeOrder())
				.asFloatBuffer()
				.put(vertexData);
		vertexBuffer.position(0);

		//存储纹理坐标
		textureBuffer = ByteBuffer.allocateDirect(textureData.length * 4)
				.order(ByteOrder.nativeOrder())
				.asFloatBuffer()
				.put(textureData);
		textureBuffer.position(0);
	}


	public void setOnSurfaceCreateListener(OnSurfaceCreateListener onSurfaceCreateListener) {
		this.onSurfaceCreateListener = onSurfaceCreateListener;
	}

	public void setOnRenderListener(OnRenderListener onRenderListener) {
		this.onRenderListener = onRenderListener;
	}

	public void setRenderType(int renderType) {
		this.renderType = renderType;
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		//if (renderType == RENDER_YUV) {
			initRenderYUV();
		//} else if (renderType == RENDER_MEDIACODEC) {
			initRenderMediaCodec();
		//}

	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		GLES20.glViewport(0, 0, width, height);
	}

	@Override
	public void onDrawFrame(GL10 gl) {
		//用黑色清屏
		GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
		GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		if (renderType == RENDER_YUV) {
			renderYUV();
		} else if (renderType == RENDER_MEDIACODEC){
			renderMediaCodec();
		}

		GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
	}


	/**
	 * 初始化
	 */
	private void initRenderYUV(){
		String vertexSource = JfShaderUtil.readRawTxt(context, R.raw.vertex_shader);
		String fragmentSource = JfShaderUtil.readRawTxt(context,R.raw.fragment_yuv);
		//创建一个渲染程序
		program_yuv = JfShaderUtil.createProgram(vertexSource,fragmentSource);

		//得到着色器中的属性
		avPosition_yuv = GLES20.glGetAttribLocation(program_yuv,"av_Position");
		afPosition_yuv = GLES20.glGetAttribLocation(program_yuv,"af_Position");



		sampler_y = GLES20.glGetUniformLocation(program_yuv, "sampler_y");
		sampler_u = GLES20.glGetUniformLocation(program_yuv, "sampler_u");
		sampler_v = GLES20.glGetUniformLocation(program_yuv, "sampler_v");

		//创建纹理
		textureId_yuv = new int[3];
		GLES20.glGenTextures(3, textureId_yuv, 0);

		for(int i = 0; i < 3; i++)
		{
			//绑定纹理
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[i]);
			//设置环绕和过滤方式
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
			GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
		}
		JfLog.d("initRenderYUV");
	}

	public void setYUVRenderData(int width, int height, byte[] y, byte[] u, byte[] v)
	{
		this.width_yuv = width;
		this.height_yuv = height;
		this.y = ByteBuffer.wrap(y);
		this.u = ByteBuffer.wrap(u);
		this.v = ByteBuffer.wrap(v);
	}

	/**
	 * 渲染
	 */
	private void renderYUV(){
		JfLog.d("渲染中");
		if(width_yuv > 0 && height_yuv > 0 && y != null && u != null && v != null){
			GLES20.glUseProgram(program_yuv);//使用源程序

			GLES20.glEnableVertexAttribArray(avPosition_yuv);//使顶点属性数组有效
			GLES20.glVertexAttribPointer(avPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);//为顶点属性赋值

			GLES20.glEnableVertexAttribArray(afPosition_yuv);
			GLES20.glVertexAttribPointer(afPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

			GLES20.glActiveTexture(GLES20.GL_TEXTURE0);//激活纹理
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[0]);//绑定纹理
			GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv, height_yuv, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, y);//

			GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[1]);
			GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv / 2, height_yuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, u);

			GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
			GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[2]);
			GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv / 2, height_yuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, v);

			GLES20.glUniform1i(sampler_y, 0);
			GLES20.glUniform1i(sampler_u, 1);
			GLES20.glUniform1i(sampler_v, 2);

			y.clear();
			u.clear();
			v.clear();
			y = null;
			u = null;
			v = null;

			//GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
		}
	}


	private void initRenderMediaCodec(){
		JfLog.d("initRenderMediaCodec");
		String vertexSource = JfShaderUtil.readRawTxt(context, R.raw.vertex_shader);
		String fragmentSource = JfShaderUtil.readRawTxt(context,R.raw.fragment_mediacodec);
		//创建一个渲染程序
		program_mediacodec = JfShaderUtil.createProgram(vertexSource,fragmentSource);

		//得到着色器中的属性
		avPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec,"av_Position");
		afPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec,"af_Position");

		samplerOES_mediacodec = GLES20.glGetUniformLocation(program_mediacodec, "sTexture");
		int[] textureids = new int[1];
		GLES20.glGenTextures(1,textureids,0);

		textureId_mediacodec = textureids[0];


		//绑定纹理
		//GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[i]);
		//设置环绕和过滤方式
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

		surfaceTexture = new SurfaceTexture(textureId_mediacodec);
		surface = new Surface(surfaceTexture);
		surfaceTexture.setOnFrameAvailableListener(this);

		if (onSurfaceCreateListener != null) {
			JfLog.d("onSurfaceCreateListener != null");
			onSurfaceCreateListener.onSurfaceCreate(surface);
		}
	}


	private void renderMediaCodec(){
		surfaceTexture.updateTexImage();
		GLES20.glUseProgram(program_mediacodec);

		GLES20.glEnableVertexAttribArray(avPosition_mediacodec);//使顶点属性数组有效
		GLES20.glVertexAttribPointer(avPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);//为顶点属性赋值

		GLES20.glEnableVertexAttribArray(afPosition_mediacodec);
		GLES20.glVertexAttribPointer(afPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);//激活纹理
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId_mediacodec);//绑定纹理
		GLES20.glUniform1i(samplerOES_mediacodec,0);
	}
	@Override
	public void onFrameAvailable(SurfaceTexture surfaceTexture) {
		if (onRenderListener != null) {
			onRenderListener.onRender();
		}
	}

	public interface OnSurfaceCreateListener{
		void onSurfaceCreate(Surface surface);
	}

	public interface OnRenderListener{
		void onRender();
	}
}
