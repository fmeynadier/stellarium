/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef STELTEXTUREMGR_H_
#define STELTEXTUREMGR_H_

#include <string>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include "GLee.h"
#include "STexture.hpp"
#include "STextureTypes.hpp"

class ManagedSTexture : public STexture
{
	friend class StelTextureMgr;
	friend class ImageLoader;
public:
	//! Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
	//! If the texture was lazy loaded, load it now.
	virtual void lazyBind();
	//! Return the average texture luminance.
	//! @return 0 is black, 1 is white
    virtual float getAverageLuminance(void);
    
    virtual ~ManagedSTexture()
    {
    	if (loadState==ManagedSTexture::LOADING_IMAGE)
    	{
    		// TODO should search in the loading queue the matching thread and kill it
    		assert(0);
    	}
    }
	virtual int getLoadState(void) { return loadState;}
	
	//! Supported dynamic range modes
	enum DynamicRangeMode
	{
		LINEAR,
		MINMAX_USER,
		MINMAX_QUANTILE,
		MINMAX_GREYLEVEL,
		MINMAX_GREYLEVEL_AUTO
	};
	
	enum LoadState
	{
		UNLOADED=0,
		LOADED=1,
		LOAD_ERROR,
		LOADING_IMAGE
	};
	LoadState loadState;
	
private:
	friend struct loadTextureThread;

	ManagedSTexture() : loadState(UNLOADED), avgLuminance(-1.f) {;}

	void load(void);
	
	// Cached average luminance
	float avgLuminance;
	
	DynamicRangeMode dynamicRangeMode;
};

//! Abstract class for any Image loaders.
class ImageLoader
{
public:
	virtual ~ImageLoader() {;}
	//! Load the data from the image and store it into tex.texels
	//! The caller is responsible for freeing the memory allocated in tex.texels
	//! This method must be thread compliant
	virtual bool loadImage(const std::string& filename, ManagedSTexture& tex) = 0;
};

//! Struct used for returning queued textures loaded in thread
struct QueuedTex
{
	QueuedTex(ManagedSTextureSP atex, void* auserPtr, const std::string& aurl, const std::string& alocalPath) :
		tex(atex), userPtr(auserPtr), url(aurl), localPath(alocalPath) {;}
	ManagedSTextureSP tex;
	void* userPtr;
	std::string url;
	std::string localPath;
};

/**
 * Class used to manage textures loading and manipulation.
 * @author Fabien Chereau <stellarium@free.fr>
 */
class StelTextureMgr
{
public:
	StelTextureMgr(const std::string& textureDir);
	virtual ~StelTextureMgr();
	
	//! Initialize some variable from the openGL contex.
	//! Must be called after the creation of the GLContext.
	void init();
	
	//! Update loading of textures in threads
	void update();
	
	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param lazyLoading if true the texture will be loaded only when it used for the first time
	ManagedSTextureSP createTexture(const std::string& filename, bool lazyLoading=false);
	
	//! Load an image from a file and create a new texture from it in a new thread. The created texture is inserted in
	//! the passed queue, protected by the given mutex.
	//! If the texture creation fails for any reasons, its loadState will be set to LOAD_ERROR
	//! @param url the texture file name or URL, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param queue the queue where the texture will be inserted once downloaded
	//! @param queueMutex the mutex protecting the queue
	bool createTextureThread(const std::string& url, std::vector<QueuedTex*>* queue, 
		boost::mutex* queueMutex, void* userPtr=NULL, const std::string& fileExtension="", 
		bool toDelete=true);
	
	//! Define if mipmaps must be created while creating textures
	void setMipmapsMode(bool b = false) {mipmapsMode = b;}
	
	//! Define the texture wrapping mode to use while creating textures
	//! @param m can be either GL_CLAMP, GL_CLAMP_TO_EDGE, or GL_REPEAT.
	//! See doc for glTexParameter for more info.
	void setWrapMode(GLint m = GL_CLAMP) {wrapMode = m;}
	
	//! Define the texture min filter to use while creating textures
	//! @param m can be either GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
	//! GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	//! See doc for glTexParameter for more info.
	void setMinFilter(GLint m = GL_NEAREST) {minFilter = m;}
	
	//! Define the texture magnification filter to use while creating textures
	//! @param m can be either GL_NEAREST, GL_LINEAR
	//! See doc for glTexParameter for more info.
	void setMagFilter(GLint m = GL_LINEAR) {magFilter = m;}
	
	//! Set default parameters for Mipmap mode, wrap mode, min and mag filters
	void setDefaultParams();
	
	//! Define how the dynamic range of the image will be adapted to fit on 8 bits
	//! Note that using linear mode on 8 bits images does nothing
	void setDynamicRangeMode(ManagedSTexture::DynamicRangeMode dMode = ManagedSTexture::LINEAR) {dynamicRangeMode = dMode;}
	
	//! Register a new image loader for a given image file extension
	void registerImageLoader(const std::string& fileExtension, ImageLoader* loader)
	{
		imageLoaders[fileExtension] = loader;
	}
private:
	friend class ManagedSTexture;

	//! Internal
	ManagedSTextureSP initTex(const std::string& fullPath);

	//! Load the image memory. If we use threaded loading, the texture will
	//! be uploaded to openGL memory at the next update() call.
	bool loadImage(ManagedSTexture* tex);
	
	//! Load the texture already in the RAM to openGL memory
	bool glLoadTexture(ManagedSTexture* tex);
	
	//! Adapt the scaling for the texture. Return true if there was no errors
	//! This method is thread safe
	bool reScale(ManagedSTexture* tex);

	// List of image loaders providing image loading for the given files extensions
	std::map<std::string, ImageLoader*> imageLoaders;
	
	std::string textureDir;
	bool mipmapsMode;
	GLint wrapMode;
	GLint minFilter;
	GLint magFilter;
	ManagedSTexture::DynamicRangeMode dynamicRangeMode;
	
	// The maximum texture size supported by the video card
	GLint maxTextureSize;
	
	// Whether GL_ARB_texture_float is supported on this card
	bool isFloatingPointTexAllowed;
	
	// Whether ARB_texture_non_power_of_two is supported on this card
	bool isNoPowerOfTwoAllowed;
	bool isNoPowerOfTwoLUMINANCEAllowed;
	
	// Everything used for the threaded loading
	friend struct loadTextureThread;
	boost::mutex loadQueueMutex;
	std::vector<class LoadQueueParam*> loadQueue;
	
	// Define a PNG loader. This implementation supports LUMINANCE, LUMINANCE+ALPHA, RGB, RGBA. 
	class PngLoader : public ImageLoader
	{
		virtual bool loadImage(const std::string& filename, ManagedSTexture& texinfo);
	};
	static PngLoader pngLoader;
	
	// Define a JPG loader. This implementation supports LUMINANCE or RGB.
	class JpgLoader : public ImageLoader
	{
		virtual bool loadImage(const std::string& filename, ManagedSTexture& texinfo);
	};
	static JpgLoader jpgLoader;
};

#endif /*STELTEXTUREMGR_H_*/
