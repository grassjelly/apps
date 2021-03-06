/****************************************************************************
 * apps/graphics/pdcurses/pdc_pad.c
 * Public Domain Curses
 * RCSID("$Id: pad.c,v 1.50 2008/07/14 12:22:13 wmcbrine Exp $")
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the 3-clause BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/* Name: pad
 *
 * Synopsis:
 *       WINDOW *newpad(int nlines, int ncols);
 *       WINDOW *subpad(WINDOW *orig, int nlines, int ncols,
 *                      int begy, int begx);
 *       int prefresh(WINDOW *win, int py, int px, int sy1, int sx1,
 *                    int sy2, int sx2);
 *       int pnoutrefresh(WINDOW *w, int py, int px, int sy1, int sx1,
 *                        int sy2, int sx2);
 *       int pechochar(WINDOW *pad, chtype ch);
 *       int pecho_wchar(WINDOW *pad, const cchar_t *wch);
 *
 * Description:
 *       A pad is a special kind of window, which is not restricted by
 *       the screen size, and is not necessarily associated with a
 *       particular part of the screen.  You can use a pad when you need
 *       a large window, and only a part of the window will be on the
 *       screen at one time.  Pads are not refreshed automatically (e.g.,
 *       from scrolling or echoing of input).  You can't call wrefresh()
 *       with a pad as an argument; use prefresh() or pnoutrefresh()
 *       instead.  Note that these routines require additional parameters
 *       to specify the part of the pad to be displayed, and the location
 *       to use on the screen.
 *
 *       newpad() creates a new pad data structure.
 *
 *       subpad() creates a new sub-pad within a pad, at position (begy,
 *       begx), with dimensions of nlines lines and ncols columns. This
 *       position is relative to the pad, and not to the screen as with
 *       subwin.  Changes to either the parent pad or sub-pad will affect
 *       both.  When using sub-pads, you may need to call touchwin()
 *       before calling prefresh().
 *
 *       pnoutrefresh() copies the specified pad to the virtual screen.
 *
 *       prefresh() calls pnoutrefresh(), followed by doupdate().
 *
 *       These routines are analogous to wnoutrefresh() and wrefresh().
 *       (py, px) specifies the upper left corner of the part of the pad
 *       to be displayed; (sy1, sx1) and (sy2, sx2) describe the screen
 *       rectangle that will contain the selected part of the pad.
 *
 *       pechochar() is functionally equivalent to addch() followed by
 *       a call to prefresh(), with the last-used coordinates and
 *       dimensions. pecho_wchar() is the wide-character version.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error.
 *
 * Portability                                X/Open    BSD    SYS V
 *       newpad                                  Y       -       Y
 *       subpad                                  Y       -       Y
 *       prefresh                                Y       -       Y
 *       pnoutrefresh                            Y       -       Y
 *       pechochar                               Y       -      3.0
 *       pecho_wchar                             Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Save values for pechochar() */

static int save_pminrow;
static int save_pmincol;
static int save_sminrow;
static int save_smincol;
static int save_smaxrow;
static int save_smaxcol;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

WINDOW *newpad(int nlines, int ncols)
{
  WINDOW *win;

  PDC_LOG(("newpad() - called: lines=%d cols=%d\n", nlines, ncols));

  if (!(win = PDC_makenew(nlines, ncols, -1, -1)) ||
      !(win = PDC_makelines(win)))
    {
      return (WINDOW *) NULL;
    }

  werase(win);
  win->_flags = _PAD;

  /* save default values in case pechochar() is the first call to prefresh(). */

  save_pminrow = 0;
  save_pmincol = 0;
  save_sminrow = 0;
  save_smincol = 0;
  save_smaxrow = min(LINES, nlines) - 1;
  save_smaxcol = min(COLS, ncols) - 1;

  return win;
}

WINDOW *subpad(WINDOW *orig, int nlines, int ncols, int begy, int begx)
{
  WINDOW *win;
  int i;
  int j = begy;
  int k = begx;

  PDC_LOG(("subpad() - called: lines=%d cols=%d begy=%d begx=%d\n",
           nlines, ncols, begy, begx));

  if (!orig || !(orig->_flags & _PAD))
    {
      return (WINDOW *) NULL;
    }

  /* Make sure window fits inside the original one */

  if ((begy < orig->_begy) || (begx < orig->_begx) ||
      (begy + nlines) > (orig->_begy + orig->_maxy) ||
      (begx + ncols) > (orig->_begx + orig->_maxx))
    {
      return (WINDOW *) NULL;
    }

  if (!nlines)
    {
      nlines = orig->_maxy - 1 - j;
    }

  if (!ncols)
    {
      ncols = orig->_maxx - 1 - k;
    }

  if (!(win = PDC_makenew(nlines, ncols, begy, begx)))
    {
      return (WINDOW *) NULL;
    }

  /* Initialize window variables */

  win->_attrs      = orig->_attrs;
  win->_leaveit    = orig->_leaveit;
  win->_scroll     = orig->_scroll;
  win->_nodelay    = orig->_nodelay;
  win->_use_keypad = orig->_use_keypad;
  win->_parent     = orig;

  for (i = 0; i < nlines; i++)
    {
      win->_y[i] = (orig->_y[j++]) + k;
    }

  win->_flags = _SUBPAD;

  /* Save default values in case pechochar() is the first call to
   * prefresh().
   */

  save_pminrow = 0;
  save_pmincol = 0;
  save_sminrow = 0;
  save_smincol = 0;
  save_smaxrow = min(LINES, nlines) - 1;
  save_smaxcol = min(COLS, ncols) - 1;

  return win;
}

int prefresh(WINDOW *win, int py, int px, int sy1, int sx1, int sy2,
             int sx2)
{
  PDC_LOG(("prefresh() - called\n"));

  if (pnoutrefresh(win, py, px, sy1, sx1, sy2, sx2) == ERR)
    {
      return ERR;
    }

  doupdate();
  return OK;
}

int pnoutrefresh(WINDOW *w, int py, int px, int sy1, int sx1, int sy2,
                 int sx2)
{
  int num_cols;
  int sline = sy1;
  int pline = py;

  PDC_LOG(("pnoutrefresh() - called\n"));

  if (!w || !(w->_flags & (_PAD | _SUBPAD)) || (sy2 >= LINES) || (sy2 >= COLS))
    {
      return ERR;
    }

  if (py < 0)
    {
      py = 0;
    }

  if (px < 0)
    {
      px = 0;
    }

  if (sy1 < 0)
    {
      sy1 = 0;
    }

  if (sx1 < 0)
    {
      sx1 = 0;
    }

  if (sy2 < sy1 || sx2 < sx1)
    {
      return ERR;
    }

  num_cols = min((sx2 - sx1 + 1), (w->_maxx - px));

  while (sline <= sy2)
    {
      if (pline < w->_maxy)
        {
          memcpy(curscr->_y[sline] + sx1, w->_y[pline] + px,
                 num_cols * sizeof(chtype));

          if ((curscr->_firstch[sline] == _NO_CHANGE) ||
              (curscr->_firstch[sline] > sx1))
            {
              curscr->_firstch[sline] = sx1;
            }

          if (sx2 > curscr->_lastch[sline])
            {
              curscr->_lastch[sline] = sx2;
            }

          w->_firstch[pline] = _NO_CHANGE;      /* updated now */
          w->_lastch[pline]  = _NO_CHANGE;      /* updated now */
        }

      sline++;
      pline++;
    }

  if (w->_clear)
    {
      w->_clear = false;
      curscr->_clear = true;
    }

  /* Position the cursor to the pad's current position if possible -- is the
   * pad current position going to end up displayed? if not, then don't move
   * the cursor; if so, move it to the correct place.
   */

  if (!w->_leaveit && w->_cury >= py && w->_curx >= px &&
      w->_cury <= py + (sy2 - sy1) && w->_curx <= px + (sx2 - sx1))
    {
      curscr->_cury = (w->_cury - py) + sy1;
      curscr->_curx = (w->_curx - px) + sx1;
    }

  return OK;
}

int pechochar(WINDOW *pad, chtype ch)
{
  PDC_LOG(("pechochar() - called\n"));

  if (waddch(pad, ch) == ERR)
    {
      return ERR;
    }

  return prefresh(pad, save_pminrow, save_pmincol, save_sminrow,
                  save_smincol, save_smaxrow, save_smaxcol);
}

#ifdef CONFIG_PDCURSES_WIDE
int pecho_wchar(WINDOW *pad, const cchar_t *wch)
{
  PDC_LOG(("pecho_wchar() - called\n"));

  if (!wch || (waddch(pad, *wch) == ERR))
    {
      return ERR;
    }

  return prefresh(pad, save_pminrow, save_pmincol, save_sminrow,
                  save_smincol, save_smaxrow, save_smaxcol);
}
#endif
