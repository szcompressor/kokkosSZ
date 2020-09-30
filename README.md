kokkosSZ (kSZ): A Portable Accelerator Implementation of SZ Using Kokkos  
---

# introduction

kSZ is a [Kokkos](https://github.com/kokkos/kokkos)-based implementation of the world-widely used [SZ lossy compressor](https://github.com/szcompressor/SZ). We use Kokkos because it provides abstractions for both parallel execution of code and data management, which can be used to support portable implementation across different accelerator technologies. Kokkos can support OpenMP/OpenMPTarget, oneAPI, Pthreads, and CUDA as backend programming models. 

(C) 2020 by Washington State University and Argonne National Laboratory. See COPYRIGHT in top-level directory.

Developers: Jiannan Tian, Dingwen Tao, Sheng Di, Franck Cappello

# compile
The toolchain on login node is by default okay,
```bash
git clone git@github.com:jtian0/kSZ-jtian.git ksz-omptarget
cd ksz-omptarget
make -j8
```

# run on target testbed
```bash
qsub -I -t 60 -n 1 -q <testbed name>
```
You may want to tune omp following the instruction
```bash
# In general, for best performance with OpenMP 4.0 or better set OMP_PROC_BIND=spread and OMP_PLACES=threads
#   For best performance with OpenMP 3.1 set OMP_PROC_BIND=true
#   For unit testing set OMP_PROC_BIND=false

export OMP_PROC_BIND=spread 
export OMP_PLACES=threads
```
And to run,
```bash
./ksz -f32 -m r2r -e 1e-4 -i ~/280953867/xx.f32 -1 280953867 -z
```
A sample output is given below
```
[info] bin.cap:		1024
[info] user-set eb:	1 x 10^(-4) = 0.0001
[info] change to r2r mode (relative-to-value-range)
       eb --> 0.0001 x 64 = 0.0064
Kokkos::OpenMP::initialize WARNING: OMP_PROC_BIND environment variable not set
  In general, for best performance with OpenMP 4.0 or better set OMP_PROC_BIND=spread and OMP_PLACES=threads
  For best performance with OpenMP 3.1 set OMP_PROC_BIND=true
  For unit testing set OMP_PROC_BIND=false
!! using team_size (blockDim) = 32
throughput: 2.95641 GB/s

[info] verification start ---------------------
| min.val             0
| max.val             63.999996185302734375
| val.rng             63.999996185302734375
| max.err.abs.val     0.00640106201171875
| max.err.abs.idx     145043941
| max.err.vs.rng      0.00010001659989455937414
| max.pw.rel.err      1
| PSNR                84.771440846981420236
| NRMSE               5.7733509432378176415E-05
| correl.coeff        0.99999997929479633729
[info] verification end -----------------------
```
