/* Common main.c for the benchmarks

   Copyright (C) 2014 Embecosm Limited and University of Bristol
   Copyright (C) 2018 Embecosm Limited

   Contributor: James Pallister <james.pallister@bristol.ac.uk>
   Contributor: Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of the Bristol/Embecosm Embedded Benchmark Suite.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include "support.h"

#include <zephyr/arch/cpu.h>

#define DEMCR (*(volatile unsigned *) 0xE000EDFC)
#define DWT_CTRL (*(volatile unsigned *) 0xe0001000)
#define DWT_CYCCNT (*(volatile unsigned *) 0xe0001004)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

int
main (int   argc __attribute__ ((unused)),
      char *argv[] __attribute__ ((unused)) )
{
  int i;
  volatile int result;

  arm_irq_disable(SysTick_IRQn);

  DEMCR |= (1 << 24);
  DWT_CYCCNT = 0;
  DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  unsigned start = DWT_CYCCNT;
  for (i = 0; i < REPEAT_FACTOR; i++)
    {
      result = benchmark ();
    }

  unsigned end = DWT_CYCCNT;
  printf("ctl-vector cycle count: %u\n", end-start);

  arm_irq_enable(SysTick_IRQn);
  /* bmarks that use arrays will check a global array rather than int result */

  return 0;

}	/* main () */

/*
   Local Variables:
   mode: C++
   c-file-style: "gnu"
   End:
*/
