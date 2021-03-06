/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS For Embedded Optimization.                                                      *
* Copyright (C) 2016-2018 by Gianluca Frison.                                                     *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* This program is free software: you can redistribute it and/or modify                            *
* it under the terms of the GNU General Public License as published by                            *
* the Free Software Foundation, either version 3 of the License, or                               *
* (at your option) any later version                                                              *.
*                                                                                                 *
* This program is distributed in the hope that it will be useful,                                 *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                                  *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                                   *
* GNU General Public License for more details.                                                    *
*                                                                                                 *
* You should have received a copy of the GNU General Public License                               *
* along with this program.  If not, see <https://www.gnu.org/licenses/>.                          *
*                                                                                                 *
* The authors designate this particular file as subject to the "Classpath" exception              *
* as provided by the authors in the LICENSE file that accompained this code.                      *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/

#include "../../include/blasfeo_common.h"
#include "../../include/blasfeo_d_aux.h"
#include "../../include/blasfeo_d_kernel.h"



// m>=1 and n={5,6,7,8}
void kernel_dgetrf_pivot_8_vs_lib(int m, double *C, int ldc, double *pd, int* ipiv, int n)
	{

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	double pU0[3*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	double pU0[2*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#elif defined(TARGET_GENERIC)
	double pU0[1*4*K_MAX_STACK];
#else
	double pU0[1*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#endif
	int sdu0 = (m+3)/4*4;
	sdu0 = sdu0<K_MAX_STACK ? sdu0 : K_MAX_STACK;

	double *pU = pU0;
	int sdu = sdu0;

	double *tmp_pU;
	int m4;

	if(m>K_MAX_STACK)
		{
		m4 = (m+3)/4*4;
		tmp_pU = malloc(3*4*m4*sizeof(double)+64);
		blasfeo_align_64_byte(tmp_pU, (void **) &pU);
		sdu = m4;
		}
	else
		{
		pU = pU0;
		sdu = sdu0;
		}

	int ii;

	double *dummy = NULL;

	double d1 = 1.0;
	double dm1 = -1.0;

	// saturate n to 8
	n = 8<n ? 8 : n;

	int p = m<n ? m : n;

	int n_max;

	// fact left column
	kernel_dgetrf_pivot_4_vs_lib(m, C, ldc, pd, ipiv, n);

	n_max = p<4 ? p : 4;

	// apply pivot to right column
	for(ii=0; ii<n_max; ii++)
		{
		if(ipiv[ii]!=ii)
			{
			kernel_drowsw_lib(n-4, C+ii+4*ldc, ldc, C+ipiv[ii]+4*ldc, ldc);
			}
		}

	// pack
	kernel_dpack_tn_4_vs_lib4(4, C+4*ldc, ldc, pU+4*sdu, n-4);

	// solve top right block
	kernel_dtrsm_nt_rl_one_4x4_vs_lib4c44c(0, dummy, dummy, 0, &d1, pU+4*sdu, pU+4*sdu, C, ldc, n-4, m);

	// unpack
	kernel_dunpack_nt_4_vs_lib4(4, pU+4*sdu, C+4*ldc, ldc, n-4);

	if(m>4)
		{

		// correct rigth block
		ii = 4;
		// TODO larger kernels ???
		for(; ii<m; ii+=4)
			{
			kernel_dgemm_nt_4x4_vs_libc4cc(4, &dm1, C+ii, ldc, pU+4*sdu, &d1, C+ii+4*ldc, ldc, C+ii+4*ldc, ldc, m-ii, n-4);
			}

		// fact right column
		kernel_dgetrf_pivot_4_vs_lib(m-4, C+4+4*ldc, ldc, pd+4, ipiv+4, n-4);

		n_max = p;

		for(ii=4; ii<n_max; ii++)
			ipiv[ii] += 4;

		// apply pivot to left column
		for(ii=4; ii<n_max; ii++)
			{
			if(ipiv[ii]!=ii)
				{
				kernel_drowsw_lib(4, C+ii, ldc, C+ipiv[ii], ldc);
				}
			}

		}

	end:
	if(m>K_MAX_STACK)
		{
		free(tmp_pU);
		}

	return;

	}




