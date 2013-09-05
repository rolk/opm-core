/*===========================================================================
//
// File: cart_grid.c
//
// Author: Jostein R. Natvig <Jostein.R.Natvig@sintef.no>
//
//==========================================================================*/


/*
  Copyright 2011 SINTEF ICT, Applied Mathematics.
  Copyright 2011 Statoil ASA.

  This file is part of the Open Porous Media Project (OPM).

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
#include <stdlib.h>
#include <stdio.h>

#include <opm/core/grid.h>
#include <opm/core/grid/cornerpoint_grid.h>

#include <opm/core/grid/cart_grid.h>

static struct UnstructuredGrid *allocate_cart_grid_3d(int nx, int ny, int nz);
static void fill_cart_topology_3d(struct UnstructuredGrid *G);
static void fill_cart_indices(struct UnstructuredGrid *G);
static void fill_cart_geometry_3d(struct UnstructuredGrid *G,
                                  const double            *x,
                                  const double            *y,
                                  const double            *z);
static void
fill_layered_geometry_3d(struct UnstructuredGrid *G,
                         const double            *x,
                         const double            *y,
                         const double            *z,
                         const double            *depthz);

struct UnstructuredGrid *
create_grid_cart3d(int nx, int ny, int nz)
{
    return create_grid_hexa3d(nx, ny, nz, 1.0, 1.0, 1.0);
}

struct UnstructuredGrid *
create_grid_hexa3d(int    nx, int    ny, int    nz,
                   double dx, double dy, double dz)
{
    int     i;
    double *x, *y, *z;
    struct UnstructuredGrid *G;

    x = malloc((nx + 1) * sizeof *x);
    y = malloc((ny + 1) * sizeof *y);
    z = malloc((nz + 1) * sizeof *z);

    if ((x == NULL) || (y == NULL) || (z == NULL)) {
        G = NULL;
    } else {
        for (i = 0; i < nx + 1; i++) { x[i] = i * dx; }
        for (i = 0; i < ny + 1; i++) { y[i] = i * dy; }
        for (i = 0; i < nz + 1; i++) { z[i] = i * dz; }

        G = create_grid_tensor3d(nx, ny, nz, x, y, z,
                                 (const double *) NULL);
    }

    free(z);  free(y);  free(x);

    return G;
}

/* --------------------------------------------------------------------- */

static struct UnstructuredGrid *allocate_cart_grid_2d(int nx, int ny);
static void fill_cart_topology_2d(struct UnstructuredGrid *G);
static void fill_cart_geometry_2d(struct UnstructuredGrid *G,
                                  const double            *x,
                                  const double            *y);

struct UnstructuredGrid*
create_grid_cart2d(int nx, int ny, double dx, double dy)
{
    int     i;
    double *x, *y;
    struct UnstructuredGrid *G;

    x = malloc((nx + 1) * sizeof *x);
    y = malloc((ny + 1) * sizeof *y);

    if ((x == NULL) || (y == NULL)) {
        G = NULL;
    } else {

        for (i = 0; i < nx + 1; i++) { x[i] = i*dx; }
        for (i = 0; i < ny + 1; i++) { y[i] = i*dy; }

        G = create_grid_tensor2d(nx, ny, x, y);
    }

    free(y);  free(x);

    return G;
}

/* --------------------------------------------------------------------- */

struct UnstructuredGrid *
create_grid_tensor2d(int nx, int ny, const double *x, const double *y)
{
    struct UnstructuredGrid *G;

    G = allocate_cart_grid_2d(nx, ny);

    if (G != NULL)
    {
        fill_cart_topology_2d(G);
        fill_cart_geometry_2d(G, x, y);
    }

    return G;
}

/* --------------------------------------------------------------------- */

struct UnstructuredGrid *
create_grid_tensor3d(int           nx    ,
                     int           ny    ,
                     int           nz    ,
                     const double *x     ,
                     const double *y     ,
                     const double *z     ,
                     const double *depthz)
{
    struct UnstructuredGrid *G;

    G = allocate_cart_grid_3d(nx, ny, nz);

    if (G != NULL)
    {
        fill_cart_topology_3d(G);

        if (depthz == NULL) {
            fill_cart_geometry_3d(G, x, y, z);
        }
        else {
            fill_layered_geometry_3d(G, x, y, z, depthz);
        }
    }

    return G;
}

/* --------------------------------------------------------------------- */
/* Static functions follow:                                              */
/* --------------------------------------------------------------------- */

static void
fill_cart_indices(struct UnstructuredGrid *G)
{
    int i;
    /* elements in a Cartesian grid are already arranged in         */
    /* lexicographic order, and there are no holes so the numbering */
    /* becomes continuous.                                          */
    for (i = 0; i < G->number_of_cells; ++i) {
        G->global_cell[i] = i;
    }
}

static struct UnstructuredGrid *
allocate_cart_grid(size_t ndims ,
                   size_t ncells,
                   size_t nfaces,
                   size_t nnodes)
{
    size_t nfacenodes, ncellfaces;
    struct UnstructuredGrid *g;

    nfacenodes = nfaces * (2 * (ndims - 1));
    ncellfaces = ncells * (2 * ndims);

    g = allocate_grid(ndims, ncells, nfaces,
                      nfacenodes, ncellfaces, nnodes);

    /* only proceed if we could allocate a general grid */
    if (g) {

        /* we always have global indexing in Cartesian grids */
        g->global_cell = malloc(ncells * sizeof *g->global_cell);

        /* if allocation failed, then deallocate the _entire_ grid */
        /* before returning failure.                               */
        if(!g->global_cell) {
            destroy_grid (g);
            g = NULL;
        }
    }

    return g;
}


static struct UnstructuredGrid*
allocate_cart_grid_3d(int nx, int ny, int nz)
{
    struct UnstructuredGrid *G;
    int Nx, Ny, Nz;
    int nxf, nyf, nzf;

    int ncells, nfaces, nnodes;

    Nx  = nx + 1;
    Ny  = ny + 1;
    Nz  = nz  +1;

    nxf = Nx * ny * nz;
    nyf = nx * Ny * nz;
    nzf = nx * ny * Nz;

    ncells = nx  * ny  * nz ;
    nfaces = nxf + nyf + nzf;
    nnodes = Nx  * Ny  * Nz ;

    G = allocate_cart_grid(3, ncells, nfaces, nnodes);

    if (G != NULL)
    {
        G->dimensions  = 3 ;
        G->cartdims[0] = nx;
        G->cartdims[1] = ny;
        G->cartdims[2] = nz;

        G->number_of_cells  = ncells;
        G->number_of_faces  = nfaces;
        G->number_of_nodes  = nnodes;
    }

    return G;
}

/* --------------------------------------------------------------------- */

static void
fill_cart_topology_3d(struct UnstructuredGrid *G)
{
    int nx, ny, nz;
    int Nx, Ny;
    int nxf, nyf;
    int i,j,k;

    int *cfaces, *cfacepos, *fnodes, *fnodepos, *fcells;

    nx = G->cartdims[0];
    ny = G->cartdims[1];
    nz = G->cartdims[2];

    Nx  = nx+1;
    Ny  = ny+1;

    nxf = Nx*ny*nz;
    nyf = nx*Ny*nz;

    cfaces     = G->cell_faces;
    cfacepos   = G->cell_facepos;
    cfacepos[0] = 0;
    for (k=0; k<nz; ++k)  {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx; ++i) {
                *cfaces++ = i+  Nx*(j+  ny* k   );
                *cfaces++ = i+1+Nx*(j+  ny* k   );
                *cfaces++ = i+  nx*(j+  Ny* k   )  +nxf;
                *cfaces++ = i+  nx*(j+1+Ny* k   )  +nxf;
                *cfaces++ = i+  nx*(j+  ny* k   )  +nxf+nyf;
                *cfaces++ = i+  nx*(j+  ny*(k+1))  +nxf+nyf;

                cfacepos[1] = cfacepos[0]+6;
                ++cfacepos;
            }
        }
    }

    for (k = 0; k < nx * ny * nz; ++k) {
        for (i = 0; i < 6; ++i) {
            G->cell_facetag[k*6 + i] = i;
        }
    }

    fnodes      = G->face_nodes;
    fnodepos    = G->face_nodepos;
    fcells      = G->face_cells;
    fnodepos[0] = 0;

    /* Faces with x-normal */
    for (k=0; k<nz; ++k) {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx+1; ++i) {
                *fnodes++ = i+Nx*(j   + Ny * k   );
                *fnodes++ = i+Nx*(j+1 + Ny * k   );
                *fnodes++ = i+Nx*(j+1 + Ny *(k+1));
                *fnodes++ = i+Nx*(j   + Ny *(k+1));
                fnodepos[1] = fnodepos[0] + 4;
                ++fnodepos;
                if (i==0) {
                    *fcells++ = -1;
                    *fcells++ =  i+nx*(j+ny*k);
                }
                else if (i == nx) {
                    *fcells++ =  i-1+nx*(j+ny*k);
                    *fcells++ = -1;
                }
                else {
                    *fcells++ =  i-1 + nx*(j+ny*k);
                    *fcells++ =  i   + nx*(j+ny*k);
                }
            }
        }
    }
    /* Faces with y-normal */
    for (k=0; k<nz; ++k) {
        for (j=0; j<ny+1; ++j) {
            for (i=0; i<nx; ++i) {
                *fnodes++ = i+    Nx*(j + Ny * k   );
                *fnodes++ = i   + Nx*(j + Ny *(k+1));
                *fnodes++ = i+1 + Nx*(j + Ny *(k+1));
                *fnodes++ = i+1 + Nx*(j + Ny * k   );
                fnodepos[1] = fnodepos[0] + 4;
                ++fnodepos;
                if (j==0) {
                    *fcells++ = -1;
                    *fcells++ =  i+nx*(j+ny*k);
                }
                else if (j == ny) {
                    *fcells++ =  i+nx*(j-1+ny*k);
                    *fcells++ = -1;
                }
                else {
                    *fcells++ =  i+nx*(j-1+ny*k);
                    *fcells++ =  i+nx*(j+ny*k);
                }
            }
        }
    }
    /* Faces with z-normal */
    for (k=0; k<nz+1; ++k) {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx; ++i) {
                *fnodes++ = i+    Nx*(j   + Ny * k);
                *fnodes++ = i+1 + Nx*(j   + Ny * k);
                *fnodes++ = i+1 + Nx*(j+1 + Ny * k);
                *fnodes++ = i+    Nx*(j+1 + Ny * k);
                fnodepos[1] = fnodepos[0] + 4;
                ++fnodepos;
                if (k==0) {
                    *fcells++ = -1;
                    *fcells++ =  i+nx*(j+ny*k);
                }
                else if (k == nz) {
                    *fcells++ =  i+nx*(j+ny*(k-1));
                    *fcells++ = -1;
                }
                else {
                    *fcells++ =  i+nx*(j+ny*(k-1));
                    *fcells++ =  i+nx*(j+ny*k);
                }
            }
        }
    }

    fill_cart_indices(G);
}

/* --------------------------------------------------------------------- */

static void
fill_cart_geometry_3d(struct UnstructuredGrid *G,
                      const double            *x,
                      const double            *y,
                      const double            *z)
{
    int nx, ny, nz;
    int i,j,k;

    double dx, dy, dz;

    double *coord, *ccentroids, *cvolumes;
    double *fnormals, *fcentroids, *fareas;

    nx  = G->cartdims[0];
    ny  = G->cartdims[1];
    nz  = G->cartdims[2];

    ccentroids = G->cell_centroids;
    cvolumes   = G->cell_volumes;
    for (k=0; k<nz; ++k)  {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx; ++i) {
                *ccentroids++ = (x[i] + x[i + 1]) / 2.0;
                *ccentroids++ = (y[j] + y[j + 1]) / 2.0;
                *ccentroids++ = (z[k] + z[k + 1]) / 2.0;

                dx = x[i + 1] - x[i];
                dy = y[j + 1] - y[j];
                dz = z[k + 1] - z[k];

                *cvolumes++ = dx * dy * dz;
            }
        }
    }


    fnormals   = G->face_normals;
    fcentroids = G->face_centroids;
    fareas     = G->face_areas;

    /* Faces with x-normal */
    for (k=0; k<nz; ++k) {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx+1; ++i) {
                dy = y[j + 1] - y[j];
                dz = z[k + 1] - z[k];

                *fnormals++ = dy * dz;
                *fnormals++ = 0;
                *fnormals++ = 0;

                *fcentroids++ = x[i];
                *fcentroids++ = (y[j] + y[j + 1]) / 2.0;
                *fcentroids++ = (z[k] + z[k + 1]) / 2.0;

                *fareas++ = dy * dz;
            }
        }
    }
    /* Faces with y-normal */
    for (k=0; k<nz; ++k) {
        for (j=0; j<ny+1; ++j) {
            for (i=0; i<nx; ++i) {
                dx = x[i + 1] - x[i];
                dz = z[k + 1] - z[k];

                *fnormals++ = 0;
                *fnormals++ = dx * dz;
                *fnormals++ = 0;

                *fcentroids++ = (x[i] + x[i + 1]) / 2.0;
                *fcentroids++ = y[j];
                *fcentroids++ = (z[k] + z[k + 1]) / 2.0;

                *fareas++ = dx * dz;
            }
        }
    }
    /* Faces with z-normal */
    for (k=0; k<nz+1; ++k) {
        for (j=0; j<ny; ++j) {
            for (i=0; i<nx; ++i) {
                dx = x[i + 1] - x[i];
                dy = y[j + 1] - y[j];

                *fnormals++ = 0;
                *fnormals++ = 0;
                *fnormals++ = dx * dy;

                *fcentroids++ = (x[i] + x[i + 1]) / 2.0;
                *fcentroids++ = (y[j] + y[j + 1]) / 2.0;
                *fcentroids++ = z[k];

                *fareas++ = dx * dy;
            }
        }
    }

    coord = G->node_coordinates;
    for (k=0; k<nz+1; ++k) {
        for (j=0; j<ny+1; ++j) {
            for (i=0; i<nx+1; ++i) {
                *coord++ = x[i];
                *coord++ = y[j];
                *coord++ = z[k];
            }
        }
    }
}

/* ---------------------------------------------------------------------- */

static void
fill_layered_geometry_3d(struct UnstructuredGrid *G,
                         const double            *x,
                         const double            *y,
                         const double            *z,
                         const double            *depthz)
{
    int i , j , k ;
    int nx, ny, nz;

    const double *depth;
    double       *coord;

    nx = G->cartdims[0];  ny = G->cartdims[1];  nz = G->cartdims[2];

    coord = G->node_coordinates;
    for (k = 0; k < nz + 1; k++) {

        depth = depthz;

        for (j = 0; j < ny + 1; j++) {
            for (i = 0; i < nx + 1; i++) {
                *coord++ = x[i];
                *coord++ = y[j];
                *coord++ = z[k] + *depth++;
            }
        }
    }

    compute_geometry(G);
}

/* --------------------------------------------------------------------- */

static struct UnstructuredGrid*
allocate_cart_grid_2d(int nx, int ny)
{
    int nxf, nyf;
    int Nx , Ny ;

    int ncells, nfaces, nnodes;

    struct UnstructuredGrid *G;

    Nx     = nx + 1;
    Ny     = ny + 1;

    nxf    = Nx * ny;
    nyf    = nx * Ny;

    ncells = nx  * ny ;
    nfaces = nxf + nyf;
    nnodes = Nx  * Ny ;

    G = allocate_cart_grid(2, ncells, nfaces, nnodes);

    if (G != NULL)
    {
        G->dimensions  = 2 ;
        G->cartdims[0] = nx;
        G->cartdims[1] = ny;
        G->cartdims[2] = 1 ;

        G->number_of_cells = ncells;
        G->number_of_faces = nfaces;
        G->number_of_nodes = nnodes;
    }

    return G;
}

/* --------------------------------------------------------------------- */

static void
fill_cart_topology_2d(struct UnstructuredGrid *G)
{
    int    i,j;
    int    nx, ny;
    int    nxf;
    int    Nx;

    int    *fnodes, *fnodepos, *fcells, *cfaces, *cfacepos;

    cfaces     = G->cell_faces;
    cfacepos   = G->cell_facepos;

    nx  = G->cartdims[0];
    ny  = G->cartdims[1];
    Nx  = nx + 1;
    nxf = Nx * ny;

    cfacepos[0] = 0;
    for (j=0; j<ny; ++j) {
        for (i=0; i<nx; ++i) {
            *cfaces++ = i+  Nx*j;
            *cfaces++ = i+  nx*j  +nxf;
            *cfaces++ = i+1+Nx*j;
            *cfaces++ = i+  nx*(j+1)+nxf;

            cfacepos[1] = cfacepos[0]+4;
            ++cfacepos;
        }
    }

    for (j = 0; j < nx * ny; ++j) {
        G->cell_facetag[j*4 + 0] = 0;
        G->cell_facetag[j*4 + 1] = 2;
        G->cell_facetag[j*4 + 2] = 1;
        G->cell_facetag[j*4 + 3] = 3;
    }


    fnodes     = G->face_nodes;
    fnodepos   = G->face_nodepos;
    fcells     = G->face_cells;
    fnodepos[0] = 0;

    /* Faces with x-normal */
    for (j=0; j<ny; ++j) {
        for (i=0; i<nx+1; ++i) {
            *fnodes++ = i+Nx*j;
            *fnodes++ = i+Nx*(j+1);
            fnodepos[1] = fnodepos[0] + 2;
            ++fnodepos;
            if (i==0) {
                *fcells++ = -1;
                *fcells++ =  i+nx*j;
            }
            else if (i == nx) {
                *fcells++ =  i-1+nx*j;
                *fcells++ = -1;
            }
            else {
                *fcells++ =  i-1 + nx*j;
                *fcells++ =  i   + nx*j;
            }
        }
    }

    /* Faces with y-normal */
    for (j=0; j<ny+1; ++j) {
        for (i=0; i<nx; ++i) {
            *fnodes++ = i+1 + Nx*j;
            *fnodes++ = i+    Nx*j;
            fnodepos[1] = fnodepos[0] + 2;
            ++fnodepos;
            if (j==0) {
                *fcells++ = -1;
                *fcells++ =  i+nx*j;
            }
            else if (j == ny) {
                *fcells++ =  i+nx*(j-1);
                *fcells++ = -1;
            }
            else {
                *fcells++ =  i+nx*(j-1);
                *fcells++ =  i+nx*j;
            }
        }
    }

    fill_cart_indices(G);
}

/* --------------------------------------------------------------------- */

static void
fill_cart_geometry_2d(struct UnstructuredGrid *G,
                      const double            *x,
                      const double            *y)
{
    int    i,j;
    int    nx, ny;

    double dx, dy;

    double *coord, *ccentroids, *cvolumes;
    double *fnormals, *fcentroids, *fareas;

    nx  = G->cartdims[0];
    ny  = G->cartdims[1];

    ccentroids = G->cell_centroids;
    cvolumes   = G->cell_volumes;

    for (j=0; j<ny; ++j) {
        for (i=0; i<nx; ++i) {
            *ccentroids++ = (x[i] + x[i + 1]) / 2.0;
            *ccentroids++ = (y[j] + y[j + 1]) / 2.0;

            dx = x[i + 1] - x[i];
            dy = y[j + 1] - y[j];

            *cvolumes++ = dx * dy;
        }
    }



    fnormals   = G->face_normals;
    fcentroids = G->face_centroids;
    fareas     = G->face_areas;

    /* Faces with x-normal */
    for (j=0; j<ny; ++j) {
        for (i=0; i<nx+1; ++i) {
            dy = y[j + 1] - y[j];

            *fnormals++ = dy;
            *fnormals++ = 0;

            *fcentroids++ = x[i];
            *fcentroids++ = (y[j] + y[j + 1]) / 2.0;

            *fareas++ = dy;
        }
    }

    /* Faces with y-normal */
    for (j=0; j<ny+1; ++j) {
        for (i=0; i<nx; ++i) {
            dx = x[i + 1] - x[i];

            *fnormals++ = 0;
            *fnormals++ = dx;

            *fcentroids++ = (x[i] + x[i + 1]) / 2.0;
            *fcentroids++ = y[j];

            *fareas++ = dx;
        }
    }

    coord = G->node_coordinates;
    for (j=0; j<ny+1; ++j) {
        for (i=0; i<nx+1; ++i) {
            *coord++ = x[i];
            *coord++ = y[j];
        }
    }
}
