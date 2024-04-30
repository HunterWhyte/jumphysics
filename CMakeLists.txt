cmake_minimum_required(VERSION 3.7)
project(jumphysics
	VERSION 0.0.1
	LANGUAGES C CXX)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
message(STATUS "${SOURCES}")

add_library(jumphysics ${SOURCES})

target_include_directories(jumphysics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_include_directories(jumphysics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ext")

SET(SOFTFLOAT_OPTS -DSOFTFLOAT_ROUND_ODD -DINLINE_LEVEL=5 -DSOFTFLOAT_FAST_DIV32TO16 -DSOFTFLOAT_FAST_DIV64TO32 -DSOFTFLOAT_FAST_INT64)

set(SOFTFLOAT_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/ext/softfloat/source")
file(GLOB SOFTFLOAT_SOURCES 
    "${SOFTFLOAT_PATH}/../trig.cpp"
    "${SOFTFLOAT_PATH}/8086/s_propagateNaNF16UI.c"
    "${SOFTFLOAT_PATH}/8086/s_propagateNaNF64UI.c"
    "${SOFTFLOAT_PATH}/8086/s_propagateNaNF32UI.c"
    "${SOFTFLOAT_PATH}/s_roundToUI32.c"
    "${SOFTFLOAT_PATH}/s_roundToUI64.c"
    "${SOFTFLOAT_PATH}/s_roundToI32.c"
    "${SOFTFLOAT_PATH}/s_roundToI64.c"
    "${SOFTFLOAT_PATH}/s_normSubnormalF16Sig.c"
    "${SOFTFLOAT_PATH}/s_roundPackToF16.c"
    "${SOFTFLOAT_PATH}/s_normRoundPackToF16.c"
    "${SOFTFLOAT_PATH}/s_addMagsF16.c"
    "${SOFTFLOAT_PATH}/s_subMagsF16.c"
    "${SOFTFLOAT_PATH}/s_mulAddF16.c"
    "${SOFTFLOAT_PATH}/s_normSubnormalF32Sig.c"
    "${SOFTFLOAT_PATH}/s_roundPackToF32.c"
    "${SOFTFLOAT_PATH}/s_normRoundPackToF32.c"
    "${SOFTFLOAT_PATH}/s_addMagsF32.c"
    "${SOFTFLOAT_PATH}/s_subMagsF32.c"
    "${SOFTFLOAT_PATH}/s_mulAddF32.c"
    "${SOFTFLOAT_PATH}/s_normSubnormalF64Sig.c"
    "${SOFTFLOAT_PATH}/s_roundPackToF64.c"
    "${SOFTFLOAT_PATH}/s_normRoundPackToF64.c"
    "${SOFTFLOAT_PATH}/s_addMagsF64.c"
    "${SOFTFLOAT_PATH}/s_subMagsF64.c"
    "${SOFTFLOAT_PATH}/s_mulAddF64.c"
    "${SOFTFLOAT_PATH}/s_countLeadingZeros8.c"
    "${SOFTFLOAT_PATH}/s_countLeadingZeros32.c"
    "${SOFTFLOAT_PATH}/s_shiftRightJam32.c"
    "${SOFTFLOAT_PATH}/s_shiftRightJam32.c"
    "${SOFTFLOAT_PATH}/s_shiftRightJam64.c"
    "${SOFTFLOAT_PATH}/s_shiftRightJam64Extra.c"
    "${SOFTFLOAT_PATH}/s_countLeadingZeros16.c"
    "${SOFTFLOAT_PATH}/s_shortShiftRightJam64.c"
    "${SOFTFLOAT_PATH}/s_countLeadingZeros64.c"
    "${SOFTFLOAT_PATH}/s_approxRecipSqrt32_1.c"
    "${SOFTFLOAT_PATH}/s_approxRecipSqrt_1Ks.c"
    "${SOFTFLOAT_PATH}/s_approxRecipSqrt_1k1.c"
    "${SOFTFLOAT_PATH}/softfloat_state.c"
    "${SOFTFLOAT_PATH}/8086/softfloat_raiseFlags.c"
    "${SOFTFLOAT_PATH}/f32_add.c"
    "${SOFTFLOAT_PATH}/f32_div.c"
    "${SOFTFLOAT_PATH}/f32.c"
    "${SOFTFLOAT_PATH}/f32_add.c"
    "${SOFTFLOAT_PATH}/f32_div.c"
    "${SOFTFLOAT_PATH}/f32_eq.c"
    "${SOFTFLOAT_PATH}/f32_eq_signaling.c"
    "${SOFTFLOAT_PATH}/f32_isSignalingNaN.c"
    "${SOFTFLOAT_PATH}/f32_le.c"
    "${SOFTFLOAT_PATH}/f32_le_quiet.c"
    "${SOFTFLOAT_PATH}/f32_lt.c"
    "${SOFTFLOAT_PATH}/f32_lt_quiet.c"
    "${SOFTFLOAT_PATH}/f32_mulAdd.c"
    "${SOFTFLOAT_PATH}/f32_mul.c"
    "${SOFTFLOAT_PATH}/f32_rem.c"
    "${SOFTFLOAT_PATH}/f32_roundToInt.c"
    "${SOFTFLOAT_PATH}/f32_sqrt.c"
    "${SOFTFLOAT_PATH}/f32_sub.c"
    "${SOFTFLOAT_PATH}/f32_to_i32.c"
    "${SOFTFLOAT_PATH}/f32_to_i32_r_minMag.c"
    "${SOFTFLOAT_PATH}/f32_to_i64.c"
    "${SOFTFLOAT_PATH}/f32_to_i64_r_minMag.c"
    "${SOFTFLOAT_PATH}/f32_to_ui32.c"
    "${SOFTFLOAT_PATH}/f32_to_ui32_r_minMag.c"
    "${SOFTFLOAT_PATH}/f32_to_ui64.c"
    "${SOFTFLOAT_PATH}/f32_to_ui64_r_minMag.c"
    "${SOFTFLOAT_PATH}/ui32_to_f32.c"
    "${SOFTFLOAT_PATH}/ui64_to_f32.c"
    "${SOFTFLOAT_PATH}/i32_to_f32.c"
    "${SOFTFLOAT_PATH}/i64_to_f32.c"
) 
add_library("softfloat" SHARED ${SOFTFLOAT_SOURCES})
target_include_directories(softfloat PUBLIC "${SOFTFLOAT_PATH}/8086" "${SOFTFLOAT_PATH}/include" ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(softfloat PRIVATE ${SOFTFLOAT_OPTS})

target_link_libraries(jumphysics softfloat)
