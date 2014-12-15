package com.a2k.vncserver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLU;
import android.opengl.Matrix;
import android.util.Log;

import com.a2k.vncserver.VncJni;

class TextureRender
{
	private static final String TAG = "TextureRender";

	private VncJni m_VncJni;
	private int m_Width;
	private int m_Height;

	private EGL10 m_Egl;
	private EGLDisplay m_EglDisplay;
	private EGLContext m_EglContext;
	private EGLSurface m_EglSurface;

	private static final int TEX_SURFACE_TEXTURE = 0;
	private static final int TEX_RENDER_TEXTURE = 1;
	private static final int TEX_NUMBER = 2;
	private int[] m_EglTextures = new int[TEX_NUMBER];
	private int m_FrameBuffer;
	private int m_PixelFormat;
	private long m_GraphicBuffer;

	private String m_DumpOutputDir;

	private static final int FLOAT_SIZE_BYTES = 4;
	private static final int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
	private static final int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
	private static final int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;
	private final float[] m_TriangleVerticesData =
	{
		 /* X      Y  Z,   U    V */
		-1.0f, -1.0f, 0, 0.f, 0.f,
		 1.0f, -1.0f, 0, 1.f, 0.f,
		-1.0f,  1.0f, 0, 0.f, 1.f,
		 1.0f,  1.0f, 0, 1.f, 1.f,
	};

	private FloatBuffer m_TriangleVertices;

	private static final String VERTEX_SHADER =
		"uniform mat4 uMVPMatrix;\n" +
		"uniform mat4 uSTMatrix;\n" +
		"attribute vec4 aPosition;\n" +
		"attribute vec4 aTextureCoord;\n" +
		"varying vec2 vTextureCoord;\n" +
		"void main() {\n" +
		"  gl_Position = uMVPMatrix * aPosition;\n" +
		"  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n" +
		"  vTextureCoord = vec2(vTextureCoord.s, 1.0 - vTextureCoord.t);\n" +
		"}\n";

	private static final String FRAGMENT_SHADER =
		"#extension GL_OES_EGL_image_external : require\n" +
		"precision mediump float;\n" +      // highp here doesn't seem to matter
		"varying vec2 vTextureCoord;\n" +
		"uniform samplerExternalOES sTexture;\n" +
		"void main() {\n" +
		"  gl_FragColor = texture2D(sTexture, vTextureCoord);\n" +
		"}\n";

	private static final int EGL_OPENGL_ES2_BIT = 4;
	private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

	private int[] getConfig()
	{
		if (m_PixelFormat == GLES20.GL_RGB565)
		{
			return new int[] {
				EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
				EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
				EGL10.EGL_RED_SIZE, 5,
				EGL10.EGL_GREEN_SIZE, 6,
				EGL10.EGL_BLUE_SIZE, 5,
				EGL10.EGL_ALPHA_SIZE, 0,
				EGL10.EGL_DEPTH_SIZE, 0,
				EGL10.EGL_STENCIL_SIZE, 0,
				EGL10.EGL_NONE
			};
		}
		else if (m_PixelFormat == GLES20.GL_RGBA)
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
		Log.d(TAG, "Unsupported pixel format " + m_PixelFormat);
		return null;
	}

	private float[] m_MVPMatrix = new float[16];
	private float[] m_STMatrix = new float[16];

	private int m_Program;
	private int m_uMVPMatrixHandle;
	private int m_uSTMatrixHandle;
	private int m_aPositionHandle;
	private int m_aTextureHandle;

	public TextureRender(VncJni vncJni, int width, int height, int pixelFormat)
	{
		m_VncJni = vncJni;
		m_Width = width;
		m_Height = height;
		m_PixelFormat = pixelFormat;
		m_TriangleVertices = ByteBuffer.allocateDirect(
			m_TriangleVerticesData.length * FLOAT_SIZE_BYTES)
			.order(ByteOrder.nativeOrder()).asFloatBuffer();
		m_TriangleVertices.put(m_TriangleVerticesData).position(0);

		Matrix.setIdentityM(m_STMatrix, 0);
	}

	public void setDumpOutputDir(String outputDir)
	{
		m_DumpOutputDir = outputDir;
	}

	public int getTextureId()
	{
		return m_EglTextures[TEX_SURFACE_TEXTURE];
	}

	private int m_SaveCounter = 20;
	public void drawFrame(SurfaceTexture st)
	{
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, m_FrameBuffer);
		draw(st);
		m_VncJni.glOnFrameAvailable(m_GraphicBuffer);
		if (--m_SaveCounter == 0)
		{
			m_VncJni.glDumpFrame(m_GraphicBuffer, m_DumpOutputDir + "/surface.data");
			m_SaveCounter = 20;
		}
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
	}

	public void draw(SurfaceTexture st)
	{
		checkGlError("onDrawFrame start");
		st.getTransformMatrix(m_STMatrix);

		GLES20.glViewport(0, 0, m_Width, m_Height);
		GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

		GLES20.glUseProgram(m_Program);
		checkGlError("glUseProgram");

		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, getTextureId());

		m_TriangleVertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
		GLES20.glVertexAttribPointer(m_aPositionHandle, 3, GLES20.GL_FLOAT, false,
			TRIANGLE_VERTICES_DATA_STRIDE_BYTES, m_TriangleVertices);
		checkGlError("glVertexAttribPointer maPosition");
		GLES20.glEnableVertexAttribArray(m_aPositionHandle);
		checkGlError("glEnableVertexAttribArray maPositionHandle");

		m_TriangleVertices.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
		GLES20.glVertexAttribPointer(m_aTextureHandle, 2, GLES20.GL_FLOAT, false,
			TRIANGLE_VERTICES_DATA_STRIDE_BYTES, m_TriangleVertices);
		checkGlError("glVertexAttribPointer maTextureHandle");
		GLES20.glEnableVertexAttribArray(m_aTextureHandle);
		checkGlError("glEnableVertexAttribArray maTextureHandle");

		Matrix.setIdentityM(m_MVPMatrix, 0);
		GLES20.glUniformMatrix4fv(m_uMVPMatrixHandle, 1, false, m_MVPMatrix, 0);
		GLES20.glUniformMatrix4fv(m_uSTMatrixHandle, 1, false, m_STMatrix, 0);

		GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
		checkGlError("glDrawArrays");
		GLES20.glFinish();
	}

	public void swapBuffers()
	{
		m_Egl.eglSwapBuffers(m_EglDisplay, m_EglSurface);
	}

	private EGLConfig chooseEglConfig()
	{
		int[] configsCount = new int[1];
		int[] configSpec = getConfig();

		/* get number of configurations */
		if (!m_Egl.eglChooseConfig(m_EglDisplay, null, null, 0, configsCount))
		{
			throw new IllegalArgumentException("Failed to get number of configs: " +
				GLU.gluErrorString(m_Egl.eglGetError()));
		}
		/* read all */
		EGLConfig[] configs = new EGLConfig[configsCount[0]];
		if (!m_Egl.eglChooseConfig(m_EglDisplay, null, configs, configsCount[0], configsCount))
		{
			throw new IllegalArgumentException("Failed to choose config: " +
				GLU.gluErrorString(m_Egl.eglGetError()));
		}
		else if (configsCount[0] > 0)
		{
			/* find number of pairs in the configuration we want */
			int numPairs = 0;
			while (configSpec[numPairs] != EGL10.EGL_NONE)
			{
				numPairs += 2;
			}
			numPairs /= 2;
			/* find configuration that suits */
			int[] attr = new int[1];
			for (int i = 0; i < configsCount[0]; i++)
			{
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

	private void dumpConfig(EGLConfig eglConfig)
	{
		int[] att = new int[1];
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_SURFACE_TYPE, att);
		Log.d(TAG, "EGL_SURFACE_TYPE " + att[0]);
		if ((att[0] & EGL10.EGL_WINDOW_BIT) != EGL10.EGL_WINDOW_BIT)
		{
			Log.d(TAG, "Failed to choose EGL_WINDOW_BIT");
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
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_STENCIL_SIZE, att);
		Log.d(TAG, "EGL_STENCIL_SIZE " + att[0]);
		m_Egl.eglGetConfigAttrib(m_EglDisplay, eglConfig, EGL10.EGL_BUFFER_SIZE, att);
		Log.d(TAG, "EGL_BUFFER_SIZE " + att[0]);
	}

	private EGLContext createContext(EGL10 egl, EGLDisplay eglDisplay, EGLConfig eglConfig)
	{
		int[] attribList =
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL10.EGL_NONE
		};
		return m_Egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attribList);
	}

	private int createFrameBuffer(int width, int height, int targetTextureId)
	{
		int framebuffer;
		int[] framebuffers = new int[1];
		GLES20.glGenFramebuffers(1, framebuffers, 0);
		framebuffer = framebuffers[0];
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);

		int depthbuffer;
		int[] renderbuffers = new int[1];
		GLES20.glGenRenderbuffers(1, renderbuffers, 0);
		depthbuffer = renderbuffers[0];

		GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, depthbuffer);
		GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER,
			GLES20.GL_DEPTH_COMPONENT16, width, height);
		GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER,
			GLES20.GL_DEPTH_ATTACHMENT, GLES20.GL_RENDERBUFFER, depthbuffer);

		GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER,
			GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D,
			targetTextureId, 0);
		int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
		if (status != GLES20.GL_FRAMEBUFFER_COMPLETE)
		{
			throw new RuntimeException("Framebuffer is not complete: " +
				GLU.gluErrorString(status));
		}
		GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
		return framebuffer;
	}

	private void initGL(int width, int height, int pixelFormat)
	{
		m_Egl = (EGL10)EGLContext.getEGL();
		m_EglDisplay = m_Egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		int[] version = new int[2];
		m_Egl.eglInitialize(m_EglDisplay, version);

		EGLConfig eglConfig = chooseEglConfig();
		m_EglContext = createContext(m_Egl, m_EglDisplay, eglConfig);
		dumpConfig(eglConfig);
		/* we always render to FBO */
		int surfaceAttribs[] =
		{
			EGL10.EGL_WIDTH, 1,
			EGL10.EGL_HEIGHT, 1,
			EGL10.EGL_NONE
		};
		m_EglSurface = m_Egl.eglCreatePbufferSurface(m_EglDisplay, eglConfig, surfaceAttribs);
		if ((m_EglSurface == null) || (m_EglSurface == EGL10.EGL_NO_SURFACE))
		{
			throw new RuntimeException("GL Error: 0x" + Integer.toHexString(m_Egl.eglGetError()));
		}
		if (!m_Egl.eglMakeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext))
		{
			throw new RuntimeException("GL Make current error: 0x" + Integer.toHexString(m_Egl.eglGetError()));
		}
		/* Generate textures */
		GLES20.glGenTextures(TEX_NUMBER, m_EglTextures, 0);
		checkGlError("Textures generated");
		GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, m_EglTextures[TEX_RENDER_TEXTURE]);
		checkGlError("glBindTexture TEX_RENDER_TEXTURE");
		GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
		GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
		checkGlError("glTexParameter");
		m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(width, height, pixelFormat);
		m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);
		GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
		m_FrameBuffer = createFrameBuffer(width, height, m_EglTextures[TEX_RENDER_TEXTURE]);
		Log.d(TAG, "OpenGL initialized");
	}

	public void start()
	{
		initGL(m_Width, m_Height, m_PixelFormat);
		m_Program = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
		if (m_Program == 0)
		{
			throw new RuntimeException("failed creating program");
		}
		m_aPositionHandle = GLES20.glGetAttribLocation(m_Program, "aPosition");
		checkGlError("glGetAttribLocation aPosition");
		if (m_aPositionHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for aPosition");
		}
		m_aTextureHandle = GLES20.glGetAttribLocation(m_Program, "aTextureCoord");
		checkGlError("glGetAttribLocation aTextureCoord");
		if (m_aTextureHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for aTextureCoord");
		}

		m_uMVPMatrixHandle = GLES20.glGetUniformLocation(m_Program, "uMVPMatrix");
		checkGlError("glGetUniformLocation uMVPMatrix");
		if (m_uMVPMatrixHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for uMVPMatrix");
		}

		m_uSTMatrixHandle = GLES20.glGetUniformLocation(m_Program, "uSTMatrix");
		checkGlError("glGetUniformLocation uSTMatrix");
		if (m_uSTMatrixHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for uSTMatrix");
		}
	}

	public void changeFragmentShader(String fragmentShader)
	{
		GLES20.glDeleteProgram(m_Program);
		m_Program = createProgram(VERTEX_SHADER, fragmentShader);
		if (m_Program == 0)
		{
			throw new RuntimeException("failed creating program");
		}
	}

	private int loadShader(int shaderType, String source)
	{
		int shader = GLES20.glCreateShader(shaderType);
		checkGlError("glCreateShader type=" + shaderType);
		GLES20.glShaderSource(shader, source);
		GLES20.glCompileShader(shader);
		int[] compiled = new int[1];
		GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
		if (compiled[0] == 0)
		{
			Log.e(TAG, "Could not compile shader " + shaderType + ":");
			Log.e(TAG, " " + GLES20.glGetShaderInfoLog(shader));
			GLES20.glDeleteShader(shader);
			shader = 0;
		}
		return shader;
	}

	private int createProgram(String vertexSource, String fragmentSource)
	{
		int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
		if (vertexShader == 0)
		{
			return 0;
		}
		int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
		if (pixelShader == 0)
		{
			return 0;
		}

		int program = GLES20.glCreateProgram();
		checkGlError("glCreateProgram");
		if (program == 0)
		{
			Log.e(TAG, "Could not create program");
		}
		GLES20.glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		GLES20.glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");
		GLES20.glLinkProgram(program);
		int[] linkStatus = new int[1];
		GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
		if (linkStatus[0] != GLES20.GL_TRUE)
		{
			Log.e(TAG, "Could not link program: ");
			Log.e(TAG, GLES20.glGetProgramInfoLog(program));
			GLES20.glDeleteProgram(program);
			program = 0;
		}
		return program;
	}

	public void checkGlError(String op)
	{
		int error;
		while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR)
		{
			Log.e(TAG, op + ": glError 0x" + Integer.toHexString(error));
			throw new RuntimeException(op + ": glError " + GLU.gluErrorString(error));
		}
	}
}
