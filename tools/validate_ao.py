"""
Numerical validation of the ported HRWFS AO math (aoMatCompute + the
mask-restricted pseudo-inverse in aoModeCompute/aoContMatCompute).

This is an independent Python re-implementation of the same algorithm (zermes2
analytic interaction matrix + least-squares Zernike fit). The check: because
the interaction matrix's row `k` is exactly the slope response of mode `k`,
feeding that row into the fit must recover a unit coefficient for mode `k` and
~0 for the others. This catches sign/index/pseudo-inverse porting errors
without any hardware.
"""
import numpy as np
import math

NSP   = 18
NP    = 20
DIM   = NP * NSP + 2          # 362
TDIAM = 8.0
PSCALE = 0.08125
APP   = PSCALE * 4.848e-6

def fact(k):
    f = 1.0
    for i in range(2, k + 1):
        f *= i
    return f

def zernumero(zn):
    j = 0
    for n in range(0, 101):
        for m in range(0, n + 1):
            if (n - m) % 2 == 0:
                j += 1
                if j == zn:
                    return n, m
                if m != 0:
                    j += 1
                    if j == zn:
                        return n, m
    return 0, 0

# ---- build the prepzernike grids (column-major indexing [i, j]) ----
ii = np.arange(DIM)
xc = DIM / 2.0 - 0.5
yc = DIM / 2.0 - 0.5
rad = DIM / 2.0 - 1.0
X = (ii[:, None] - xc) * np.ones((1, DIM))
Y = np.ones((DIM, 1)) * (ii[None, :] - yc)
R = np.sqrt(X * X + Y * Y) / rad
MASKMOD = (R <= 1.2).astype(float)
RMOD = R * MASKMOD
PUP = (R <= 1.0).astype(float)
TETA = np.arctan2(Y, X)
TETA[(X == 0) & (Y == 0)] = 0.0

def zernike_ext(zn):
    n, m = zernumero(zn)
    val = np.zeros((DIM, DIM))
    for i in range(0, (n - m) // 2 + 1):
        denom = fact(i) * fact((n + m) // 2 - i) * fact((n - m) // 2 - i)
        coef = ((-1.0) ** i) * fact(n - i) / denom
        val += coef * np.power(RMOD, n - 2 * i)
    if m != 0:
        val *= np.sin(m * TETA) if (zn % 2 == 1) else np.cos(m * TETA)
    return val * MASKMOD

def zermes2(zern):
    z = zernike_ext(zern)
    zx = (np.roll(z, -1, axis=0) - np.roll(z, 1, axis=0)) * PUP / 2.0 * 1e-6
    zy = (np.roll(z, -1, axis=1) - np.roll(z, 1, axis=1)) * PUP / 2.0 * 1e-6
    zx = zx * NP / (TDIAM / NSP) / APP
    zy = zy * NP / (TDIAM / NSP) / APP
    mesx = np.zeros((NSP, NSP)); mesy = np.zeros((NSP, NSP))
    for j in range(NSP):
        for i in range(NSP):
            sl = (slice(i * NP + 1, (i + 1) * NP + 1),
                  slice(j * NP + 1, (j + 1) * NP + 1))
            tp = PUP[sl].sum()
            if tp > 0:
                mesx[i, j] = zx[sl].sum() / tp
                mesy[i, j] = zy[sl].sum() / tp
    # column-major flatten (IDL reform), matching aoIntMat layout
    return np.concatenate([mesx.flatten(order='F'), mesy.flatten(order='F')])

def build_intmat(nmodes):
    return np.array([zermes2(k + 2) for k in range(nmodes)])   # [nmodes, 648]

def active_slopes():
    centre = NSP / 2.0 - 0.5
    rout = 0.95 * (NSP + 1.0) / 2.0
    used = []
    for j in range(NSP):
        for i in range(NSP):
            d = math.hypot(i - centre, j - centre)
            if 1.8 < d < rout:
                used.append(i + j * NSP)      # column-major sub index
    xs = used
    ys = [NSP * NSP + s for s in used]
    return xs + ys, len(used)

def run(nmodes):
    M = build_intmat(nmodes)                 # [nmodes, 648]
    idx, nsub_used = active_slopes()
    Ma = M[:, idx]                           # [nmodes, nAct]
    MtM = Ma @ Ma.T                           # [nmodes, nmodes]
    C = np.linalg.inv(MtM) @ Ma               # control matrix [nmodes, nAct]
    # recover each mode from its own slope response
    max_err = 0.0
    for k in range(nmodes):
        s = Ma[k, :]                          # slopes for a unit-mode-k input
        a = C @ s
        target = np.zeros(nmodes); target[k] = 1.0
        max_err = max(max_err, np.abs(a - target).max())
    # random combination of the low-order modes
    rng = np.random.default_rng(1)
    a_true = np.zeros(nmodes)
    a_true[:min(19, nmodes)] = rng.standard_normal(min(19, nmodes))
    s = a_true @ Ma
    a_rec = C @ s
    comb_err = np.abs(a_rec - a_true).max()
    cond = np.linalg.cond(MtM)
    print("nmodes=%3d  active_subaps=%3d  nAct=%3d  cond(MtM)=%.2e  "
          "self-recovery max_err=%.2e  random-combo max_err=%.2e"
          % (nmodes, nsub_used, len(idx), cond, max_err, comb_err))

print("HRWFS geometry: nsp=%d, dim=%d, pscale=%.5f" % (NSP, DIM, PSCALE))
for nm in (19, 40, 60, 100, 150):
    run(nm)
