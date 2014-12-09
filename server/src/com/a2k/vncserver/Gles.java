package com.a2k.vncserver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.graphics.Bitmap;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL11Ext;

import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.util.Log;

import com.a2k.vncserver.TextureRender;

public class Gles
{
	public static final String TAG = "Gles";

	private EGL10 m_Egl;
	private EGLDisplay m_EglDisplay;
	private EGLContext m_EglContext;
	private EGLSurface m_EglSurface;
	private int m_FrameBuffer;
	private int m_DepthBuffer;

	private VncJni m_VncJni;
	private long m_GraphicBuffer;

	private static final int TEX_SURFACE_TEXTURE = 0;
	private static final int TEX_RENDER_TEXTURE = 1;
	private static final int TEX_NUMBER = 2;
	private int[] m_EglTextures = new int[TEX_NUMBER];

	private TextureRender m_TextureRender;

	public void checkGlError(String op)
	{
		int error;
		while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR)
		{
			Log.e(TAG, op + ": glError " + GLUtils.getEGLErrorString(error));
		}
	}

	private EGLConfig chooseEglConfig()
	{
		int[] configsCount = new int[1];
		EGLConfig[] configs = new EGLConfig[1];
		int[] configSpec = getConfig();

		if (!m_Egl.eglChooseConfig(m_EglDisplay, configSpec, configs, 1, configsCount))
		{
			throw new IllegalArgumentException("Failed to choose config: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		else if (configsCount[0] > 0)
		{
			return configs[0];
		}
		return null;
	}

	private static final int EGL_OPENGL_ES2_BIT = 4;
	private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

	private int[] getConfig()
	{
		return new int[] {
			EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL10.EGL_RED_SIZE, 8,
			EGL10.EGL_GREEN_SIZE, 8,
			EGL10.EGL_BLUE_SIZE, 8,
			EGL10.EGL_ALPHA_SIZE, 8,
			EGL10.EGL_DEPTH_SIZE, 0,
			EGL10.EGL_STENCIL_SIZE, 0,
			EGL10.EGL_NONE
		};
	}

	private EGLContext createContext(EGL10 egl, EGLDisplay eglDisplay, EGLConfig eglConfig)
	{
		int[] attribList = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
		return m_Egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attribList);
	}

	public void initGL(VncJni vncJni, int width, int height)
	{
		m_VncJni = vncJni;
		m_Egl = (EGL10)EGLContext.getEGL();
		m_EglDisplay = m_Egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		int[] version = new int[2];
		m_Egl.eglInitialize(m_EglDisplay, version);

		EGLConfig eglConfig = chooseEglConfig();
		m_EglContext = createContext(m_Egl, m_EglDisplay, eglConfig);
		int surfaceAttribs[] =
		{
				EGL10.EGL_WIDTH, width,
				EGL10.EGL_HEIGHT, height,
				EGL10.EGL_NONE
		};
		m_EglSurface = m_Egl.eglCreatePbufferSurface(m_EglDisplay, eglConfig, surfaceAttribs);
		if ((m_EglSurface == null) || (m_EglSurface == EGL10.EGL_NO_SURFACE))
		{
			throw new RuntimeException("GL Error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		if (!m_Egl.eglMakeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext))
		{
			throw new RuntimeException("GL Make current error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		/* Generate textures */
		GLES20.glGenTextures(TEX_NUMBER, m_EglTextures, 0);
		checkGlError("Textures generated");

		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_EglTextures[TEX_RENDER_TEXTURE]);
		checkGlError("Bind rendering texture");
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
		checkGlError("glTexParameter");
		m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(width, height);
		m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);

		/* Create framebuffer object and bind it */
		int[] values = new int[1];
		GLES20.glGenFramebuffers(1, values, 0);
		checkGlError("Framebuffer generated");
		m_FrameBuffer = values[0];
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, m_FrameBuffer);
		checkGlError("Framebuffer bound");

		GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
			GLES20.GL_TEXTURE_2D, m_EglTextures[TEX_RENDER_TEXTURE], 0);
		checkGlError("glFramebufferTexture2DOES");

		/* Create a depth buffer and bind it */
		GLES20.glGenRenderbuffers(1, values, 0);
		checkGlError("Render buffer generated");
		m_DepthBuffer = values[0];
		GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, m_DepthBuffer);
		checkGlError("Render buffer bound");

		/* Allocate storage for the depth buffer */
		GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16, width, height);
		checkGlError("Render buffer storage created");

		/* Attach the depth buffer and the texture (color buffer) to the framebuffer object */
		GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER, GLES20.GL_DEPTH_ATTACHMENT,
			GLES20.GL_RENDERBUFFER, m_DepthBuffer);
		checkGlError("Depth render buffer attached");

		int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
		if (status != GLES20.GL_FRAMEBUFFER_COMPLETE)
		{
			Log.e(TAG, "Framebuffer is not OK");
		}

		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, getTexture());
		checkGlError("Texture bind");
		m_TextureRender = new TextureRender(getTexture());
		m_TextureRender.surfaceCreated();
		Log.d(TAG, "OpenGL initialized");
	}

	public void deinitGL()
	{
		GLES20.glDeleteTextures(TEX_NUMBER, m_EglTextures, 0);
		m_Egl.eglMakeCurrent(m_EglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
		m_Egl.eglDestroySurface(m_EglDisplay, m_EglSurface);
		m_Egl.eglDestroyContext(m_EglDisplay, m_EglContext);
		m_Egl.eglTerminate(m_EglDisplay);
		Log.d(TAG, "OpenGL de-initialized");
	}
	
	public int getTexture()
	{
		return m_EglTextures[TEX_SURFACE_TEXTURE];
	}

	public void swapBufers()
	{
		m_Egl.eglSwapBuffers(m_EglDisplay, m_EglSurface);
		checkGlError("Swap buffers");
	}

	public void draw(SurfaceTexture surfaceTexture)
	{
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, m_FrameBuffer);
		m_TextureRender.drawFrame(surfaceTexture);
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
	}

	public static void saveFrame(String filename, int width, int height)
	{
		ByteBuffer mPixelBuf = ByteBuffer.allocateDirect(width * height * 4);
		mPixelBuf.order(ByteOrder.LITTLE_ENDIAN);

		mPixelBuf.rewind();
		GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, mPixelBuf);

		BufferedOutputStream bos = null;
		try
		{
			bos = new BufferedOutputStream(new FileOutputStream(filename));
			Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
			mPixelBuf.rewind();
			bmp.copyPixelsFromBuffer(mPixelBuf);
			bmp.compress(Bitmap.CompressFormat.PNG, 90, bos);
			bmp.recycle();
		}
		catch (FileNotFoundException e)
		{
		}
		finally
		{
			try
			{
				if (bos != null) bos.close();
			}
			catch (IOException e)
			{
			}
		}
		Log.d(TAG, "Saved " + width + "x" + height + " frame as '" + filename + "'");
	}
}
