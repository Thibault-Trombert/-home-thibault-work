/*
 This file is part of DMGBoy.
 
 DMGBoy is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DMGBoy is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with DMGBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <wx/thread.h>
#include "SoundPortaudio.h"

static bool paInitialized = false;

SoundPortaudio::SoundPortaudio()
{
	m_bufs = NULL;
	m_semaphore = NULL;
	m_soundOpen = false;
    m_fullBuffers = 0;
}

SoundPortaudio::~SoundPortaudio()
{
	Stop();
}

bool SoundPortaudio::Start(long sampleRate, int numChannels)
{
	assert( !m_bufs ); // can only be initialized once
	
    PaError err;
	m_writeBuf = 0;
	m_writePos = 0;
	m_readBuf = 0;
	
	m_bufs = new short[(long) bufSize * numBuffers];
	if (!m_bufs)
		return false;
	
    m_fullBuffers = 0;
	m_semaphore = new wxSemaphore(numBuffers - 1);
    m_mutex = new wxMutex();
	
    if (!paInitialized)
    {
        err = Pa_Initialize();
        if( err != paNoError )
        {
            printf("PortAudio error: %s\n", Pa_GetErrorText( err ));
            return false;
        }
        
        paInitialized = true;
    }
    
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &m_stream,
                               0,           /* no input channels */
                               numChannels, /* mono, stereo, ..., output */
                               paInt16,     /* 16 bits output */
                               sampleRate,
                               bufSize / numChannels, /* frames per buffer, i.e. the number
                                            of sample frames that PortAudio will
                                            request from the callback. Many apps
                                            may want to use
                                            paFramesPerBufferUnspecified, which
                                            tells PortAudio to pick the best,
                                            possibly changing, buffer size.*/
                               PortaudioCallback, /* this is your callback function */
                               this ); /*This is a pointer that will be passed to
                                         your callback*/
    if( err != paNoError )
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText( err ));
        return false;
    }
    
    err = Pa_StartStream( m_stream );
    if( err != paNoError )
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText( err ));
        return false;
    }
    
	m_soundOpen = true;
	
	return true;
}

void SoundPortaudio::Stop()
{
	if (m_soundOpen)
	{
		m_soundOpen = false;
        
        PaError err;
        
		err = Pa_StopStream(m_stream);
        if(err != paNoError)
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
	}
	
	if (m_semaphore)
	{
		delete m_semaphore;
		m_semaphore = NULL;
	}
    
    if (m_mutex)
    {
        delete m_mutex;
        m_mutex = NULL;
    }
	
    if (m_bufs)
    {
        delete [] m_bufs;
        m_bufs = NULL;
    }
}

inline short* SoundPortaudio::GetBufPtr( int index )
{
	assert( (unsigned) index < numBuffers );
	return m_bufs + (long) index * bufSize;
}

void SoundPortaudio::Write(const short* in, int count)
{
	while (count)
	{
		// n = espacio disponible en el buffer actual
		int n = bufSize - m_writePos;
		// si es mayor a lo que queremos copiar
		// n = count
		if (n > count)
			n = count;
		
		// copiar en el buffer actual (write_buf) n samples
		memcpy(GetBufPtr(m_writeBuf) + m_writePos, in, n * sizeof(short));
		in += n;
		m_writePos += n;
		count -= n;
		
		// si el buffer ya está lleno
		if (m_writePos >= bufSize )
		{
			m_writePos = 0;
			// seleccionar el buffer siguiente
			m_writeBuf = (m_writeBuf + 1) % numBuffers;
			// si todos los buffers estan a la espera de ser
			// reproducidos se suspende este hilo
			m_semaphore->Wait();
            
            wxMutexLocker lock(*m_mutex);
            m_fullBuffers++;
		}
		
		// si aun no se han copiado todos los datos se hace
		// una nueva pasada
	}
}

int SoundPortaudio::FillBuffer(void *outputBuffer, unsigned long framesPerBuffer)
{
    short * out = (short *)outputBuffer;
    size_t copyBytes = framesPerBuffer*2 * sizeof(short);
    
    // si hay por lo menos un buffer para reproducir
	if ( m_fullBuffers > 0 )
	{
		memcpy(out, GetBufPtr(m_readBuf), copyBytes);
		m_readBuf = (m_readBuf + 1) % numBuffers;
        m_semaphore->Post();
        
        wxMutexLocker lock(*m_mutex);
        m_fullBuffers--;
	}
	// si no hay ninguno reproducir silencio
	else
	{
		memset(out, 0, copyBytes);
	}
    
    return 0;
}

int SoundPortaudio::PortaudioCallback( const void *inputBuffer, void *outputBuffer,
                                        unsigned long framesPerBuffer,
                                        const PaStreamCallbackTimeInfo* timeInfo,
                                        PaStreamCallbackFlags statusFlags,
                                        void *userData )
{
	return ((SoundPortaudio*) userData)->FillBuffer(outputBuffer, framesPerBuffer);
}

