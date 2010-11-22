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

#include <cstdlib>
#include "StelTextureMgr.hpp"
#include "StelTexture.hpp"
#include "glues.h"
#include "StelFileMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"

#include <QThread>
#include <QMutexLocker>
#include <QSemaphore>
#include <QImageReader>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QSize>
#include <QHttp>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>
#include <QGLWidget>

// Just for testing.  Contains all the active imageloaders.
static QSet<ImageLoader*> test;

ImageLoader::ImageLoader(const QString& path, int delay)
    : QObject(), path(path), networkReply(NULL)
{
    QTimer::singleShot(delay, this, SLOT(start()));
}

ImageLoader::~ImageLoader() {
    if (networkReply != NULL) {
        qDebug() << "TEST abort";
        networkReply->abort();
    }
    test.remove(this);
    qDebug() << "TEST nb Loaders =" << test.size();
    foreach(ImageLoader* loader, test) {
        if (loader->networkReply)
            qDebug() << loader->path << loader->networkReply->isRunning();
    }
}

void ImageLoader::start()
{
    // We limit the number of loaders downloading from internet.
    // XXX: this is just for testing.
//    if (path.startsWith("http://") && test.size() >= 4) {
//        QTimer::singleShot(500, this, SLOT(start()));
//        return;
//    }

    if (path.startsWith("http://")) {
        QNetworkRequest req = QNetworkRequest(QUrl(path));
        // Define that preference should be given to cached files (no etag checks)
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        req.setRawHeader("User-Agent", StelUtils::getApplicationName().toAscii());
        networkReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
        connect(networkReply, SIGNAL(finished()), this, SLOT(onNetworkReply()));
        connect(networkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
        connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress()));
    } else {
        // At next loop iteration we start to load from the file.
        QTimer::singleShot(0, this, SLOT(directLoad()));
    }
    test.insert(this);
    qDebug() << "TEST nb Loaders =" << test.size();

    // Move this object outside of the main thread.
    StelTextureMgr* textureMgr = &StelApp::getInstance().getTextureManager();
    moveToThread(textureMgr->loaderThread);
}


void ImageLoader::onNetworkReply()
{
    qDebug() << "TEST network reply" << path;
    if (networkReply->error() != QNetworkReply::NoError) {
        qDebug() << "ERROR" << networkReply->errorString();
        emit error(networkReply->errorString());
    } else {
        QByteArray data = networkReply->readAll();
        QImage image = QImage::fromData(data);
        if (image.isNull()) {
            qDebug() << "ERROR parsing image failed " << path;
        } else {
            emit finished(image);
        }
    }
    networkReply->deleteLater();
    networkReply = NULL;
    qDebug() << "TEST done";
}

void ImageLoader::onNetworkError(QNetworkReply::NetworkError code)
{
    qDebug() << "ERROR" << code << networkReply->errorString() << networkReply->url();
    // emit error(networkReply->errorString());
}

void ImageLoader::onDownloadProgress()
{
    qDebug() << "TEST download progress for" << path;
}

void ImageLoader::directLoad() {
    QImage image = QImage(path);
    emit finished(image);
}


StelTexture::StelTexture() : loader(NULL), downloaded(false), isLoadingImage(false),
				   errorOccured(false), id(0), avgLuminance(-1.f)
{
	width = -1;
	height = -1;
}

StelTexture::~StelTexture()
{
	if (id != 0)
	{
		StelPainter::makeMainGLContextCurrent();
		if (glIsTexture(id)==GL_FALSE)
		{
			qDebug() << "WARNING: in StelTexture::~StelTexture() tried to delete invalid texture with ID=" << id << " Current GL ERROR status is " << glGetError();
		}
		else
		{
			StelPainter::glContext->deleteTexture(id);
		}
		id = 0;
	}
	if (loader != NULL) {
		loader->deleteLater();
		loader = NULL;
	}
}

/*************************************************************************
 This method should be called if the texture loading failed for any reasons
 *************************************************************************/
void StelTexture::reportError(const QString& aerrorMessage)
{
	errorOccured = true;
	errorMessage = aerrorMessage;
	// Report failure of texture loading
	emit(loadingProcessFinished(true));
}

/*************************************************************************
 Bind the texture so that it can be used for openGL drawing (calls glBindTexture)
 *************************************************************************/
bool StelTexture::bind()
{
	// qDebug() << "TEST bind" << fullPath;
	if (id != 0)
	{
		// The texture is already fully loaded, just bind and return true;
#ifdef USE_OPENGL_ES2
		glActiveTexture(GL_TEXTURE0);
#endif
		glBindTexture(GL_TEXTURE_2D, id);
		return true;
	}
	if (errorOccured)
		return false;

	if (!isLoadingImage && loader == NULL) {
		isLoadingImage = true;
		loader = new ImageLoader(fullPath, 100);
		connect(loader, SIGNAL(finished(QImage)), this, SLOT(onImageLoaded(QImage)));
	}

	return false;
}

void StelTexture::onImageLoaded(QImage image)
{
	qImage = image;
	Q_ASSERT(!qImage.isNull());
	glLoad();
	isLoadingImage = false;
	loader->deleteLater();
	loader = NULL;
}

/*************************************************************************
 Return the width and heigth of the texture in pixels
*************************************************************************/
bool StelTexture::getDimensions(int &awidth, int &aheight)
{
	if (width<0 || height<0)
	{
		if (!qImage.isNull())
		{
			width = qImage.width();
			height = qImage.height();
		}
		else
		{
			// Try to get the size from the file without loading data
			QImageReader im(fullPath);
			if (!im.canRead())
			{
				return false;
			}
			QSize size = im.size();
			width = size.width();
			height = size.height();
		}
	}
	awidth = width;
	aheight = height;
	return true;
}

// Actually load the texture to openGL memory
bool StelTexture::glLoad()
{
	if (qImage.isNull())
	{
		errorOccured = true;
		reportError("Unknown error");
		return false;
	}

	QGLContext::BindOptions opt = QGLContext::InvertedYBindOption;
	if (loadParams.filtering==GL_LINEAR)
		opt |= QGLContext::LinearFilteringBindOption;

	// Mipmap seems to be pretty buggy on windows..
#ifndef Q_OS_WIN
	if (loadParams.generateMipmaps==true)
		opt |= QGLContext::MipmapBindOption;
#endif

	GLint glformat;
	if (qImage.isGrayscale())
	{
		glformat = qImage.hasAlphaChannel() ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
	}
	else if (qImage.hasAlphaChannel())
	{
		glformat = GL_RGBA;
	}
	else
		glformat = GL_RGB;

	Q_ASSERT(StelPainter::glContext==QGLContext::currentContext());
#ifdef USE_OPENGL_ES2
	glActiveTexture(GL_TEXTURE0);
#endif
	id = StelPainter::glContext->bindTexture(qImage, GL_TEXTURE_2D, glformat, opt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, loadParams.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, loadParams.wrapMode);

	// Release shared memory
	qImage = QImage();

	// Report success of texture loading
	emit(loadingProcessFinished(false));
	return true;
}
