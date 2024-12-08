.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.text
.align 2
.thumb_func
.global atan_asm

/**
 * int32_t atan_asm(int16_t yData, int16_t xData)
 *
 * Calculates atan2(yData, xData) and returns the angle in degrees.
 * This function handles special cases and ensures accurate results.
 *
 * Parameters:
 *   - R0: yData (int16_t)
 *   - R1: xData (int16_t)
 *
 * Returns:
 *   - R0: Angle in degrees (int32_t)
 */

atan_asm:
    @ Preserve callee-saved registers and link register
    push    {r4-r7, lr}           @ Save R4-R7 and LR on the stack

    @ Sign-extend yData and xData from int16_t to int32_t
    sxth    r0, r0                @ Sign-extend yData (R0)
    sxth    r1, r1                @ Sign-extend xData (R1)

    @ Check for special cases
    cmp     r0, #0                @ Compare yData with zero
    bne     check_xdata           @ If yData != 0, proceed to check xData

    @ Handle special cases where yData == 0
    cmp     r1, #0                @ Check if xData == 0
    beq     y_zero                @ If xData == 0, handle accordingly

check_xdata:
    cmp     r1, #0                @ Compare xData with zero
    beq     x_zero                @ If xData == 0, handle accordingly

proceed:
    @ Convert yData and xData to floats
    vmov    s0, r0                @ Move yData into S0 as integer bits
    vcvt.f32.s32 s0, s0           @ Convert S0 from int32_t to float

    vmov    s1, r1                @ Move xData into S1 as integer bits
    vcvt.f32.s32 s1, s1           @ Convert S1 from int32_t to float

    @ Call atan2f function
    bl      atan2f                @ Call atan2f(yData, xData)
    @ Result is in S0 (radians)

    @ Multiply result by (180 / pi) to convert to degrees
    adr     r4, const_180_over_pi @ Load address of constant into R4
    vldr    s1, [r4]              @ Load constant (180 / pi) into S1
    vmul.f32 s0, s0, s1           @ S0 = S0 * S1 (convert radians to degrees)

    @ Round the result to the nearest integer
    vldr    s2, [r4, #4]          @ Load the value 0.5 from memory into S2
    vadd.f32 s0, s0, s2           @ Add 0.5 to S0 to achieve rounding
    vcvt.s32.f32 s0, s0           @ Convert S0 from float to int32_t (round down)
    vmov    r0, s0                @ Move result from S0 to R0

    @ Restore registers and return
    pop     {r4-r7, pc}           @ Restore R4-R7 and return

y_zero:
    @ Handle case when yData == 0
    cmp     r1, #0                @ Check xData
    bgt     x_positive            @ If xData > 0, angle = 0
    blt     x_negative            @ If xData < 0, angle = 180 or -180
    @ xData == 0, undefined angle
    movs    r0, #0                @ Return 0 for undefined angle
    b       end_function

x_zero:
    @ Handle case when xData == 0
    cmp     r0, #0                @ Check yData
    bgt     y_positive            @ If yData > 0, angle = 90
    blt     y_negative            @ If yData < 0, angle = -90
    @ yData == 0, undefined angle (should not reach here)
    movs    r0, #0                @ Return 0 for undefined angle
    b       end_function

x_positive:
    movs    r0, #0                @ Angle = 0 degrees
    b       end_function

x_negative:
    movs    r0, #180              @ Angle = 180 degrees
    b       end_function

y_positive:
    movs    r0, #90               @ Angle = 90 degrees
    b       end_function

y_negative:
    movs    r0, #-90              @ Angle = -90 degrees
    b       end_function

end_function:
    @ Restore registers and return
    pop     {r4-r7, pc}           @ Restore R4-R7 and return

.align 4
const_180_over_pi:
    .float 57.2957795             @ Constant value (180 / pi)
    .float 0.5                    @ Constant value 0.5 for rounding
