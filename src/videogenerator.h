#ifndef VIDEOGENERATOR_H
#define VIDEOGENERATOR_H

#include <QSize>

#include "lyrics.h"
#include "project.h"

class VideoGenerator
{
	public:
		VideoGenerator( Project * prj );
		void generate( const Lyrics& lyrics, qint64 total_length, const QString& outfile );

	private:
		Project *	m_project;
		QSize		m_size;
		int			m_fps;
};

#endif // VIDEOGENERATOR_H
