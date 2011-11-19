#ifndef LINAL_TIMER_H
#define LINAL_TIMER_H
/* -*- charset: utf-8 -*- */
/*$Id$*/

/* Copyright (c) 2009-2010 Alexey Ozeritsky (Алексей Озерицкий)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * @defgroup misc Miscellaneous functions and classes.
 * @{
 */
/**
 * Returns the number of seconds since epoch.
 * @return the number of seconds since epoch.
 */
double get_full_time();

/** @} */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * @ingroup misc
 * Timer class.
 */
class Timer
{
	double t1_;

public:
	/**
	 * Default constructor.
	 */
	Timer() : t1_ (get_full_time() ) {}

	/**
	 * @return the number of seconds from timer initialize or restart.
	 */
	double elapsed()
	{
		return (get_full_time() - t1_) / 100.0;
	}

	/**
	 * Restart timer.
	 */
	void restart()
	{
		t1_ = get_full_time();
	}
};
#endif

#endif /* LINAL_TIMER_H */
