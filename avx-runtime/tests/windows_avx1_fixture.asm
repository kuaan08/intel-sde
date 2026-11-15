        .code

avx_fixture_add PROC
        vaddps  xmm0, xmm0, xmm1
        vzeroupper
        ret
avx_fixture_add ENDP

        END
