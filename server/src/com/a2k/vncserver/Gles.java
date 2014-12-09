package com.a2k.vncserver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.util.Log;

public class Gles
{
	public static final String TAG = "Gles";

	private EGL10 m_Egl;
	private EGLDisplay m_EglDisplay;
	private EGLContext m_EglContext;
	private EGLSurface m_EglSurface;
	private int m_EglTextures[] = new int[1];

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
			EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
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

	public void initGL()
	{
		m_Egl = (EGL10)EGLContext.getEGL();
		m_EglDisplay = m_Egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		int[] version = new int[2];
		m_Egl.eglInitialize(m_EglDisplay, version);

		EGLConfig eglConfig = chooseEglConfig();
		m_EglContext = createContext(m_Egl, m_EglDisplay, eglConfig);
		int surfaceAttribs[] =
		{
				EGL10.EGL_WIDTH, 1,
				EGL10.EGL_HEIGHT, 1,
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
		/* Generate the actual texture */
		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		GLES20.glGenTextures(1, m_EglTextures, 0);
		checkGlError("Texture generate");
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_EglTextures[0]);
		checkGlError("Texture bind");
		Log.d(TAG, "OpenGL initialized");
	}

	public void deinitGL()
	{
		GLES20.glDeleteTextures(1, m_EglTextures, 0);
		m_Egl.eglMakeCurrent(m_EglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
		m_Egl.eglDestroySurface(m_EglDisplay, m_EglSurface);
		m_Egl.eglDestroyContext(m_EglDisplay, m_EglContext);
		m_Egl.eglTerminate(m_EglDisplay);
		Log.d(TAG, "OpenGL de-initialized");
	}
	
	public int getTexture()
	{
		return m_EglTextures[0];
	}

	public void swapBufers()
	{
		m_Egl.eglSwapBuffers(m_EglDisplay, m_EglSurface);
		checkGlError("Swap buffers");
	}
}
