/* Copyright 2019 Remi Lehe, Edoardo Zoni
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_SPECTRAL_BASE_ALGORITHM_H_
#define WARPX_SPECTRAL_BASE_ALGORITHM_H_

#include "FieldSolver/SpectralSolver/SpectralKSpace.H"
#include "Utils/WarpX_Complex.H"

#include "FieldSolver/SpectralSolver/SpectralFieldData_fwd.H"
#include "FieldSolver/SpectralSolver/SpectralFieldData.H"

#include <AMReX_BaseFab.H>
#include <AMReX_Config.H>
#include <AMReX_FabArray.H>
#include <AMReX_REAL.H>

#include <AMReX_BaseFwd.H>

#include <array>
#include <memory>

#if WARPX_USE_PSATD

/* \brief Class that updates the field in spectral space
 * and stores the coefficients of the corresponding update equation.
 *
 * `SpectralBaseAlgorithm` is only a base class and cannot be used directly.
 * Instead use its subclasses, which implement the specific field update
 * equations for a given spectral algorithm.
 */
class SpectralBaseAlgorithm
{
    public:
        // Virtual member function ; meant to be overridden in subclasses
        virtual void pushSpectralFields(SpectralFieldData& f) const = 0;

        // The destructor should also be a virtual function, so that
        // a pointer to subclass of `SpectraBaseAlgorithm` actually
        // calls the subclass's destructor.
        virtual ~SpectralBaseAlgorithm() {}

        /**
         * \brief Virtual function for current correction in Fourier space
         * (<a href="https://doi.org/10.1016/j.jcp.2013.03.010"> Vay et al, 2013</a>).
         * This virtual function is pure and must be defined in derived classes.
         *
         * \param[in,out] field_data All fields in Fourier space
         */
        virtual void CurrentCorrection (SpectralFieldData& field_data) = 0;

        /**
         * \brief Virtual function for Vay current deposition in Fourier space
         * (<a href="https://doi.org/10.1016/j.jcp.2013.03.010"> Vay et al, 2013</a>).
         * This virtual function is pure and must be defined in derived classes.
         *
         * \param[in,out] field_data All fields in Fourier space
         */
        virtual void VayDeposition (SpectralFieldData& field_data) = 0;

        /**
         * \brief Compute spectral divergence of E
         */
        void ComputeSpectralDivE ( const int lev,
                                   SpectralFieldData& field_data,
                                   const std::array<std::unique_ptr<amrex::MultiFab>,3>& Efield,
                                   amrex::MultiFab& divE );

    protected: // Meant to be used in the subclasses

        amrex::IntVect m_fill_guards;

        using SpectralRealCoefficients = \
            amrex::FabArray< amrex::BaseFab <amrex::Real> >;
        using SpectralComplexCoefficients = \
            amrex::FabArray< amrex::BaseFab <Complex> >;

        /**
        * \brief Constructor
        */
        SpectralBaseAlgorithm(const SpectralKSpace& spectral_kspace,
                              const amrex::DistributionMapping& dm,
                              const SpectralFieldIndex& spectral_index,
                              const int norder_x, const int norder_y,
                              const int norder_z, const bool nodal,
                              const amrex::IntVect& fill_guards);

        SpectralFieldIndex m_spectral_index;

        // Modified finite-order vectors
        KVectorComponent modified_kx_vec;
#if defined(WARPX_DIM_3D)
        KVectorComponent modified_ky_vec;
#endif
        KVectorComponent modified_kz_vec;
};

#endif // WARPX_USE_PSATD
#endif // WARPX_SPECTRAL_BASE_ALGORITHM_H_
