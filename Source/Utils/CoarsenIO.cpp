#include "CoarsenIO.H"

#include "Utils/TextMsg.H"

#include <AMReX_BLProfiler.H>
#include <AMReX_BLassert.H>
#include <AMReX_Box.H>
#include <AMReX_BoxArray.H>
#include <AMReX_Config.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_FabArray.H>
#include <AMReX_GpuControl.H>
#include <AMReX_GpuLaunch.H>
#include <AMReX_IndexType.H>
#include <AMReX_MFIter.H>
#include <AMReX_MultiFab.H>

using namespace amrex;

void
CoarsenIO::Loop ( MultiFab& mf_dst,
                  const MultiFab& mf_src,
                  const int dcomp,
                  const int scomp,
                  const int ncomp,
                  const IntVect ngrowvect,
                  const IntVect crse_ratio )
{
    // Staggering of source fine MultiFab and destination coarse MultiFab
    const IntVect stag_src = mf_src.boxArray().ixType().toIntVect();
    const IntVect stag_dst = mf_dst.boxArray().ixType().toIntVect();

    if ( crse_ratio > IntVect(1) ) WARPX_ALWAYS_ASSERT_WITH_MESSAGE( ngrowvect == IntVect(0),
        "option of filling guard cells of destination MultiFab with coarsening not supported for this interpolation" );

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE( mf_src.nGrowVect() >= stag_dst-stag_src+ngrowvect,
        "source fine MultiFab does not have enough guard cells for this interpolation" );

    // Auxiliary integer arrays (always 3D)
    GpuArray<int,3> sf; // staggering of source fine MultiFab
    GpuArray<int,3> sc; // staggering of destination coarse MultiFab
    GpuArray<int,3> cr; // coarsening ratio

    sf[0] = stag_src[0];
#if   defined(WARPX_DIM_1D_Z)
    sf[1] = 0;
#else
    sf[1] = stag_src[1];
#endif
#if   (AMREX_SPACEDIM <= 2)
    sf[2] = 0;
#elif defined(WARPX_DIM_3D)
    sf[2] = stag_src[2];
#endif

    sc[0] = stag_dst[0];
#if   defined(WARPX_DIM_1D_Z)
    sc[1] = 0;
#else
    sc[1] = stag_dst[1];
#endif
#if   (AMREX_SPACEDIM <= 2)
    sc[2] = 0;
#elif defined(WARPX_DIM_3D)
    sc[2] = stag_dst[2];
#endif

    cr[0] = crse_ratio[0];
#if   defined(WARPX_DIM_1D_Z)
    cr[1] = 1;
#else
    cr[1] = crse_ratio[1];
#endif
#if   (AMREX_SPACEDIM <= 2)
    cr[2] = 1;
#elif defined(WARPX_DIM_3D)
    cr[2] = crse_ratio[2];
#endif

#ifdef AMREX_USE_OMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    // Loop over boxes (or tiles if not on GPU)
    for (MFIter mfi( mf_dst, TilingIfNotGPU() ); mfi.isValid(); ++mfi)
    {
        // Tiles defined at the coarse level
        const Box& bx = mfi.growntilebox( ngrowvect );
        Array4<Real> const& arr_dst = mf_dst.array( mfi );
        Array4<Real const> const& arr_src = mf_src.const_array( mfi );
        ParallelFor( bx, ncomp,
                     [=] AMREX_GPU_DEVICE( int i, int j, int k, int n )
                     {
                         arr_dst(i,j,k,n+dcomp) = CoarsenIO::Interp(
                             arr_src, sf, sc, cr, i, j, k, n+scomp );
                     } );
    }
}

void
CoarsenIO::Coarsen ( MultiFab& mf_dst,
                     const MultiFab& mf_src,
                     const int dcomp,
                     const int scomp,
                     const int ncomp,
                     const int ngrow,
                     const IntVect crse_ratio )
{
    amrex::IntVect ngrowvect(ngrow);
    Coarsen(mf_dst,
            mf_src,
            dcomp,
            scomp,
            ncomp,
            ngrowvect,
            crse_ratio);
}

void
CoarsenIO::Coarsen ( MultiFab& mf_dst,
                     const MultiFab& mf_src,
                     const int dcomp,
                     const int scomp,
                     const int ncomp,
                     const IntVect ngrowvect,
                     const IntVect crse_ratio )
{
    BL_PROFILE("CoarsenIO::Coarsen()");

    // Convert BoxArray of source MultiFab to staggering of destination MultiFab and coarsen it
    BoxArray ba_tmp = amrex::convert( mf_src.boxArray(), mf_dst.ixType().toIntVect() );
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE( ba_tmp.coarsenable( crse_ratio ),
        "source MultiFab converted to staggering of destination MultiFab is not coarsenable" );
    ba_tmp.coarsen( crse_ratio );

    if ( ba_tmp == mf_dst.boxArray() and mf_src.DistributionMap() == mf_dst.DistributionMap() )
        CoarsenIO::Loop( mf_dst, mf_src, dcomp, scomp, ncomp, ngrowvect, crse_ratio );
    else
    {
        // Cannot coarsen into MultiFab with different BoxArray or DistributionMapping:
        // 1) create temporary MultiFab on coarsened version of source BoxArray with same DistributionMapping
        MultiFab mf_tmp( ba_tmp, mf_src.DistributionMap(), ncomp, 0, MFInfo(), FArrayBoxFactory() );
        // 2) interpolate from mf_src to mf_tmp (start writing into component 0)
        CoarsenIO::Loop( mf_tmp, mf_src, 0, scomp, ncomp, ngrowvect, crse_ratio );
        // 3) copy from mf_tmp to mf_dst (with different BoxArray or DistributionMapping)
        mf_dst.ParallelCopy( mf_tmp, 0, dcomp, ncomp );
    }
}
