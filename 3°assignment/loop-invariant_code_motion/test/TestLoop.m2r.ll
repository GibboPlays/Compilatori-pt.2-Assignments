; ModuleID = 'TestLoop.ll'
source_filename = "TestLoop.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: mustprogress noinline nounwind uwtable
define dso_local noundef i32 @_Z7exampleii(i32 noundef %0, i32 noundef %1) #0 {
  %3 = mul nsw i32 %1, 10
  br label %4

4:                                                ; preds = %15, %2
  %.01 = phi i32 [ 0, %2 ], [ %.1, %15 ]
  %.0 = phi i32 [ 0, %2 ], [ %16, %15 ]
  %5 = icmp slt i32 %.0, %0
  br i1 %5, label %6, label %17

6:                                                ; preds = %4
  %7 = add nsw i32 %.0, %1
  %8 = mul nsw i32 %3, 5
  %9 = icmp sgt i32 %7, 100
  br i1 %9, label %10, label %12

10:                                               ; preds = %6
  %11 = add nsw i32 %.01, %8
  br label %14

12:                                               ; preds = %6
  %13 = add nsw i32 %.01, %7
  br label %14

14:                                               ; preds = %12, %10
  %.1 = phi i32 [ %11, %10 ], [ %13, %12 ]
  br label %15

15:                                               ; preds = %14
  %16 = add nsw i32 %.0, 1
  br label %4, !llvm.loop !6

17:                                               ; preds = %4
  %18 = add nsw i32 %1, %0
  ret i32 %.01
}

; Function Attrs: mustprogress noinline nounwind uwtable
define dso_local void @_Z9memoryOpsPii(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %12, %2
  %.01 = phi i32 [ 0, %2 ], [ %9, %12 ]
  %.0 = phi i32 [ 0, %2 ], [ %13, %12 ]
  %4 = icmp slt i32 %.0, %1
  br i1 %4, label %5, label %14

5:                                                ; preds = %3
  %6 = sext i32 %.0 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  %8 = load i32, ptr %7, align 4
  %9 = add nsw i32 %.01, %8
  %10 = sext i32 %.0 to i64
  %11 = getelementptr inbounds i32, ptr %0, i64 %10
  store i32 %9, ptr %11, align 4
  br label %12

12:                                               ; preds = %5
  %13 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !8

14:                                               ; preds = %3
  ret void
}

; Function Attrs: mustprogress noinline nounwind uwtable
define dso_local noundef i32 @_Z13multipleExitsii(i32 noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %14, %2
  %.02 = phi i32 [ 0, %2 ], [ %6, %14 ]
  %.01 = phi i32 [ %0, %2 ], [ %7, %14 ]
  %4 = icmp sgt i32 %.01, 0
  br i1 %4, label %5, label %15

5:                                                ; preds = %3
  %6 = add nsw i32 %.02, %1
  %7 = add nsw i32 %.01, -1
  %8 = icmp sgt i32 %6, 1000
  br i1 %8, label %9, label %10

9:                                                ; preds = %5
  br label %16

10:                                               ; preds = %5
  %11 = srem i32 %7, 2
  %12 = icmp eq i32 %11, 0
  br i1 %12, label %13, label %14

13:                                               ; preds = %10
  br label %15

14:                                               ; preds = %10
  br label %3, !llvm.loop !9

15:                                               ; preds = %13, %3
  %.1 = phi i32 [ %6, %13 ], [ %.02, %3 ]
  br label %16

16:                                               ; preds = %15, %9
  %.0 = phi i32 [ -1, %9 ], [ %.1, %15 ]
  ret i32 %.0
}

attributes #0 = { mustprogress noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 19.1.7 (++20250114103320+cd708029e0b2-1~exp1~20250114103432.75)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
