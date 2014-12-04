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

	private static final String vertexShaderCode =
		"attribute vec4 vPosition;" +
		"attribute vec4 vTexCoordinate;" +
		"uniform mat4 textureTransform;" +
		"varying vec2 v_TexCoordinate;" +
		"void main() {" +
		"   v_TexCoordinate = (textureTransform * vTexCoordinate).xy;" +
		"   gl_Position = vPosition;" +
		"}";

	private static final String fragmentShaderCode =
		"#extension GL_OES_EGL_image_external : require\n" +
		"precision mediump float;" +
		"uniform samplerExternalOES texture;" +
		"varying vec2 v_TexCoordinate;" +
		"void main () {" +
		"    vec4 color = texture2D(texture, v_TexCoordinate);" +
		"    gl_FragColor = color;" +
		"}";

	private static float squareSize = 1.0f;
	private static float squareCoords[] =
	{
		-squareSize,  squareSize, 0.0f,	// top left
		-squareSize, -squareSize, 0.0f,	// bottom left
		squareSize, -squareSize, 0.0f,	// bottom right
		squareSize,  squareSize, 0.0f	// top right
	};

	private static short drawOrder[] = { 0, 1, 2, 0, 2, 3};

	private FloatBuffer textureBuffer;
	private float textureCoords[] =
	{
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f
	};

	private int m_TextureWidth;
	private int m_TextureHeight;
	private int m_ViewWidth;
	private int m_ViewHeight;

	private EGL10 m_Egl;
	private EGLDisplay m_EglDisplay;
	private EGLContext m_EglContext;
	private EGLSurface m_EglSurface;
	private int m_EglTextures[] = new int[1];

	private int m_VertexShaderHandle;
	private int m_FragmentShaderHandle;
	private int m_ShaderProgram;
	private FloatBuffer m_VertexBuffer;
	private ShortBuffer m_DrawListBuffer;
	private boolean m_AdjustViewport = false;

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

	public void checkGlError(String op)
	{
		int error;
		while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR)
		{
			Log.e(TAG, op + ": glError " + GLUtils.getEGLErrorString(error));
		}
	}

	public void initGL(SurfaceTexture nativeWindow)
	{
		m_Egl = (EGL10)EGLContext.getEGL();
		m_EglDisplay = m_Egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		int[] version = new int[2];
		m_Egl.eglInitialize(m_EglDisplay, version);

		EGLConfig eglConfig = chooseEglConfig();
		m_EglContext = createContext(m_Egl, m_EglDisplay, eglConfig);
		m_EglSurface = m_Egl.eglCreateWindowSurface(m_EglDisplay, eglConfig, nativeWindow, null);
		if ((m_EglSurface == null) || (m_EglSurface == EGL10.EGL_NO_SURFACE))
		{
			throw new RuntimeException("GL Error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
		if (!m_Egl.eglMakeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext))
		{
			throw new RuntimeException("GL Make current error: " + GLUtils.getEGLErrorString(m_Egl.eglGetError()));
		}
	}

	public void setupTexture(int width, int height)
	{
		m_TextureWidth = width;
		m_TextureHeight = height;

		ByteBuffer texturebb = ByteBuffer.allocateDirect(textureCoords.length * 4);
		texturebb.order(ByteOrder.nativeOrder());

		textureBuffer = texturebb.asFloatBuffer();
		textureBuffer.put(textureCoords);
		textureBuffer.position(0);

		/* Generate the actual texture */
		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		GLES20.glGenTextures(1, m_EglTextures, 0);
		checkGlError("Texture generate");

		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_EglTextures[0]);
		checkGlError("Texture bind");
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

	public void loadShaders()
	{
		m_VertexShaderHandle = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
		GLES20.glShaderSource(m_VertexShaderHandle, vertexShaderCode);
		GLES20.glCompileShader(m_VertexShaderHandle);
		checkGlError("Vertex shader compile");

		m_FragmentShaderHandle = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
		GLES20.glShaderSource(m_FragmentShaderHandle, fragmentShaderCode);
		GLES20.glCompileShader(m_FragmentShaderHandle);
		checkGlError("Pixel shader compile");

		m_ShaderProgram = GLES20.glCreateProgram();
		GLES20.glAttachShader(m_ShaderProgram, m_VertexShaderHandle);
		GLES20.glAttachShader(m_ShaderProgram, m_FragmentShaderHandle);
		GLES20.glLinkProgram(m_ShaderProgram);
		checkGlError("Shader program compile");

		int[] status = new int[1];
		GLES20.glGetProgramiv(m_ShaderProgram, GLES20.GL_LINK_STATUS, status, 0);
		if (status[0] != GLES20.GL_TRUE)
		{
			String error = GLES20.glGetProgramInfoLog(m_ShaderProgram);
			Log.e("SurfaceTest", "Error while linking program:\n" + error);
		}
	}

	public void setupVertexBuffer()
	{
		ByteBuffer dlb = ByteBuffer.allocateDirect(drawOrder. length * 2);
		dlb.order(ByteOrder.nativeOrder());
		m_DrawListBuffer = dlb.asShortBuffer();
		m_DrawListBuffer.put(drawOrder);
		m_DrawListBuffer.position(0);

		/* Initialize the texture holder */
		ByteBuffer bb = ByteBuffer.allocateDirect(squareCoords.length * 4);
		bb.order(ByteOrder.nativeOrder());

		m_VertexBuffer = bb.asFloatBuffer();
		m_VertexBuffer.put(squareCoords);
		m_VertexBuffer.position(0);
	}

	private void adjustViewport()
	{
		float surfaceAspect = m_TextureHeight / (float)m_TextureWidth;
		float viewAspect = m_ViewHeight / (float)m_ViewWidth;

		if (surfaceAspect > viewAspect)
		{
			float heightRatio = m_TextureHeight / (float)m_ViewHeight;
			int newWidth = (int)(m_TextureWidth * heightRatio);
			int xOffset = (newWidth - m_TextureWidth) / 2;
			GLES20.glViewport(-xOffset, 0, newWidth, m_TextureHeight);
		}
		else
		{
			float widthRatio = m_TextureWidth / (float)m_ViewWidth;
			int newHeight = (int)(m_TextureHeight * widthRatio);
			int yOffset = (newHeight - m_TextureHeight) / 2;
			GLES20.glViewport(0, -yOffset, m_TextureWidth, newHeight);
		}
		m_AdjustViewport = false;
	}

	public void setViewport(int width, int height)
	{
		m_ViewHeight = height;
		m_ViewWidth = width;
		m_AdjustViewport = true;
	}

	public void draw(float videoTextureTransform[])
	{
		if (m_AdjustViewport)
		{
			adjustViewport();
		}

		GLES20.glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

		/* Draw texture */
		GLES20.glUseProgram(m_ShaderProgram);
		int textureParamHandle = GLES20.glGetUniformLocation(m_ShaderProgram, "texture");
		int textureCoordinateHandle = GLES20.glGetAttribLocation(m_ShaderProgram, "vTexCoordinate");
		int positionHandle = GLES20.glGetAttribLocation(m_ShaderProgram, "vPosition");
		int textureTranformHandle = GLES20.glGetUniformLocation(m_ShaderProgram, "textureTransform");

		GLES20.glEnableVertexAttribArray(positionHandle);
		GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 4 * 3, m_VertexBuffer);

		GLES20.glBindTexture(GLES20.GL_TEXTURE0, m_EglTextures[0]);
		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		GLES20.glUniform1i(textureParamHandle, 0);

		GLES20.glEnableVertexAttribArray(textureCoordinateHandle);
		GLES20.glVertexAttribPointer(textureCoordinateHandle, 4, GLES20.GL_FLOAT, false, 0, textureBuffer);

		GLES20.glUniformMatrix4fv(textureTranformHandle, 1, false, videoTextureTransform, 0);

		GLES20.glDrawElements(GLES20.GL_TRIANGLES, drawOrder.length, GLES20.GL_UNSIGNED_SHORT, m_DrawListBuffer);
		GLES20.glDisableVertexAttribArray(positionHandle);
		GLES20.glDisableVertexAttribArray(textureCoordinateHandle);
	}
}
