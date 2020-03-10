/*
Copyright 2004-2020 Kendall F. Morris

This file is part of the Scope software suite.

    The Scope software suite is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    The suite is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the suite.  If not, see <https://www.gnu.org/licenses/>.
*/


typedef struct
{
  float min_bpm;
  float max_bpm;
  int surrogate_count;
  unsigned char palette;
  unsigned global : 1;
  unsigned threshold : 1;
  unsigned normal : 1;
  unsigned log : 1;
  unsigned envelope : 1;
  unsigned hist_on : 1;
  unsigned save_on : 1;
  unsigned gaussian : 1;
  unsigned empirical : 1;
  unsigned p01 : 1;
  unsigned p05 : 1;
  unsigned p50 : 1;
  unsigned p95 : 1;
  unsigned p99 : 1;
  unsigned legs : 1;
} BandpassParams;

double bandpass (BandpassParams *bp);

