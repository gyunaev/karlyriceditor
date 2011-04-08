#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>

class AudioPlayerPrivate;


// Must be a single instance
class AudioPlayer : public QObject
{
	Q_OBJECT

	public:
		AudioPlayer();
		virtual ~AudioPlayer();

		bool	init();
		bool	open( const QString& filename );
		bool	isPlaying() const;
		qint64	currentTime() const;
		qint64	totalTime() const;
		QString	errorMsg() const;

	signals:
		void	tick( qint64 tickvalue );

	public slots:
		void	play();
		void	reset();
		void	stop();
		void	seekTo( qint64 value );

	private:
		friend class AudioPlayerPrivate;
		void	emitTickSignal( qint64 tickvalue );

		AudioPlayerPrivate *	d;
};

extern AudioPlayer  * pAudioPlayer;

#endif // AUDIOPLAYER_H
