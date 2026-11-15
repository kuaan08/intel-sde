.text
.intel_syntax noprefix

.globl fixture_vaddps_xmm
fixture_vaddps_xmm:
    vaddps xmm0, xmm1, xmm2
    ret

.globl fixture_vsubps_xmm
fixture_vsubps_xmm:
    vsubps xmm0, xmm1, xmm2
    ret

.globl fixture_vmulps_xmm
fixture_vmulps_xmm:
    vmulps xmm0, xmm1, xmm2
    ret

.globl fixture_vdivps_xmm
fixture_vdivps_xmm:
    vdivps xmm0, xmm1, xmm2
    ret

.globl fixture_vxorps_xmm
fixture_vxorps_xmm:
    vxorps xmm0, xmm1, xmm2
    ret

.globl fixture_vandps_xmm
fixture_vandps_xmm:
    vandps xmm0, xmm1, xmm2
    ret

.globl fixture_vorps_xmm
fixture_vorps_xmm:
    vorps xmm0, xmm1, xmm2
    ret

.globl fixture_vaddps_ymm_high_regs
fixture_vaddps_ymm_high_regs:
    vaddps ymm12, ymm9, ymm14
    ret

.globl fixture_vmovups_xmm
fixture_vmovups_xmm:
    vmovups xmm3, xmm7
    ret

.globl fixture_vmovaps_xmm
fixture_vmovaps_xmm:
    vmovaps xmm3, xmm7
    ret

.globl fixture_vzeroupper
fixture_vzeroupper:
    vzeroupper
    ret
