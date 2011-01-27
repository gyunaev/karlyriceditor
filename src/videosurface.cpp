#include <QtMultimedia/QVideoSurfaceFormat>
#include <QMessageBox>
#include "videosurface.h"


VideoSurface::VideoSurface( QObject *parent )
	: QAbstractVideoSurface( parent )
{
}


QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats(
		QAbstractVideoBuffer::HandleType handleType) const
{
	if ( handleType == QAbstractVideoBuffer::NoHandle ) {
		return QList<QVideoFrame::PixelFormat>()
				<< QVideoFrame::Format_RGB32
				<< QVideoFrame::Format_ARGB32
				<< QVideoFrame::Format_ARGB32_Premultiplied
				<< QVideoFrame::Format_RGB565
				<< QVideoFrame::Format_RGB555;
	} else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool VideoSurface::isFormatSupported( const QVideoSurfaceFormat &format, QVideoSurfaceFormat *) const
{
	const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat( format.pixelFormat() );
	const QSize size = format.frameSize();

	return imageFormat != QImage::Format_Invalid
			&& !size.isEmpty()
			&& format.handleType() == QAbstractVideoBuffer::NoHandle;
}

bool VideoSurface::start( const QVideoSurfaceFormat &format )
{
	const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
	const QSize size = format.frameSize();

	if ( imageFormat == QImage::Format_Invalid || size.isEmpty() )
		return false;

	if ( format.scanLineDirection() == QVideoSurfaceFormat::BottomToTop )
	{
		QMessageBox::critical( 0, "Unsupported format", "Scan line format is not supported" );
		return false;
	}

	QAbstractVideoSurface::start(format);
	return true;
}

void VideoSurface::stop()
{
	QAbstractVideoSurface::stop();
}

bool VideoSurface::present( const QVideoFrame &frame )
{
	QVideoFrame framecopy = frame;

	if ( framecopy.map(QAbstractVideoBuffer::ReadOnly) )
	{
/*		QPainter painter( &m_image );
		const QTransform oldTransform = painter.transform();

		if ( surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop )
		{
		   painter.scale(1, -1);
		   painter.translate( 0, -m_image.height() );
		}
*/
		m_image = QImage( framecopy.bits(),
						  framecopy.width(),
						  framecopy.height(),
						  framecopy.bytesPerLine(),
						  QVideoFrame::imageFormatFromPixelFormat( framecopy.pixelFormat() ) );

		framecopy.unmap();
	}

	return true;
}
