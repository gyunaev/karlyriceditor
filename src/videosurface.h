#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QRect>
#include <QImage>
#include <QtMultimedia/QAbstractVideoSurface>
#include <QtMultimedia/QVideoFrame>


class VideoSurface : public QAbstractVideoSurface
{
	Q_OBJECT

	public:
		VideoSurface( QObject *parent = 0 );

		// Overridden function
		QList<QVideoFrame::PixelFormat> supportedPixelFormats(
			QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;

		// Overridden function
		bool isFormatSupported(const QVideoSurfaceFormat &format, QVideoSurfaceFormat *similar) const;

		// Overridden function
		bool start( const QVideoSurfaceFormat &format );

		// Overridden function
		void stop();

		// Overridden function
		bool present( const QVideoFrame &frame );

/*		QRect videoRect() const { return targetRect; }
		void updateVideoRect();

		void paint(QPainter *painter);*/

	private:
		QImage			m_image;
};


#endif // VIDEOSURFACE_H
