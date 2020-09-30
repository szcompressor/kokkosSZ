/**
 * @file ksz.cpp
 * @author Jiannan Tian
 * @brief
 * @version 0.1
 * @date 2020-09-30
 *
 * @copyright (C) 2020 by Washington State University, Argonne National Laboratory
 *
 */

#include <Kokkos_Core.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include "argparse.hh"
#include "io.hh"
#include "timer.hh"
#include "types.hh"
#include "verify.hh"

// uncomment to use omptarget backend (pass)
typedef Kokkos::Experimental::OpenMPTarget      ExeSpace;
typedef Kokkos::Experimental::OpenMPTargetSpace MemSpace;

// uncomment to use CUDA backend (pass)
// typedef Kokkos::Cuda                  ExeSpace;
// typedef Kokkos::CudaSpace             MemSpace;
// uncomment to use omp backend ()
// typedef Kokkos::OpenMP                ExeSpace;
// typedef Kokkos::OpenMP                MemSpace;
typedef Kokkos::LayoutRight           Layout;
typedef Kokkos::RangePolicy<ExeSpace> range_policy;

typedef Kokkos::View<
    float*,  //
    ExeSpace::scratch_memory_space,
    Kokkos::MemoryTraits<Kokkos::Unmanaged>>  // raw C type
    ScratchViewType;

typedef Kokkos::View<float*, Layout, MemSpace>    dtype;  // data type
typedef Kokkos::View<uint16_t*, Layout, MemSpace> qtype;  // quant type

typedef Kokkos::TeamPolicy<ExeSpace>              team_policy;
typedef Kokkos::TeamPolicy<ExeSpace>::member_type member_type;

template <typename T>
struct DryRun {
    dtype  a;
    double ebx2;
    double ebx2_r;
    DryRun(dtype a_, double ebx2_) : a(a_), ebx2(ebx2_) { ebx2_r = 1 / ebx2; }

    KOKKOS_INLINE_FUNCTION
    void operator()(const int i) const { a(i) = std::round(a(i) * ebx2_r) * ebx2; }
};

struct DualQuant1D {
    dtype a;
    qtype q;
    // member_type teamMember;
    double ebx2, ebx2_r;
    size_t dim0, radius;

    DualQuant1D(dtype a_, qtype q_, size_t dim0_, double ebx2_, size_t radius_) : a(a_), q(q_), dim0(dim0_), ebx2(ebx2_), radius(radius_) { ebx2_r = 1 / ebx2; }

    KOKKOS_INLINE_FUNCTION
    void operator()(const member_type& team_member) const
    {
        // CUDA counterpart:
        // auto id = blockDim.x * blockIdx.x + threadIdx.x;
        auto id = team_member.league_rank() /*blockIdx*/ * team_member.team_size() /*blockDim*/ + team_member.team_rank() /*threadIdx*/;
        if (id >= dim0) return;

        a(id) = std::round(a(id) * ebx2_r);

        // CUDA counterpart
        // __syncthreads();
        team_member.team_barrier();

        // CUDA counterpart
        // float pred = threadIdx.x == 0? 0: a[id-1];
        float pred      = team_member.team_rank() == 0 ? 0 : a(id - 1);
        float posterror = a(id) - pred;

        bool q_able = fabs(posterror) < radius;

        // CUDA counterpart
        // __syncthreads();
        team_member.team_barrier();

        q(id) = q_able * static_cast<uint16_t>(posterror + radius);
        a(id) = (1 - q_able) * a(id);
    }
};

int main(int argc, char* argv[])
{
    auto ap        = new argpack(argc, argv);
    auto eb_config = new config_t(ap->dict_size, ap->mantissa, ap->exponent);
    if (ap->mode == "r2r") {
        auto valrng = GetDatumValueRange<float>(ap->fname, ap->d0);
        eb_config->ChangeToRelativeMode(valrng);
    }

    Kokkos::initialize(argc, argv);

    {
        auto real_len = ap->d0;
        auto pod_data = io::ReadBinaryFile<float>(ap->fname, real_len);

        // auto len = 1024; // for shorter run
        auto len = real_len;

        auto data  = dtype("data", len);
        auto quant = qtype("qtype", len);

        dtype::HostMirror h_data  = Kokkos::create_mirror_view(data);
        qtype::HostMirror h_quant = Kokkos::create_mirror_view(quant);

        for (auto i = 0; i < len; i++) h_data(i) = pod_data[i];
        Kokkos::deep_copy(data, h_data);

        Kokkos::Timer timer;

        // temporarily, we need to change team_size to change block size
        auto team_size = 32;                                              // CUDA: dim3 blockDim(??);
                                                                          // #endif
        auto league_size = (data.extent(0) /*len*/ - 1) / team_size + 1;  // CUDA: dim3 gridDim( (len-1)/128 + 1 );

        cout << "!! using team_size (blockDim) = " << team_size << endl;

        timer.reset();

        auto a = hires::now();

        // cudaLaunchKernel(gridDim, blockDim, SHAREDMEMORY=0, args, kernel name);
        Kokkos::parallel_for(
            team_policy(league_size, team_size),                                       //
            DualQuant1D(data, quant, len, 2 * eb_config->eb_final, ap->dict_size / 2)  //
        );

        // CUDA counterpart: cudaDeviceSynchronize();
        Kokkos::fence();

        auto b = hires::now();

        cout << "throughput: " << (len * sizeof(float) * 1.0) / (1024 * 1024 * 1024) / static_cast<duration_t>(b - a).count() << " GB/s" << endl;

        auto              data_dryrun   = dtype("data_dryrun", len);
        dtype::HostMirror h_data_dryrun = Kokkos::create_mirror_view(data_dryrun);
        for (auto i = 0; i < len; i++) h_data_dryrun(i) = pod_data[i];

        Kokkos::deep_copy(data_dryrun, h_data_dryrun);
        Kokkos::parallel_for(data_dryrun.extent(0), DryRun<float>(data_dryrun, 2 * eb_config->eb_final));
        Kokkos::deep_copy(h_data_dryrun, data_dryrun);

        auto pod_data_dryrun = new float[len];
        for (auto i = 0; i < len; i++) pod_data_dryrun[i] = h_data_dryrun(i);

        analysis::VerifyData(
            pod_data_dryrun,      //
            pod_data,             //
            len,                  //
            false,                // no overriding
            eb_config->eb_final,  //
            0,                    // archive size that won't work
            1                     // scale that won't work because it's about binning
        );
    }
    Kokkos::finalize();
    return 0;
}
