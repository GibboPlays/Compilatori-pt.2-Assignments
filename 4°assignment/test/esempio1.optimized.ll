; ModuleID = '../test/esempio1.bc'
source_filename = "../test/esempio1.ll"

@A = global [100 x i32] zeroinitializer
@B = global [100 x i32] zeroinitializer

define void @two_loops() {
entry:
  br label %loop1_header

loop1_header:                                     ; preds = %loop1_latch, %entry
  %i1 = phi i32 [ 0, %entry ], [ %i1_next, %loop1_latch ]
  %cond1 = icmp slt i32 %i1, 100
  br i1 %cond1, label %loop1_body, label %loop2_header

loop1_body:                                       ; preds = %loop1_header
  %a_ptr = getelementptr inbounds [100 x i32], ptr @A, i32 0, i32 %i1
  %a_val = load i32, ptr %a_ptr, align 4
  %a_add = add i32 %a_val, 1
  store i32 %a_add, ptr %a_ptr, align 4
  %i1_next = add i32 %i1, 1
  br label %loop1_latch

loop1_latch:                                      ; preds = %loop1_body
  br label %loop1_header

loop2_header:                                     ; preds = %loop2_latch, %loop1_header
  %i2 = phi i32 [ 0, %loop1_header ], [ %i2_next, %loop2_latch ]
  %cond2 = icmp slt i32 %i2, 100
  br i1 %cond2, label %loop2_body, label %end

loop2_body:                                       ; preds = %loop2_header
  %b_ptr = getelementptr inbounds [100 x i32], ptr @B, i32 0, i32 %i2
  %b_val = load i32, ptr %b_ptr, align 4
  %b_add = add i32 %b_val, 1
  store i32 %b_add, ptr %b_ptr, align 4
  %i2_next = add i32 %i2, 1
  br label %loop2_latch

loop2_latch:                                      ; preds = %loop2_body
  br label %loop2_header

end:                                              ; preds = %loop2_header
  ret void
}
