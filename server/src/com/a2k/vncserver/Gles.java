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
import android.graphics.Bitmap.Config;

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

import com.a2k.vncserver.TextureRender;

public class Gles
{
	public static final String TAG = "Gles";

	private EGL10 m_Egl;
	private EGLDisplay m_EglDisplay;
	private EGLContext m_EglContext;
	private EGLSurface m_EglSurface;
	private int m_EglTextures[] = new int[1];
	private Bitmap m_Pixmap;
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
		int[] configSpec = getConfig();

		if (!m_Egl.eglChooseConfig(m_EglDisplay, /*configSpec*/null, null, 0, configsCount))
		{
			throw new IllegalArgumentException("Failed to get number of configs: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		EGLConfig[] configs = new EGLConfig[configsCount[0]];
		if (!m_Egl.eglChooseConfig(m_EglDisplay, /*configSpec*/null, configs, configsCount[0], configsCount))
		{
			throw new IllegalArgumentException("Failed to choose config: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		else if (configsCount[0] > 0)
		{
			int numPairs = 0;
			while (configSpec[numPairs] != EGL10.EGL_NONE)
			{
				numPairs += 2;
			}
			numPairs /= 2;
			int[] attr = new int[1];
			for (int i = 0; i < configsCount[0]; i++)
			{
				Log.d(TAG, "\n\nEGLConfig[" + i + "]:");
				dumpConfig(configs[i]);
				int match = 0;
				for (int j = 0, idx = 0; j < numPairs; j++, idx += 2)
				{
					m_Egl.eglGetConfigAttrib(m_EglDisplay, configs[i], configSpec[idx], attr);
					if (configSpec[idx] == EGL10.EGL_SURFACE_TYPE)
					{
						if ((attr[0] & configSpec[idx + 1]) == configSpec[idx + 1])
						{
							match++;
						}
						else
						{
							break;
						}
					}
					else if (configSpec[idx] == EGL10.EGL_RENDERABLE_TYPE)
					{
						if ((attr[0] & configSpec[idx + 1]) == configSpec[idx + 1])
						{
							match++;
						}
						else
						{
							break;
						}
					}
					else if (attr[0] == configSpec[idx + 1])
					{
						match++;
					}
					else
					{
						break;
					}
				}
				if (match == numPairs)
				{
					return configs[i]; 
				}
			}
			return null;
		}
		return null;
	}

	private static final int EGL_OPENGL_ES2_BIT = 4;
	private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
	private static final int GGL_PIXEL_FORMAT_RGB_565 = 4;

	private int[] getConfig()
	{
		return new int[] {
			EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PIXMAP_BIT,
			EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL10.EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565,
			EGL10.EGL_RED_SIZE, 5,
			EGL10.EGL_GREEN_SIZE, 6,
			EGL10.EGL_BLUE_SIZE, 5,
			EGL10.EGL_ALPHA_SIZE, 0,
			EGL10.EGL_DEPTH_SIZE, 0,
			EGL10.EGL_BUFFER_SIZE, 16,
			EGL10.EGL_NONE
		};
	}

	private EGLContext createContext(EGL10 egl, EGLDisplay eglDisplay, EGLConfig eglConfig)
	{
		int[] attribList = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
		return m_Egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attribList);
	}
	
	private void dumpConfig(EGLConfig eglConfig)
	{
		int[] att = new int[1];
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_SURFACE_TYPE, att);
		Log.d(TAG, "EGL_SURFACE_TYPE " + att[0]);
		if ((att[0] & EGL10.EGL_PIXMAP_BIT) != EGL10.EGL_PIXMAP_BIT)
		{
			Log.d(TAG, "Failed to choose EGL_PIXMAP_BIT");
		}
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_RENDERABLE_TYPE, att);
		Log.d(TAG, "EGL_RENDERABLE_TYPE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_RED_SIZE, att);
		Log.d(TAG, "EGL_RED_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_GREEN_SIZE, att);
		Log.d(TAG, "EGL_GREEN_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_BLUE_SIZE, att);
		Log.d(TAG, "EGL_BLUE_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_ALPHA_SIZE, att);
		Log.d(TAG, "EGL_ALPHA_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_DEPTH_SIZE, att);
		Log.d(TAG, "EGL_DEPTH_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_CONFIG_ID, att);
		Log.d(TAG, "EGL_CONFIG_ID " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_NATIVE_VISUAL_ID, att);
		Log.d(TAG, "EGL_NATIVE_VISUAL_ID " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_BUFFER_SIZE, att);
		Log.d(TAG, "EGL_BUFFER_SIZE " + att[0]);
	}

	public void initGL(int width, int height)
	{
		m_Egl = (EGL10)EGLContext.getEGL();
		m_EglDisplay = m_Egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		int[] version = new int[2];
		m_Egl.eglInitialize(m_EglDisplay, version);

		EGLConfig eglConfig = chooseEglConfig();
		m_EglContext = createContext(m_Egl, m_EglDisplay, eglConfig);
		/* create pixmap buffer */
		m_Pixmap = Bitmap.createBitmap(width, height, Config.RGB_565);
		m_Pixmap.setHasAlpha(false);
		m_Pixmap.setPremultiplied(false);
		int surfaceAttribs[] =
		{
				//EGL10.EGL_WIDTH, width,
				//EGL10.EGL_HEIGHT, height,
				EGL10.EGL_NONE
		};
		dumpConfig(eglConfig);
		m_EglSurface = m_Egl.eglCreatePixmapSurface(m_EglDisplay, eglConfig, m_Pixmap, /*surfaceAttribs*/null);
		if ((m_EglSurface == null) || (m_EglSurface == EGL10.EGL_NO_SURFACE))
		{
			throw new RuntimeException("GL Error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		if (!m_Egl.eglMakeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext))
		{
			throw new RuntimeException("GL Make current error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		/* Generate the actual texture */
		GLES20.glGenTextures(1, m_EglTextures, 0);
		checkGlError("Texture generate");
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_EglTextures[0]);
		checkGlError("Texture bind");
		m_TextureRender = new TextureRender(getTexture());
		m_TextureRender.surfaceCreated();
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
	
	public Bitmap getBitmap()
	{
		return m_Pixmap;
	}

	public void swapBufers()
	{
		m_Egl.eglSwapBuffers(m_EglDisplay, m_EglSurface);
		checkGlError("Swap buffers");
	}

	public void draw(SurfaceTexture surfaceTexture)
	{
		m_TextureRender.drawFrame(surfaceTexture);
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
