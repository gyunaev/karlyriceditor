#include <QTime>

#include "videogeneratorthread.h"
#include "editor.h"

VideoGeneratorThread::VideoGeneratorThread(FFMpegVideoEncoder *encoder, TextRenderer *renderer, qint64 total_length, qint64 timestep )
    : QThread(0)
{
    mEncoder = encoder;
    mTextRenderer = renderer;
    mTotalLength = total_length;
    mTimeStep = timestep;
    mAborted = 0;
}

VideoGeneratorThread::~VideoGeneratorThread()
{
    delete mEncoder;
    delete mTextRenderer;
}

QImage VideoGeneratorThread::currentImage() const
{
    QMutexLocker m( &mCurrentImageMutex );
    return mCurrentImage;
}

void VideoGeneratorThread::abort()
{
    mAborted = 1;
}

void VideoGeneratorThread::run()
{
    qint64 dialog_step = mTotalLength / 100;
    qint64 time = 0;

    int frames = 0, totalframes = mTotalLength  / mTimeStep;
    QTime timing, total;
    QString finishedMsg;

    timing.start();

    // Rendering
    while ( time < mTotalLength )
    {
        // Show no message in case of aborted
        if ( mAborted )
        {
            finishedMsg = "Aborted by user";
            break;
        }

        frames++;
        mTextRenderer->update( time );
        QImage image = mTextRenderer->image();

        int ret = mEncoder->encodeImage( image, time );

        if ( ret < 0 )
        {
            finishedMsg = QString("Encoding error while creating the video file" );
            break;
        }

        // Should we update the progress dialog?
        if ( time == 0 || timing.elapsed() > 1000 )
        {
            timing.restart();

            // Save the progress image
            mCurrentImageMutex.lock();
            mCurrentImage = image;
            mCurrentImageMutex.unlock();

            emit progress(
                        time / dialog_step,
                        QString("%1 of %2") .arg( frames ) .arg( totalframes ),
                        QString( "%1 Mb" ) .arg( ret / (1024*1024) ),
                        markToTime( total.elapsed() ) );
        }

        time += mTimeStep;
    }

    mEncoder->close();

    emit finished( finishedMsg );
}
