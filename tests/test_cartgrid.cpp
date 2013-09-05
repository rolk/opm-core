/*
  Copyright 2012 SINTEF ICT, Applied Mathematics.
  Portions Copyright 2013 Uni Research AS.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

/* --- Boost.Test boilerplate --- */
#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define NVERBOSE  // Suppress own messages when throw()ing

#define BOOST_TEST_MODULE CartGridTest
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

/* --- our own headers --- */
#include <opm/core/grid/cart_grid.h>
#include <opm/core/grid.h>
#include <stdio.h>
#include <vector>

BOOST_AUTO_TEST_SUITE ()

BOOST_AUTO_TEST_CASE (facenumbers)
{
    int faces[] = { 0, 6, 1, 8,
                    1, 7, 2, 9,
                    3, 8, 4, 10,
                    4, 9, 5, 11 };
    struct UnstructuredGrid *g = create_grid_cart2d(2, 2, 1., 1.);
    int i;
    int k;
    for (i = 0; i < g->number_of_cells; ++i) {
        for (k = g->cell_facepos[i]; k < g->cell_facepos[i + 1]; ++k) {
            BOOST_REQUIRE_EQUAL (g->cell_faces[k], faces[k]);
        }
    }
    destroy_grid(g);
}

BOOST_AUTO_TEST_CASE (globalindex)
{
	const int nx = 2;
	const int ny = 2;
	const int nz = 2;
	std::vector <int> hits (nx*ny*nz, 0);
	struct UnstructuredGrid *g = create_grid_cart3d (nx, ny, nz);
	for (unsigned int i = 0; i < nx*ny*nz; ++i) {
		hits[g->global_cell[i]]++;
	}
	std::vector <int> ones (nx*ny*nz, 1);
	// each global index should be hit exactly once
	for (unsigned int i = 0; i < hits.size(); ++i) {
		BOOST_REQUIRE_EQUAL (hits[i], ones[i]);
	}
}

BOOST_AUTO_TEST_SUITE_END()
