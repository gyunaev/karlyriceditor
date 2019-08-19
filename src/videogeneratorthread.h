#ifndef VIDEOGENERATORTHREAD_H
#define VIDEOGENERATORTHREAD_H

#include <QThread>
#include <QMutex>
#include <QImage>

#include "ffmpegvideoencoder.h"
#include "textrenderer.h"


class VideoGeneratorThread : public QThread
{
    Q_OBJECT

    public:
        explicit VideoGeneratorThread(FFMpegVideoEncoder * encoder, TextRenderer * renderer, qint64 total_length, qint64 timestep);
        ~VideoGeneratorThread();

        QImage currentImage() const;

    signals:
        void    progress( int progress, QString frames, QString size, QString timing );
        void    finished( QString errortext );

    public slots:
        void    abort();

    protected:
        void run() override;

    private:
        mutable QMutex      mCurrentImageMutex;
        QImage      mCurrentImage;

        FFMpegVideoEncoder *    mEncoder;
        TextRenderer *          mTextRenderer;
        qint64                  mTimeStep;
        qint64                  mTotalLength;
        QAtomicInt              mAborted;
};

#endif // VIDEOGENERATORTHREAD_H
