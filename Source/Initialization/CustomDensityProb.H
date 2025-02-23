/* Copyright 2019 Maxence Thevenet, Weiqun Zhang
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef CUSTOM_DENSITY_PROB_H_
#define CUSTOM_DENSITY_PROB_H_

#include "Utils/TextMsg.H"
#include "Utils/WarpXUtil.H"

#include <AMReX_Arena.H>
#include <AMReX_Dim3.H>
#include <AMReX_Gpu.H>
#include <AMReX_ParmParse.H>

// An example of Custom Density Profile

// struct whose getDensity returns density at a given position computed from
// a custom function, with runtime input parameters.
struct InjectorDensityCustom
{
    InjectorDensityCustom (std::string const& species_name)
    {
        // Read parameters for custom density profile from file
        amrex::ParmParse pp_species_name(species_name);
        std::vector<amrex::Real> v;
        WARPX_ALWAYS_ASSERT_WITH_MESSAGE(v.size() <= 6,
                                         "Too many parameters for InjectorDensityCustom");
        getArrWithParser(pp_species_name, "custom_profile_params", v);
        for (int i = 0; i < static_cast<int>(v.size()); ++i) {
            p[i] = v[i];
        }
    }

    // Return density at given position, using user-defined parameters
    // stored in p.
    AMREX_GPU_HOST_DEVICE
    amrex::Real
    getDensity (amrex::Real, amrex::Real, amrex::Real) const noexcept
    {
        return p[0];
    }

    // Note that we are not allowed to have non-trivial destructor.
    // So we rely on clear() to free memory if needed.
    void clear () {}

private:
    amrex::GpuArray<amrex::Real,6> p;
};

#endif
