; RUN: opt %s -load ../../build/src/libAddressInterceptorPassModule.so \
; RUN:  -adin -S -adin-simple-global-skip=false | FileCheck %s
; ModuleID = '../src/mem_fn.c'
source_filename = "../src/mem_fn.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%union.SYSCTRL_OSC8M_Type = type { %struct.anon }
%struct.anon = type { i32 }
%struct.Sysctrl = type { [3 x i8], %union.SYSCTRL_OSC8M_Type }

; Function Attrs: noinline nounwind optnone uwtable
define void @q(i8*) #0 {
  %2 = alloca i8*, align 8
  %3 = alloca [11 x i8], align 1
  %4 = alloca %union.SYSCTRL_OSC8M_Type, align 4
  store i8* %0, i8** %2, align 8
  %5 = bitcast %union.SYSCTRL_OSC8M_Type* %4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %5, i8* bitcast (%union.SYSCTRL_OSC8M_Type* getelementptr inbounds (%struct.Sysctrl, %struct.Sysctrl* null, i32 0, i32 1) to i8*), i64 4, i32 4, i1 false)
  %6 = bitcast %union.SYSCTRL_OSC8M_Type* %4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* bitcast (%union.SYSCTRL_OSC8M_Type* getelementptr inbounds (%struct.Sysctrl, %struct.Sysctrl* null, i32 0, i32 1) to i8*), i8* %6, i64 4, i32 4, i1 false)
  %7 = load i8*, i8** %2, align 8
  call void @llvm.memset.p0i8.i64(i8* %7, i8 0, i64 11, i32 1, i1 false)
  %8 = getelementptr inbounds [11 x i8], [11 x i8]* %3, i32 0, i32 0
  %9 = load i8*, i8** %2, align 8
  call void @llvm.memmove.p0i8.p0i8.i64(i8* %8, i8* %9, i64 11, i32 1, i1 false)
; CHECK-LABEL: @q(
; CHECK: call void @__adin_memcpy_(i8* %5, i8* bitcast (%union.SYSCTRL_OSC8M_Type* getelementptr inbounds (%struct.Sysctrl, %struct.Sysctrl* null, i32 0, i32 1) to i8*), i32 4)
; CHECK: call void @__adin_memcpy_(i8* bitcast (%union.SYSCTRL_OSC8M_Type* getelementptr inbounds (%struct.Sysctrl, %struct.Sysctrl* null, i32 0, i32 1) to i8*), i8* %6, i32 4)
; CHECK: call void @__adin_memset_(i8* %7, i8 0, i32 11)
; CHECK: call void @__adin_memmove_(i8* %8, i8* %9, i32 11)
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i32, i1) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memmove.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
